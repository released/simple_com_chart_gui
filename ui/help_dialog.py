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
        text.setText(
            "Supported Log Format\n"
            "====================\n\n"
            "Each UART log must be exactly ONE line and must end with CRLF (\\r\\n).\n\n"
            "General Format:\n"
            "  key:value,key:value,key:value,...\\r\\n\n\n"
            "Example (single line):\n"
            "  state:5,CHG:4179mv,T1:2296mv,T2:1589mv,Q6:2111mv,Q2/Q3:21mv\\r\\n\n\n"
            "Rules:\n"
            "- Fields are separated by comma ','\n"
            "- Key and value are separated by colon ':'\n"
            "- Spaces are ignored\n"
            "- Field order does not matter\n"
            "- Unknown keys are ignored\n"
            "- One log line represents one sample\n"
            "- Line termination must be CRLF (\\r\\n)\n\n"
            "MCU Firmware Example (C):\n"
            "  printf(\"state:%d,CHG:%dmv,T1:%dmv,T2:%dmv,Q6:%dmv,Q2/Q3:%dmv\\r\\n\", "
            "state, chg_mv, t1_mv, t2_mv, q6_mv, q23_mv);\n\n"
            "Notes:\n"
            "- Timestamp is generated on the PC side when data is received\n"
            "- This tool does not control MCU output timing or content\n"
            "- Any change in log format on MCU side must be reflected in the parser\n"
        )
        layout.addWidget(text, 1)

        # ---------- Close button ----------
        btn = QPushButton("Close")
        btn.clicked.connect(self.accept)
        layout.addWidget(btn, alignment=Qt.AlignRight)
