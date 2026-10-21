from PyQt5.QtWidgets import (
    QDialog, QVBoxLayout, QLabel, QTextEdit, QPushButton
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont


class HelpDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setWindowTitle("How to Use / MCU Log Format")
        self.resize(1024, 800)

        layout = QVBoxLayout(self)

        # ---------- Title ----------
        title = QLabel("MCU Log Output Requirement")
        title_font = QFont()
        title_font.setPointSize(13)
        title_font.setBold(True)
        title.setFont(title_font)
        layout.addWidget(title)

        # ---------- Description ----------
        desc = QLabel(
            "This tool visualizes real-time MCU UART log output.\n"
            "MCU firmware must output log messages in the format specified below "
            "for the tool to correctly decode, plot, and display channel values."
        )
        desc.setWordWrap(True)
        layout.addWidget(desc)

        # ---------- Main text ----------
        text = QTextEdit()
        text.setReadOnly(True)
        text.setFont(QFont("Consolas", 11))
        text.setPlainText(
            "Text Telemetry / Waveforms\n"
            "=========================\n"
            "\n"
            "Connect to a Windows COM port, then select the channels to draw.\n"
            "Channels are discovered from incoming data; no firmware-specific names are required.\n"
            "\n"
            "Input: one text line per sample, ending in LF or CRLF.\n"
            "Fields: comma separated key:value or key=value.\n"
            "Example:\n"
            "fan,duty:920,fg_hz:69.1,offset:-2.5,err:11%,cmd:0x3202,cap:105/76/29\n"
            "\n"
            "Numeric values: signed integers, decimals, scientific notation, 0x hex.\n"
            "ASCII unit suffixes (mv, Hz) are accepted; units are not rescaled.\n"
            "11% means 11. cap:105/76/29 creates cap[0], cap[1], cap[2].\n"
            "Text-only tokens such as fan and malformed values are skipped.\n"
            "Keys may contain spaces or symbols; max 128 UTF-8 bytes.\n"
            "\n"
            "Include / Exclude:\n"
            "- Text: case-sensitive substring search over the entire raw line.\n"
            "- Regex: search using a common ECMAScript-style expression.\n"
            "- Include must match (empty = all); Exclude takes priority (empty = none).\n"
            "- Example Include ^(fan,|ac_adc_mv:) selects both line types in Regex mode.\n"
            "- Apply starts a new capture and discovers channels again.\n"
            "- Invalid expressions keep the previous applied filter active.\n"
            "\n"
            "Max CH: 1..1024; default 256. Limits are applied on Apply.\n"
            "Use None, then select a few signals with similar units and press Fit.\n"
            "Fit rescales Y; signed values and decimals remain visible.\n"
            "Snapshot freezes the plot; Live resumes. Overlay toggles end values.\n"
            "Refresh clears samples while retaining channels. Changing filters clears both.\n"
            "\n"
            "Native MFC settings are saved in SimpleComChart.ini beside the EXE.\n"
            "Timestamps use PC receipt time, not device timestamps.\n"
            "Launch with --demo to view simulated telemetry without a serial device.\n"
            "USB-UART hardware may require its vendor driver.\n"
        )
        layout.addWidget(text, 1)

        # ---------- Close button ----------
        btn = QPushButton("Close")
        btn.clicked.connect(self.accept)
        layout.addWidget(btn, alignment=Qt.AlignRight)
