from collections import deque

MAX_CHANNELS = 16


class ChannelModel(object):
    """
    Stores time-series data per key and enabled flags.
    Prunes by time window (seconds).

    Channel order rule:
    - Order is defined by FIRST appearance timestamp in log stream
    - Earlier first-seen channel = higher priority (shown first)
    """

    def __init__(self):
        self._time_window_sec = 5.0

        # key -> deque[(t, v)]
        self._channels = {}

        # key -> bool
        self._enabled = {}

        # display order
        self._key_order = []

        # key -> first seen timestamp
        self._first_seen_ts = {}

        # statistics
        self._total_samples = 0
        self._rx_lines = 0

        # per-channel last timestamp
        self._last_ts = {}

        self._ts_eps = 0.0005  # 0.5ms
        self._dropped_keys = 0

    # =========================================================
    # basic controls
    # =========================================================
    def reset_samples(self):
        """
        Clear time-series data but keep channel order and first-seen info.
        """
        for buf in self._channels.values():
            buf.clear()

        self._last_ts.clear()
        self._total_samples = 0
        self._rx_lines = 0
        self._dropped_keys = 0

    def reset(self):
        """
        Full reset: remove channels and ordering info.
        Used on disconnect.
        """
        self._channels.clear()
        self._enabled.clear()
        self._key_order.clear()
        self._first_seen_ts.clear()
        self._last_ts.clear()
        self._total_samples = 0
        self._rx_lines = 0
        self._dropped_keys = 0

    def set_time_window(self, sec):
        self._time_window_sec = max(float(sec), 1.0)

    def get_time_window(self):
        return self._time_window_sec

    # =========================================================
    # channel management
    # =========================================================
    def ensure_channel(self, key, timestamp):
        """
        Ensure channel exists.
        Record FIRST appearance timestamp.
        """
        if key in self._channels:
            return True

        if len(self._channels) >= MAX_CHANNELS:
            self._dropped_keys += 1
            return False

        self._channels[key] = deque()
        self._enabled[key] = True
        self._last_ts[key] = 0.0

        # record first-seen timestamp
        try:
            ts = float(timestamp)
        except Exception:
            ts = 0.0

        self._first_seen_ts[key] = ts
        self._key_order.append(key)
        return True

    def consume_dropped_keys(self):
        count = self._dropped_keys
        self._dropped_keys = 0
        return count

    def set_enabled(self, key, enabled):
        if key in self._enabled:
            self._enabled[key] = bool(enabled)

    def is_enabled(self, key):
        return bool(self._enabled.get(key, True))

    def get_keys(self):
        return list(self._key_order)

    # =========================================================
    # statistics
    # =========================================================
    def get_total_samples(self):
        return self._total_samples

    def get_enabled_count(self):
        return sum(1 for k in self._enabled if self.is_enabled(k))

    # =========================================================
    # update
    # =========================================================
    def update_from_kv(self, kv, timestamp):
        """
        kv: dict[key]=int
        timestamp: float seconds
        """
        if not kv:
            return

        for key, value in kv.items():
            self.ensure_channel(key, timestamp)

            if key not in self._channels:
                continue

            try:
                v = int(value)
            except Exception:
                continue

            if v < 0:
                continue

            # sanitize timestamp (monotonic per channel)
            try:
                t = float(timestamp)
            except Exception:
                t = self._last_ts.get(key, 0.0) + self._ts_eps

            last = self._last_ts.get(key, 0.0)
            if t <= last:
                t = last + self._ts_eps

            self._last_ts[key] = t
            buf = self._channels[key]

            if buf and abs(t - buf[-1][0]) < self._ts_eps:
                buf[-1] = (t, v)
            else:
                buf.append((t, v))
                self._total_samples += 1

    # =========================================================
    # prune / query
    # =========================================================
    def prune(self, now):
        try:
            cutoff = float(now) - self._time_window_sec
        except Exception:
            return

        for buf in self._channels.values():
            while buf and buf[0][0] < cutoff:
                buf.popleft()

    def get_enabled_keys_with_data(self):
        return [
            k for k in self._key_order
            if self.is_enabled(k) and self._channels.get(k)
        ]

    def get_series(self, key):
        buf = self._channels.get(key)
        return list(buf) if buf else []
