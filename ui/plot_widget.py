import pyqtgraph as pg
from pyqtgraph import mkPen
from pyqtgraph.Qt import QtCore
from PyQt5.QtWidgets import QGraphicsRectItem
from PyQt5.QtGui import QBrush, QPen
import time


COLOR_TABLE = [
    (255,  99,  71),   # red
    ( 30, 144, 255),   # blue
    ( 50, 205,  50),   # green
    (255,  20, 147),   # deep pink
    (138,  43, 226),   # purple
    (255, 140,   0),   # dark orange
    (  0, 206, 209),   # cyan
    (220,  20,  60),   # crimson
]


class MonitorPlotWidget(pg.PlotWidget):
    """
    Plot policy:
    - STEP plot (ZOH)
    - End-value overlay:
        x fixed at right edge (view right, with margin)
        y follows last data point (plus a small pixel offset upward)
        left-stacked when overlapping (pixel-space collision)

    Auto Y policy (IMPORTANT):
    - Only expand upward (never expand downward, never shrink automatically).
    - Enforce minimum visible Y span to mitigate pixel quantization for small-range signals.
    - Overlay rendering MUST NOT drive Y range changes (avoid toggle overlay causing scale jump).
    """

    END_TAG_GAP_PX = 10
    END_TAG_Y_THRESHOLD_PX = 14
    END_TAG_X_MARGIN_PX = 8
    END_TAG_Y_OFFSET_PX = 10      # move overlay above the point
    END_TAG_SAFE_MARGIN_PX = 8    # keep a safe margin inside plot

    AUTO_EXPAND_PAD_PX = 12       # when expanding y-range for visibility

    # Minimum visible Y span protection (Solution A)
    # Policy: expand upward only.
    MIN_VISIBLE_Y_SPAN = 50.0  # tune: 50/80/100; 80 works well for 0~21 style signals

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        self._model = None
        self._curves = {}
        self._end_tags = {}   # key -> {text, bg}
        self._time_window = 5.0
        self._frozen = False
        self._fit_until_ts = 0.0

        self._overlay_enabled = True

        self.showGrid(x=True, y=True)
        self.setLabel("bottom", "Time (s)")
        self.setLabel("left", "Value")

        self._plot_item = self.getPlotItem()
        self._vb = self._plot_item.getViewBox()
        self._vb.enableAutoRange(x=False, y=False)

        # ---------- cursor ----------
        self._vline = pg.InfiniteLine(
            angle=90,
            movable=False,
            pen=pg.mkPen((180, 180, 180), width=1, style=QtCore.Qt.DashLine)
        )
        self._vline.setZValue(900)
        self._plot_item.addItem(self._vline)
        self._vline.hide()

        # ---------- hover ----------
        self._hover_text = pg.TextItem(anchor=(0, 1))
        self._hover_text.setZValue(1000)
        self._plot_item.addItem(self._hover_text)
        self._hover_text.hide()

        self._plot_item.scene().sigMouseMoved.connect(self._on_mouse_moved)

        self.setXRange(0, self._time_window, padding=0)
        self._apply_mouse_policy()

    # =========================================================
    # Public API
    # =========================================================
    def set_overlay_enabled(self, enabled: bool):
        self._overlay_enabled = bool(enabled)
        if not self._overlay_enabled:
            self._hide_all_end_tags()

    def refresh_overlay_only(self):
        if self._model is None:
            return
        # IMPORTANT: overlay refresh must NOT change y-range
        self._update_end_value_tags(self._model)

    def reset_visual(self):
        for info in self._curves.values():
            info["curve"].setData([], [])

        self._hide_all_end_tags()

        self._vline.hide()
        self._hover_text.hide()

    def get_channel_color(self, key):
        if key in self._curves:
            return self._curves[key]["color"]
        return (200, 200, 200)

    # =========================================================
    # Mode control
    # =========================================================
    def set_frozen(self, frozen: bool):
        self._frozen = bool(frozen)
        self._apply_mouse_policy()

        if not self._frozen:
            self._vline.hide()
            self._hover_text.hide()

    def _apply_mouse_policy(self):
        self._vb.setMouseEnabled(x=self._frozen, y=self._frozen)

    # =========================================================
    # Curve management
    # =========================================================
    def ensure_curve(self, key):
        if key not in self._curves:
            idx = len(self._curves) % len(COLOR_TABLE)
            color = COLOR_TABLE[idx]
            pen = mkPen(color=color, width=5)
            pen.setCosmetic(True)  # constant pixel width

            curve = pg.PlotDataItem(pen=pen, stepMode="left")
            self._plot_item.addItem(curve)

            self._curves[key] = {
                "curve": curve,
                "color": color
            }

    def _ensure_end_tag(self, key, color):
        if key in self._end_tags:
            return

        # anchor right-middle
        text = pg.TextItem(anchor=(1, 0.5))
        text.setZValue(1100)
        text.setColor(color)
        text.setFont(pg.QtGui.QFont("", 10, pg.QtGui.QFont.Bold))

        bg = QGraphicsRectItem()
        bg.setZValue(1090)
        bg.setPen(QPen(QtCore.Qt.NoPen))
        bg.setBrush(QBrush(QtCore.Qt.black))
        bg.setOpacity(0.1)

        self._plot_item.addItem(bg)
        self._plot_item.addItem(text)

        self._end_tags[key] = {"text": text, "bg": bg}

    def _hide_all_end_tags(self):
        for tag in self._end_tags.values():
            tag["text"].hide()
            tag["bg"].hide()

    # =========================================================
    # Y-range helpers (UP-ONLY)
    # =========================================================
    def _enforce_min_visible_span_up_only(self, y_min, y_max):
        if y_min is None or y_max is None:
            return y_min, y_max

        span = float(y_max) - float(y_min)
        if span < float(self.MIN_VISIBLE_Y_SPAN):
            y_max = float(y_min) + float(self.MIN_VISIBLE_Y_SPAN)
        return y_min, y_max

    def _expand_y_up_only_to_include(self, required_y_max):
        """
        Only expand the current Y range upward (never downward, never shrink).
        Also enforces MIN_VISIBLE_Y_SPAN.
        """
        vb = self._vb
        vr = vb.viewRect()

        cur_min = min(vr.top(), vr.bottom())
        cur_max = max(vr.top(), vr.bottom())

        if required_y_max is None:
            return

        target_min = cur_min
        target_max = cur_max

        if float(required_y_max) > float(cur_max):
            target_max = float(required_y_max)

        # enforce minimum span up-only
        target_min, target_max = self._enforce_min_visible_span_up_only(target_min, target_max)

        # keep min not below 0 for typical ADC, but do not enforce if already negative
        if target_min < 0 and cur_min >= 0:
            target_min = 0
            target_min, target_max = self._enforce_min_visible_span_up_only(target_min, target_max)

        if target_min != cur_min or target_max != cur_max:
            self.setYRange(target_min, target_max, padding=0)

    def _compute_overlay_required_y_max(self, model):
        """
        Compute a conservative required Y max so overlays won't be clipped.
        IMPORTANT: This must NOT change y-range directly; caller handles expansion.
        """
        if model is None:
            return None
        if not self._overlay_enabled:
            return None

        enabled = model.get_enabled_keys_with_data()
        if not enabled:
            return None

        vb = self._vb
        px = vb.viewPixelSize()
        if px is None:
            return None

        y_per_px = float(px[1])
        dy = float(self.END_TAG_Y_OFFSET_PX) * y_per_px
        dy += float(self.AUTO_EXPAND_PAD_PX) * y_per_px  # safety

        req = None
        for key in model.get_keys():
            if key not in enabled:
                continue
            series = model.get_series(key)
            if not series:
                continue
            v = float(series[-1][1])
            y = v + dy
            if req is None or y > req:
                req = y
        return req

    # =========================================================
    # Update from model
    # =========================================================
    def update_from_model(self, model, now):
        self._model = model
        self._time_window = model.get_time_window()

        if self._frozen:
            return

        self.setXRange(0, self._time_window, padding=0)

        enabled_keys = model.get_enabled_keys_with_data()

        for key in model.get_keys():
            self.ensure_curve(key)

        data_min = None
        data_max = None

        for key, info in self._curves.items():
            curve = info["curve"]

            if key not in enabled_keys:
                curve.setData([], [])
                continue

            series = model.get_series(key)
            if not series:
                curve.setData([], [])
                continue

            t_end = series[-1][0]
            t_start = t_end - self._time_window

            xs, ys = [], []
            for t, v in series:
                if t >= t_start:
                    xs.append(t - t_start)
                    ys.append(v)

            if ys:
                local_min = min(ys)
                local_max = max(ys)
                if data_min is None or local_min < data_min:
                    data_min = local_min
                if data_max is None or local_max > data_max:
                    data_max = local_max

            curve.setData(xs, ys)

        # overlay update (MUST NOT change y-range)
        self._update_end_value_tags(model)

        # ---- Y-range policy (UP ONLY) ----
        # 1) ensure minimum visible span is satisfied even when data range is tiny
        # 2) expand upward to include overlay if needed
        if data_min is not None and data_max is not None:
            # We never expand downward automatically, so we only use data_max here.
            # Enforce min span using current bottom as baseline.
            vb = self._vb
            vr = vb.viewRect()
            cur_min = min(vr.top(), vr.bottom())

            # baseline required max from data: at least max(data_max + pad)
            px = vb.viewPixelSize()
            if px is None:
                y_per_px = 0.0
            else:
                y_per_px = float(px[1])

            data_required_max = float(data_max) + (float(self.AUTO_EXPAND_PAD_PX) * y_per_px)

            # also consider overlay required max (computed from current scale, conservative)
            overlay_required_max = self._compute_overlay_required_y_max(model)
            required_max = data_required_max
            if overlay_required_max is not None and float(overlay_required_max) > required_max:
                required_max = float(overlay_required_max)

            # enforce min span from current bottom (up-only)
            # e.g., if cur_min is 0, required_max becomes at least 50.
            minspan_required_max = float(cur_min) + float(self.MIN_VISIBLE_Y_SPAN)
            if required_max < minspan_required_max:
                required_max = minspan_required_max

            self._expand_y_up_only_to_include(required_max)

        # temporary fit window (explicit)
        if self._fit_until_ts > now:
            self.fit_enabled_channels()

    # =========================================================
    # End-value overlay (NO Y-RANGE SIDE EFFECTS)
    # =========================================================
    def _update_end_value_tags(self, model):
        if model is None or not self._overlay_enabled:
            self._hide_all_end_tags()
            return

        vb = self._vb
        vr = vb.viewRect()

        px = vb.viewPixelSize()
        if px is None:
            x_per_px = 0.0
            y_per_px = 0.0
        else:
            x_per_px = float(px[0])
            y_per_px = float(px[1])

        # x anchor: right edge with margin (DATA SPACE)
        base_x = vr.right() - (self.END_TAG_X_MARGIN_PX * x_per_px)

        placed = []  # [(scene_y, left_x_data)]

        self._hide_all_end_tags()

        for key in model.get_keys():
            if key not in model.get_enabled_keys_with_data():
                continue

            series = model.get_series(key)
            if not series:
                continue

            value = series[-1][1]
            color = self._curves[key]["color"]
            self._ensure_end_tag(key, color)

            tag = self._end_tags[key]
            text = tag["text"]
            bg = tag["bg"]

            # text content: value only
            text.setText(f"{value}")
            br = text.boundingRect()

            # initial position (DATA SPACE)
            x = base_x
            y = value + (self.END_TAG_Y_OFFSET_PX * y_per_px)

            # -------- clamp Y to view (UI-level handling, NO Y-RANGE CHANGE) --------
            top_limit = vr.top() + (self.END_TAG_SAFE_MARGIN_PX * y_per_px)
            bot_limit = vr.bottom() - (self.END_TAG_SAFE_MARGIN_PX * y_per_px)

            if y > bot_limit:
                y = bot_limit
            elif y < top_limit:
                y = top_limit

            # -------- collision detection in PIXEL SPACE --------
            scene_pt = vb.mapViewToScene(QtCore.QPointF(x, y))
            scene_y = float(scene_pt.y())

            for prev_scene_y, prev_left_x in placed:
                if abs(prev_scene_y - scene_y) < self.END_TAG_Y_THRESHOLD_PX:
                    x = prev_left_x - (self.END_TAG_GAP_PX * x_per_px)

            # -------- clamp X so overlay stays inside view --------
            left_x_data = x - (br.width() * x_per_px)

            if left_x_data < vr.left():
                x = vr.left() + (br.width() * x_per_px) + (self.END_TAG_SAFE_MARGIN_PX * x_per_px)
                left_x_data = x - (br.width() * x_per_px)

            if x > vr.right():
                x = vr.right() - (self.END_TAG_SAFE_MARGIN_PX * x_per_px)

            # -------- apply position --------
            text.setPos(x, y)
            bg.setRect(br.adjusted(-6, -3, 6, 3))
            bg.setPos(x, y)

            text.show()
            bg.show()

            placed.append((scene_y, left_x_data))


    # =========================================================
    # Fit
    # =========================================================
    def request_temporary_fit(self, duration_sec=0.5):
        self._fit_until_ts = time.time() + float(duration_sec)

    def fit_enabled_channels(self):
        if self._model is None:
            return

        vals = []
        for key in self._model.get_enabled_keys_with_data():
            series = self._model.get_series(key)
            if series:
                vals.extend(v for _, v in series)

        if not vals:
            return

        y_min = min(vals)
        y_max = max(vals)

        px = self._vb.viewPixelSize()
        if px is None:
            pad = max((y_max - y_min) * 0.05, 1.0)
        else:
            pad = max(self.AUTO_EXPAND_PAD_PX * float(px[1]), 1.0)

        target_min = y_min - pad
        target_max = y_max + pad

        if target_min < 0 and y_min >= 0:
            target_min = 0

        # Still keep minimum span (expand upward only reminder)
        target_min, target_max = self._enforce_min_visible_span_up_only(target_min, target_max)

        self.setYRange(target_min, target_max, padding=0)

    # =========================================================
    # Mouse hover (unchanged)
    # =========================================================
    def _on_mouse_moved(self, pos):
        if self._model is None:
            return

        if not self._plot_item.sceneBoundingRect().contains(pos):
            self._vline.hide()
            self._hover_text.hide()
            return

        vb = self._vb
        mouse_point = vb.mapSceneToView(pos)
        t_view = mouse_point.x()

        if t_view < 0 or t_view > self._time_window:
            self._vline.hide()
            self._hover_text.hide()
            return

        lines = []
        snap_t = None

        for key in self._model.get_enabled_keys_with_data():
            series = self._model.get_series(key)
            if not series:
                continue

            t_end = series[-1][0]
            t_start = t_end - self._time_window

            windowed = [(tt, v) for (tt, v) in series if tt >= t_start]
            if not windowed:
                continue

            xs = [tt - t_start for (tt, _) in windowed]
            idx = min(range(len(xs)), key=lambda i: abs(xs[i] - t_view))

            real_t = xs[idx]
            value = windowed[idx][1]
            snap_t = real_t if snap_t is None else snap_t

            color = self._curves[key]["color"]
            lines.append(
                f'<span style="color: rgb{color};">{key}: {value}</span>'
            )

        if not lines:
            self._vline.hide()
            self._hover_text.hide()
            return

        self._vline.setPos(snap_t)
        self._vline.show()

        html = f"""
        <div style="
            background-color: rgba(0,0,0,180);
            padding: 6px;
            border-radius: 4px;
        ">
            <b>t = {snap_t:.3f} s</b><br>
            {"<br>".join(lines)}
        </div>
        """

        self._hover_text.setHtml(html)
        vr = vb.viewRect()
        self._hover_text.setPos(vr.left(), vr.top())
        self._hover_text.show()
