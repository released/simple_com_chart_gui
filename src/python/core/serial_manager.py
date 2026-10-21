import serial
import serial.tools.list_ports
from serial.serialutil import SerialException

MAX_RX_BUFFER = 4096

class SerialManager(object):
    """
    Serial manager for scan/connect/read.
    This module does not depend on any UI framework.
    """

    def __init__(self):
        self._ser = None
        self._rx_buffer = ""   # <-- line assembly buffer
        self._encoding = "utf-8"
        self._rx_overflow = 0

    def scan_ports(self):
        ports = []
        for p in serial.tools.list_ports.comports():
            ports.append((p.device, p.description))
        return ports

    def is_connected(self):
        return (self._ser is not None) and (self._ser.is_open is True)

    def connect(self, port, baudrate, bytesize, parity, stopbits):
        self.disconnect()

        self._rx_buffer = ""

        self._ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=bytesize,
            parity=parity,
            stopbits=stopbits,
            timeout=0   # keep non-blocking
        )

    def disconnect(self):
        if self._ser is not None:
            try:
                if self._ser.is_open:
                    self._ser.close()
            except Exception:
                pass
        self._ser = None
        self._rx_buffer = ""
        self._rx_overflow = 0

    def consume_rx_overflow(self):
        count = self._rx_overflow
        self._rx_overflow = 0
        return count

    def read_lines(self):
        lines = []
        if not self.is_connected():
            return lines

        try:
            data = self._ser.read(self._ser.in_waiting or 1)
            if not data:
                return lines

            chunk = data.decode(self._encoding, errors="replace")
            self._rx_buffer += chunk
            if len(self._rx_buffer) > MAX_RX_BUFFER:
                # Keep only the newest data to avoid unbounded growth.
                self._rx_overflow += len(self._rx_buffer) - MAX_RX_BUFFER
                self._rx_buffer = self._rx_buffer[-MAX_RX_BUFFER:]

            while "\n" in self._rx_buffer:
                line, self._rx_buffer = self._rx_buffer.split("\n", 1)
                line = line.strip("\r")
                if line:
                    lines.append(line)

        except SerialException:
            # COM port removed / invalid
            self.disconnect()
            raise   # 交給 UI 層判斷

        return lines
