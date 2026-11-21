"""Bounded cross-thread receive storage; queued notifications carry no payload."""
from collections import deque
from threading import Lock


class ReceiveMailbox:
    def __init__(self, max_rows=2000, max_bytes=2 * 1024 * 1024):
        self._lock = Lock()
        self._rows = deque()
        self._bytes = 0
        self._max_rows = max_rows
        self._max_bytes = max_bytes
        self._notified = False
        self._lost_rows = 0
        self._lost_bytes = 0
        self._oversized_bytes = 0

    def push(self, lines, receipt, oversized=0):
        with self._lock:
            self._oversized_bytes += oversized
            for line in lines:
                size = len(line.encode("utf-8"))
                if size > self._max_bytes:
                    self._lost_rows += 1
                    self._lost_bytes += size
                    continue
                while self._rows and (len(self._rows) >= self._max_rows or
                                      self._bytes + size > self._max_bytes):
                    _, _, removed = self._rows.popleft()
                    self._bytes -= removed
                    self._lost_rows += 1
                    self._lost_bytes += removed
                self._rows.append((line, receipt, size))
                self._bytes += size
            wake = not self._notified and bool(self._rows or self._lost_rows or self._oversized_bytes)
            self._notified |= wake
            return wake

    def take(self, limit=64):
        with self._lock:
            rows = []
            while self._rows and len(rows) < limit:
                line, receipt, size = self._rows.popleft()
                self._bytes -= size
                rows.append((line, receipt))
            loss = (self._lost_rows, self._lost_bytes, self._oversized_bytes)
            self._lost_rows = self._lost_bytes = self._oversized_bytes = 0
            pending = bool(self._rows)
            if not pending:
                self._notified = False
            return rows, loss, pending

    @property
    def size(self):
        with self._lock:
            return len(self._rows), self._bytes
