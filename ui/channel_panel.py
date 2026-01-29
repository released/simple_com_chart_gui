from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QLabel, QScrollArea,
    QCheckBox, QHBoxLayout, QPushButton, QSizePolicy
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFontMetrics


class ChannelPanel(QWidget):
    """
    Left panel:
    - Detected channel count
    - Select All / None
    - Channel checkboxes with color & value
    """

    VALUE_LABEL_WIDTH = 70   # px, fixed visible width for value

    def __init__(self, parent=None):
        super(ChannelPanel, self).__init__(parent)

        self._checkboxes = {}  # key -> (QCheckBox, QLabel)

        # Allow panel itself to expand horizontally
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        root = QVBoxLayout(self)
        root.setContentsMargins(4, 4, 4, 4)

        # ===== Detected count =====
        self.label_count = QLabel("Detected: 0")
        root.addWidget(self.label_count)

        # ===== Select buttons =====
        btn_row = QHBoxLayout()
        self.btn_all = QPushButton("All")
        self.btn_none = QPushButton("None")

        self.btn_all.setMaximumWidth(60)
        self.btn_none.setMaximumWidth(60)

        btn_row.addWidget(self.btn_all)
        btn_row.addWidget(self.btn_none)
        btn_row.addStretch()

        root.addLayout(btn_row)

        self.btn_all.clicked.connect(self._select_all)
        self.btn_none.clicked.connect(self._select_none)

        # ===== Scroll area =====
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)
        self.scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.scroll_body = QWidget()
        self.scroll_body.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.scroll_layout = QVBoxLayout(self.scroll_body)
        self.scroll_layout.setAlignment(Qt.AlignTop)

        self.scroll.setWidget(self.scroll_body)
        root.addWidget(self.scroll, stretch=1)

    # =========================================================
    # Public API
    # =========================================================
    def reset(self):
        for i in reversed(range(self.scroll_layout.count())):
            w = self.scroll_layout.itemAt(i).widget()
            if w:
                w.setParent(None)
        self._checkboxes.clear()
        self.label_count.setText("Detected: 0")

    def update_count(self, count):
        self.label_count.setText(f"Detected: {int(count)}")

    def ensure_checkbox(self, key, checked, on_changed_cb, color=None):
        """
        Create checkbox row if not exist.
        Checkbox text will be elided if too long.
        """
        if key in self._checkboxes:
            cb, val_label = self._checkboxes[key]
            if color is not None:
                self._apply_color_style(cb, val_label, color)
            return

        row = QWidget()
        row.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        row_layout = QHBoxLayout(row)
        row_layout.setContentsMargins(2, 2, 2, 2)
        row_layout.setSpacing(6)

        # ---- Checkbox (expandable, elided text) ----
        cb = QCheckBox()
        cb.setChecked(bool(checked))
        cb.stateChanged.connect(on_changed_cb)
        cb.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # elide long key text
        metrics = QFontMetrics(cb.font())
        elided = metrics.elidedText(
            key,
            Qt.ElideRight,
            200   # initial max width, auto-adjust with layout
        )
        cb.setText(elided)
        cb.setToolTip(key)   # full text on hover

        # ---- Value label (fixed width, always visible) ----
        value_label = QLabel("--")
        value_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        value_label.setMinimumWidth(self.VALUE_LABEL_WIDTH)
        value_label.setMaximumWidth(self.VALUE_LABEL_WIDTH)
        value_label.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)

        if color is not None:
            self._apply_color_style(cb, value_label, color)

        row_layout.addWidget(cb, stretch=1)
        row_layout.addWidget(value_label, stretch=0)

        self._checkboxes[key] = (cb, value_label)
        self.scroll_layout.addWidget(row)

    def get_checkbox_state_map(self):
        return {
            key: bool(cb.isChecked())
            for key, (cb, _) in self._checkboxes.items()
        }

    def update_values(self, model):
        for key, (_, label) in self._checkboxes.items():
            series = model.get_series(key)
            if not series:
                label.setText("--")
                continue

            _, v = series[-1]
            label.setText(str(v))

    # =========================================================
    # Internal helpers
    # =========================================================
    def _select_all(self):
        for cb, _ in self._checkboxes.values():
            cb.setChecked(True)

    def _select_none(self):
        for cb, _ in self._checkboxes.values():
            cb.setChecked(False)

    @staticmethod
    def _apply_color_style(cb, value_label, color):
        r, g, b = color
        cb.setStyleSheet(
            f"QCheckBox {{ color: rgb({r},{g},{b}); font-weight: bold; }}"
        )
        value_label.setStyleSheet(
            f"color: rgb({r},{g},{b}); font-weight: bold;"
        )
