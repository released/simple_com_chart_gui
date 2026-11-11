"""Affected Python receive/worker tests; fake transport only, no input desktop."""
import os
import sys
import time
import threading
import unittest
from pathlib import Path
from unittest.mock import patch

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src" / "python"))
from core.serial_manager import SerialManager, MAX_RX_BUFFER


class FakeSerial:
    is_open = True

    def __init__(self, chunks):
        self.chunks = list(chunks)

    @property
    def in_waiting(self):
        return len(self.chunks[0]) if self.chunks else 0

    def read(self, count):
        return self.chunks.pop(0) if self.chunks else b""

    def close(self):
        self.is_open = False


class ReceiveTests(unittest.TestCase):
    def read(self, chunks):
        manager = SerialManager()
        manager._ser = FakeSerial(chunks)
        lines = []
        for _ in chunks:
            lines.extend(manager.read_lines())
        return manager, lines

    def test_every_utf8_split(self):
        text = "\u6eab\u5ea6:25.5,\u6e2c\u8a66:\U0001f642"
        raw = (text + "\r\n").encode("utf-8")
        for boundary in range(1, len(raw)):
            with self.subTest(boundary=boundary):
                manager, lines = self.read([raw[:boundary], raw[boundary:]])
                self.assertEqual(lines, [text])
                self.assertEqual(manager.consume_rx_overflow(), 0)

    def test_large_batch_of_complete_rows(self):
        expected = [f"value:{i}" for i in range(2000)]
        raw = ("\r\n".join(expected) + "\r\n").encode()
        self.assertGreater(len(raw), MAX_RX_BUFFER)
        manager, lines = self.read([raw])
        self.assertEqual(lines, expected)
        self.assertEqual(manager.consume_rx_overflow(), 0)

    def test_oversized_row_never_parses_suffix(self):
        chunks = [b"A" * 4000, b"B" * 200, b"value:999", b"\nvalue:12\r\n"]
        manager, lines = self.read(chunks)
        self.assertEqual(lines, ["value:12"])
        self.assertEqual(manager.consume_rx_overflow(), 4210)
        self.assertEqual(manager.consume_rx_overflow(), 0)
        self.assertFalse(manager._discarding_line)
        self.assertEqual(manager._rx_buffer, b"")

    def test_boundaries_invalid_utf8_and_reset(self):
        manager, lines = self.read([b"x" * MAX_RX_BUFFER + b"\n", b"y" * (MAX_RX_BUFFER + 1) + b"\nok\n", b"bad:\xff\n"])
        self.assertEqual(lines, ["x" * MAX_RX_BUFFER, "ok", "bad:\ufffd"])
        self.assertEqual(manager.consume_rx_overflow(), MAX_RX_BUFFER + 2)
        manager._discarding_line = True
        manager.disconnect()
        self.assertFalse(manager._discarding_line)


class WorkerLifetimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from PyQt5.QtWidgets import QApplication
        cls.app = QApplication.instance() or QApplication([])

    def pump_until(self, predicate, seconds=4):
        end = time.monotonic() + seconds
        while not predicate() and time.monotonic() < end:
            self.app.processEvents()
            time.sleep(.005)
        self.assertTrue(predicate(), "Timed out waiting for owned worker")

    def run_stop(self, closing):
        from PyQt5.QtGui import QCloseEvent
        from ui.main_window import MainWindow

        class SlowManager:
            def __init__(self):
                self.connected = True
                self.started = threading.Event()
                self.reading = False
                self.closed_while_reading = False
                self.close_count = 0

            def is_connected(self):
                return self.connected

            def scan_ports(self):
                return []

            def read_lines(self):
                self.reading = True
                self.started.set()
                time.sleep(1.25)
                self.reading = False
                return ["late:999"]

            def consume_rx_overflow(self):
                return 0

            def disconnect(self):
                self.closed_while_reading |= self.reading
                self.close_count += 1
                self.connected = False

        with patch("ui.main_window.SerialManager", SlowManager):
            window = MainWindow()
        manager = window.serial_mgr
        try:
            window._start_serial_worker()
            generation = window._serial_generation
            self.pump_until(manager.started.is_set)
            begin = time.monotonic()
            if closing:
                event = QCloseEvent()
                window.closeEvent(event)
                self.assertFalse(event.isAccepted())
            else:
                window._disconnect()
            self.assertLess(time.monotonic() - begin, .2)
            self.assertIsNotNone(window._serial_thread)
            self.assertTrue(window._serial_stopping)
            self.assertEqual(manager.close_count, 0)
            self.assertFalse(window.btn_connect.isEnabled())
            self.assertFalse(window._connect_with_validation())
            self.pump_until(lambda: window._serial_thread is None)
            self.assertFalse(manager.closed_while_reading)
            self.assertGreater(manager.close_count, 0)
            self.assertEqual(window._pending_lines, [])
            self.assertEqual(window.model.get_keys(), [])
            window._on_serial_lines(["stale:1"], time.time(), generation - 1)
            self.assertEqual(window._pending_lines, [])
            window._on_serial_lines(["late:1"], time.time(), generation)
            self.assertEqual(window._pending_lines, [])
            if not closing:
                self.assertTrue(window.btn_connect.isEnabled())
                self.assertEqual(window._sb_conn.text(), "COM: Disconnected")
        finally:
            self.pump_until(lambda: window._serial_thread is None)
            window.close()
            window.deleteLater()
            self.app.processEvents()

    def test_slow_disconnect(self):
        self.run_stop(False)

    def test_close_defers_resource_destruction(self):
        self.run_stop(True)

    def test_device_error_can_finish_before_ui_stop_request(self):
        from ui.main_window import MainWindow
        from serial.serialutil import SerialException

        class RemovedManager:
            connected = True
            close_count = 0

            def scan_ports(self):
                return []

            def is_connected(self):
                return self.connected

            def read_lines(self):
                raise SerialException("synthetic removal")

            def disconnect(self):
                self.connected = False
                self.close_count += 1

        with patch("ui.main_window.SerialManager", RemovedManager):
            window = MainWindow()
        try:
            window._start_serial_worker()
            self.pump_until(lambda: window._serial_thread is None)
            self.assertGreater(window.serial_mgr.close_count, 0)
            self.assertFalse(window.btn_connect.isChecked())
            self.assertTrue(window.btn_connect.isEnabled())
            self.assertIn("device removed", window._sb_conn.text())
        finally:
            self.pump_until(lambda: window._serial_thread is None)
            window.close()
            window.deleteLater()
            self.app.processEvents()


if __name__ == "__main__":
    unittest.main(verbosity=2)
