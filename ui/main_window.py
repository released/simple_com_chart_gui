from pathlib import Path
import time
import serial

from PyQt5.QtWidgets import (
    QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QComboBox, QGroupBox,
    QSplitter
)
from PyQt5.QtCore import (
    QTimer, Qt, QThread, QObject, pyqtSignal, pyqtSlot, QMetaObject
)

from PyQt5.QtGui import QIcon

from core.serial_manager import SerialManager
from core.log_parser import parse_kv_log
from core.channel_model import ChannelModel

from ui.plot_widget import MonitorPlotWidget
from ui.channel_panel import ChannelPanel
from ui.help_dialog import HelpDialog
from serial.serialutil import SerialException

READ_INTERVAL_MS = 20
HOTPLUG_SCAN_MS = 1000
UI_UPDATE_MS = 50
MAX_PENDING_LINES = 2000


# =========================================================
# Button Styles (QSS)
# =========================================================
BTN_BASE = """
QPushButton {
    color: white;
    font-weight: bold;
    padding: 6px 14px;
    border-radius: 6px;
}
QPushButton:pressed {
    background-color: #9E9E9E;
}
"""


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Plot Monitor")
        self.resize(1600, 900)
        icon_path = Path(__file__).resolve().parent.parent / "app.ico"
        if icon_path.exists():
            self.setWindowIcon(QIcon(str(icon_path)))
        self.setWindowState(self.windowState() | Qt.WindowMaximized)

        self.serial_mgr = SerialManager()
        self.model = ChannelModel()

        self._known_ports = []
        self._snapshot = False

        self._build_ui()

        self._serial_thread = None
        self._serial_worker = None
        self._pending_lines = []
        self._pending_now = None
        self._pending_dropped = 0
        self._ui_update_timer = QTimer(self)
        self._ui_update_timer.setSingleShot(True)
        self._ui_update_timer.timeout.connect(self._flush_pending_lines)

        # auto refresh timer
        self.auto_refresh_timer = QTimer(self)
        self.auto_refresh_timer.timeout.connect(self._on_auto_refresh)

        # hot-plug scan timer
        self.hotplug_timer = QTimer(self)
        self.hotplug_timer.timeout.connect(self._hotplug_scan)
        self.hotplug_timer.start(HOTPLUG_SCAN_MS)

    def closeEvent(self, event):
        self._stop_serial_worker()
        self.serial_mgr.disconnect()
        event.accept()

    # =========================================================
    # UI
    # =========================================================
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)

        root = QVBoxLayout(central)

        # ===== Status Bar =====
        status = self.statusBar()
        self._sb_conn = QLabel("COM: Disconnected")
        status.addWidget(self._sb_conn)

        self._sb_stat = QLabel("Samples: 0 | CH: 0")
        status.addPermanentWidget(self._sb_stat)

        # =====================================================
        # Top Row: COM / Connection
        # =====================================================
        top_bar = QHBoxLayout()

        self.combo_port = QComboBox()
        self.combo_port.setSizeAdjustPolicy(QComboBox.AdjustToContents)
        self.combo_port.setMinimumContentsLength(30)
        self.combo_port.setMinimumWidth(300)
        self.btn_scan = QPushButton("Scan")

        self.btn_connect = QPushButton("Connect")
        self.btn_connect.setCheckable(True)
        self._set_connect_btn_style(False)

        self.combo_baud = QComboBox()
        self.combo_baud.setEditable(True)
        self.combo_baud.addItems([
            "9600", "19200", "38400", "57600",
            "115200", "230400", "460800",
            "921600", "1000000", "2000000"
        ])
        self.combo_baud.setCurrentText("115200")

        self.combo_data = QComboBox()
        self.combo_data.addItems(["5", "6", "7", "8"])
        self.combo_data.setCurrentText("8")

        self.combo_parity = QComboBox()
        self.combo_parity.addItems(["NONE", "EVEN", "ODD"])
        self.combo_parity.setCurrentText("NONE")

        self.combo_stop = QComboBox()
        self.combo_stop.addItems(["1", "1.5", "2"])
        self.combo_stop.setCurrentText("1")

        top_bar.addWidget(QLabel("COM:"))
        top_bar.addWidget(self.combo_port)
        top_bar.addWidget(self.btn_scan)
        top_bar.addSpacing(8)
        top_bar.addWidget(self.btn_connect)
        top_bar.addSpacing(16)
        top_bar.addWidget(QLabel("Baud"))
        top_bar.addWidget(self.combo_baud)
        top_bar.addWidget(QLabel("Data"))
        top_bar.addWidget(self.combo_data)
        top_bar.addWidget(QLabel("Parity"))
        top_bar.addWidget(self.combo_parity)
        top_bar.addWidget(QLabel("Stop"))
        top_bar.addWidget(self.combo_stop)
        top_bar.addStretch()

        root.addLayout(top_bar)

        # =====================================================
        # Bottom Row: View / Control
        # =====================================================
        bottom_bar = QHBoxLayout()

        self.btn_refresh = QPushButton("Refresh")
        self.btn_fit = QPushButton("Fit")
        self.btn_snapshot = QPushButton("Snapshot")
        self.btn_snapshot.setCheckable(True)

        # NEW: Overlay toggle
        self.btn_overlay = QPushButton("Overlay")
        self.btn_overlay.setCheckable(True)
        self.btn_overlay.setChecked(True)

        # ---- Button styles ----
        self.btn_refresh.setStyleSheet(
            BTN_BASE + """
            QPushButton { background-color: #1976D2; }
            """
        )

        self.btn_fit.setStyleSheet(
            BTN_BASE + """
            QPushButton { background-color: #7B1FA2; }
            """
        )

        self.btn_snapshot.setStyleSheet(
            BTN_BASE + """
            QPushButton { background-color: #455A64; }
            QPushButton:checked { background-color: #EF6C00; }
            """
        )

        self.btn_overlay.setStyleSheet(
            BTN_BASE + """
            QPushButton { background-color: #2E7D32; }
            QPushButton:checked { background-color: #2E7D32; }
            QPushButton:!checked { background-color: #616161; }
            """
        )

        self.combo_auto = QComboBox()
        self.combo_auto.addItems(["Off", "5", "10", "30", "60"])
        self.combo_auto.setCurrentText("Off")

        self.combo_time = QComboBox()
        self.combo_time.addItems(["5", "10", "30", "60"])
        self.combo_time.setCurrentText("30")

        bottom_bar.addWidget(self.btn_refresh)
        bottom_bar.addWidget(self.btn_fit)
        bottom_bar.addWidget(self.btn_snapshot)
        bottom_bar.addWidget(self.btn_overlay)
        bottom_bar.addSpacing(16)
        bottom_bar.addWidget(QLabel("Auto Refresh (s):"))
        bottom_bar.addWidget(self.combo_auto)
        bottom_bar.addSpacing(16)
        bottom_bar.addWidget(QLabel("Time Window (s):"))
        bottom_bar.addWidget(self.combo_time)
        bottom_bar.addStretch()

        root.addLayout(bottom_bar)

        # =====================================================
        # Main Area (QSplitter)
        # =====================================================
        self.splitter = QSplitter(Qt.Horizontal)

        group = QGroupBox("Channels")
        group.setMinimumWidth(300)
        group_layout = QVBoxLayout(group)
        self.channel_panel = ChannelPanel()
        group_layout.addWidget(self.channel_panel)

        self.plot = MonitorPlotWidget()
        self.plot.set_overlay_enabled(True)

        self.splitter.addWidget(group)
        self.splitter.addWidget(self.plot)

        # initial splitter sizes: Channel / Plot
        self.splitter.setSizes([360, 1240])
        self.splitter.setStretchFactor(0, 1)
        self.splitter.setStretchFactor(1, 4)

        root.addWidget(self.splitter, stretch=1)

        # =====================================================
        # Signals
        # =====================================================
        self.btn_scan.clicked.connect(self._on_scan)
        self.btn_connect.toggled.connect(self._on_toggle_connect)

        self.btn_refresh.clicked.connect(self._on_refresh)
        self.btn_fit.clicked.connect(self._on_fit_clicked)
        self.btn_snapshot.toggled.connect(self._on_snapshot_toggled)
        self.btn_overlay.toggled.connect(self._on_overlay_toggled)

        self.combo_auto.currentTextChanged.connect(self._on_auto_changed)
        self.combo_time.currentTextChanged.connect(self._on_time_changed)

        help_menu = self.menuBar().addMenu("Help")

        action_log_format = help_menu.addAction("Log Format")
        action_log_format.triggered.connect(self._show_help_dialog)

    # =========================================================
    # Serial worker
    # =========================================================
    def _start_serial_worker(self):
        if self._serial_thread:
            return
        self._serial_thread = QThread(self)
        self._serial_worker = SerialReadWorker(self.serial_mgr, READ_INTERVAL_MS)
        self._serial_worker.moveToThread(self._serial_thread)
        self._serial_thread.started.connect(self._serial_worker.start)
        self._serial_worker.lines_ready.connect(self._on_serial_lines)
        self._serial_worker.disconnected.connect(self._on_serial_disconnected)
        self._serial_worker.disconnected.connect(self._serial_thread.quit)
        self._serial_thread.start()

    def _stop_serial_worker(self):
        if not self._serial_thread:
            return
        QMetaObject.invokeMethod(self._serial_worker, "stop", Qt.QueuedConnection)
        self._serial_thread.quit()
        self._serial_thread.wait(1000)
        self._serial_worker = None
        self._serial_thread = None

    # =========================================================
    # Button style helpers
    # =========================================================
    def _set_connect_btn_style(self, connected):
        if connected:
            self.btn_connect.setText("Disconnect")
            self.btn_connect.setStyleSheet(
                "QPushButton { background-color: #E53935; color: white; font-weight: bold; }"
            )
        else:
            self.btn_connect.setText("Connect")
            self.btn_connect.setStyleSheet(
                "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
            )

    def _set_connected_ui(self, connected):
        for w in (
            self.combo_port,
            self.combo_baud,
            self.combo_data,
            self.combo_parity,
            self.combo_stop,
            self.btn_scan,
        ):
            w.setEnabled(not connected)

    # =========================================================
    # Overlay
    # =========================================================
    def _on_overlay_toggled(self, checked):
        self.plot.set_overlay_enabled(checked)
        now = time.time()
        if not self._snapshot:
            self.plot.update_from_model(self.model, now)
        else:
            # frozen mode: keep current plot, just redraw overlay layer
            self.plot.refresh_overlay_only()

    # =========================================================
    # Snapshot
    # =========================================================
    def _on_snapshot_toggled(self, checked):
        self._snapshot = checked
        self.plot.set_frozen(checked)

        if checked:
            self.btn_snapshot.setText("Live")
        else:
            self.btn_snapshot.setText("Snapshot")
            now = time.time()
            self.plot.update_from_model(self.model, now)

    # =========================================================
    # Fit
    # =========================================================
    def _on_fit_clicked(self):
        self.plot.fit_enabled_channels()

    # =========================================================
    # Refresh / Auto
    # =========================================================
    def _on_refresh(self):
        now = time.time()
        self.model.reset_samples()
        self.plot.reset_visual()
        if not self._snapshot:
            self.plot.update_from_model(self.model, now)
        self._sb_stat.setText(
            f"Samples: 0 | CH: {self.model.get_enabled_count()}"
        )

    def _on_auto_changed(self, text):
        self.auto_refresh_timer.stop()
        if text == "Off":
            return
        try:
            sec = int(text)
            self.auto_refresh_timer.start(sec * 1000)
        except ValueError:
            pass

    def _on_auto_refresh(self):
        self._on_refresh()

    # =========================================================
    # COM scanning / hot-plug
    # =========================================================
    def _on_scan(self):
        ports = self.serial_mgr.scan_ports()
        self._update_port_combo(ports)

    def _hotplug_scan(self):
        if self.serial_mgr.is_connected():
            return
        ports = self.serial_mgr.scan_ports()
        devices = [dev for dev, _ in ports]
        if devices != self._known_ports:
            self._update_port_combo(ports)
            self._sb_conn.setText("COM: List updated")

    def _update_port_combo(self, ports):
        current_device = self.combo_port.currentData()
        if not current_device:
            current_device = self.combo_port.currentText().split(" ", 1)[0]
        self.combo_port.blockSignals(True)
        self.combo_port.clear()
        for device, desc in ports:
            label = f"{device} - {desc}"
            self.combo_port.addItem(label, userData=device)
        devices = [dev for dev, _ in ports]
        if current_device in devices:
            idx = devices.index(current_device)
            self.combo_port.setCurrentIndex(idx)
        self.combo_port.blockSignals(False)
        self._known_ports = devices[:]

    # =========================================================
    # Connection
    # =========================================================
    def _on_toggle_connect(self, checked):
        if checked:
            ok = self._connect_with_validation()
            if not ok:
                self.btn_connect.blockSignals(True)
                self.btn_connect.setChecked(False)
                self.btn_connect.blockSignals(False)
                self._set_connect_btn_style(False)
                return
        else:
            self._disconnect()

        self._set_connect_btn_style(checked)

    def _connect_with_validation(self):
        if self.combo_port.count() == 0:
            self._on_scan()

        ports = self.serial_mgr.scan_ports()
        port_devices = [dev for dev, _ in ports]
        if not port_devices:
            self._sb_conn.setText("COM: No port detected")
            return False

        port = self.combo_port.currentData()
        if not port:
            port = self.combo_port.currentText().split(" ", 1)[0]
        if port not in port_devices:
            self._update_port_combo(ports)
            self._sb_conn.setText("COM: Selected port disappeared")
            return False

        try:
            baud = int(self.combo_baud.currentText())
            if baud <= 0:
                raise ValueError
        except ValueError:
            self._sb_conn.setText("COM: Invalid baud rate")
            return False

        bytesize_map = {
            "5": serial.FIVEBITS,
            "6": serial.SIXBITS,
            "7": serial.SEVENBITS,
            "8": serial.EIGHTBITS,
        }
        data_key = self.combo_data.currentText()
        if data_key not in bytesize_map:
            self._sb_conn.setText("COM: Invalid data bits")
            return False
        bytesize = bytesize_map[data_key]

        parity_map = {
            "NONE": serial.PARITY_NONE,
            "EVEN": serial.PARITY_EVEN,
            "ODD": serial.PARITY_ODD,
        }
        parity_key = self.combo_parity.currentText()
        if parity_key not in parity_map:
            self._sb_conn.setText("COM: Invalid parity")
            return False
        parity = parity_map[parity_key]

        stop_key = self.combo_stop.currentText()
        if stop_key == "1":
            stopbits = serial.STOPBITS_ONE
        elif stop_key == "1.5":
            stopbits = serial.STOPBITS_ONE_POINT_FIVE
        elif stop_key == "2":
            stopbits = serial.STOPBITS_TWO
        else:
            self._sb_conn.setText("COM: Invalid stop bits")
            return False

        try:
            self.serial_mgr.connect(port, baud, bytesize, parity, stopbits)
        except SerialException as exc:
            msg = str(exc).strip()
            if msg:
                self._sb_conn.setText(f"COM: Open failed: {msg}")
            else:
                self._sb_conn.setText("COM: Open failed")
            return False
        except Exception:
            self._sb_conn.setText("COM: Open failed")
            return False

        self.model.reset()
        self.model.set_time_window(float(self.combo_time.currentText()))
        self.plot.reset_visual()
        self.channel_panel.reset()

        self._start_serial_worker()
        self._set_connected_ui(True)

        parity_short = {"NONE": "N", "EVEN": "E", "ODD": "O"}.get(parity_key, "?")
        self._sb_conn.setText(
            f"COM: Connected {port} ({baud},{data_key}{parity_short}{stop_key})"
        )
        return True

    def _disconnect(self):
        self._stop_serial_worker()
        self.serial_mgr.disconnect()
        self.auto_refresh_timer.stop()
        self.combo_auto.setCurrentText("Off")
        self._ui_update_timer.stop()
        self._pending_lines = []
        self._pending_now = None
        self._pending_dropped = 0

        self._set_connected_ui(False)

    # =========================================================
    # Serial data
    # =========================================================
    def _on_serial_lines(self, lines, now):
        if not lines:
            return
        self._pending_lines.extend(lines)
        if len(self._pending_lines) > MAX_PENDING_LINES:
            overflow = len(self._pending_lines) - MAX_PENDING_LINES
            self._pending_lines = self._pending_lines[overflow:]
            self._pending_dropped += overflow
        self._pending_now = now
        if not self._ui_update_timer.isActive():
            self._ui_update_timer.start(UI_UPDATE_MS)

    def _flush_pending_lines(self):
        if not self._pending_lines:
            return
        lines = self._pending_lines
        now = self._pending_now or time.time()
        self._pending_lines = []
        self._pending_now = None

        for line in lines:
            kv = parse_kv_log(line)
            if kv:
                self.model.update_from_kv(kv, now)

        self.model.prune(now)
        self._sync_channels()

        if not self._snapshot:
            self.plot.update_from_model(self.model, now)

        self.channel_panel.update_values(self.model)

        self._sb_stat.setText(
            f"Samples: {self.model.get_total_samples()} | CH: {self.model.get_enabled_count()}"
        )

        dropped_keys = self.model.consume_dropped_keys()
        if dropped_keys:
            self.statusBar().showMessage(
                f"Channel limit reached (max 16), ignored {dropped_keys} new keys",
                5000
            )

        if self._pending_dropped:
            self.statusBar().showMessage(
                f"Input overrun: dropped {self._pending_dropped} lines", 3000
            )
            self._pending_dropped = 0

        rx_overflow = self.serial_mgr.consume_rx_overflow()
        if rx_overflow:
            self.statusBar().showMessage(
                f"Input overflow: dropped {rx_overflow} bytes without newline",
                3000
            )

    def _on_serial_disconnected(self, message):
        self._force_disconnect_ui(f"COM: {message}")
        self.statusBar().showMessage(
            "COM port removed while connected", 5000
        )

    def _sync_channels(self):
        keys = self.model.get_keys()
        for key in keys:
            color = self.plot.get_channel_color(key)
            self.channel_panel.ensure_checkbox(
                key,
                self.model.is_enabled(key),
                self._on_checkbox_changed,
                color
            )

        self.channel_panel.update_count(len(keys))

        state_map = self.channel_panel.get_checkbox_state_map()
        for k, en in state_map.items():
            self.model.set_enabled(k, en)

    def _on_checkbox_changed(self, _):
        now = time.time()
        self.model.prune(now)

        if not self._snapshot:
            self.plot.update_from_model(self.model, now)

        self.plot.request_temporary_fit(0.5)

    def _on_time_changed(self, text):
        try:
            sec = float(text)
        except ValueError:
            return

        self.model.set_time_window(sec)

        now = time.time()
        self.model.prune(now)

        if not self._snapshot:
            self.plot.update_from_model(self.model, now)

    def _force_disconnect_ui(self, reason_text=None):
        # real disconnect
        self._disconnect()

        # --- CRITICAL ---
        # reset connect button state
        self.btn_connect.blockSignals(True)
        self.btn_connect.setChecked(False)
        self.btn_connect.blockSignals(False)
        self._set_connect_btn_style(False)

        if reason_text:
            self._sb_conn.setText(reason_text)

    def _show_help_dialog(self):
        dlg = HelpDialog(self)
        dlg.exec()


class SerialReadWorker(QObject):
    lines_ready = pyqtSignal(list, float)
    disconnected = pyqtSignal(str)

    def __init__(self, serial_mgr, interval_ms):
        super().__init__()
        self._serial_mgr = serial_mgr
        self._interval_ms = interval_ms
        self._timer = None

    def start(self):
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._timer.start(self._interval_ms)

    @pyqtSlot()
    def stop(self):
        if self._timer:
            self._timer.stop()

    def _poll(self):
        if not self._serial_mgr.is_connected():
            return
        try:
            now = time.time()
            lines = self._serial_mgr.read_lines()
        except SerialException:
            self.disconnected.emit("COM disconnected (device removed)")
            return
        if lines:
            self.lines_ready.emit(lines, now)
