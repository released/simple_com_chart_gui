# Monitor workflow improvements

Approved 2026-11-05: N1 through N6 from the user-requested terminal comparison.
Native MFC/static CRT only. Preserve all preexisting uncommitted work and private
INI/recordings. No hardware access, third-party source, new runtime or publication.

| Item | Implementation and acceptance |
| --- | --- |
| N1 | Up to 16 persistent literal keyword rules: color, word/line scope, case flag and text. Explicit Apply; invalid rules/save failure retain active rules. Display only, with bounded spans and changed-tail styling. |
| N2 | Explicit Following/Browsing/Paused status, received-byte backlog and Return to latest. Optional smart scroll defaults off; user scrolling up suspends following and reaching bottom resumes only that smart suspension. Raw capture/recording continue. |
| N3 | Immutable retained-byte inspector with editable absolute offset/count, HEX/ASCII and named control bytes. At most 4096 inspected bytes; no export/TX/source edits. Terminal content is not treated as an exact source selection. |
| N4 | Optional upper chart/table and lower Messages view, adjustable split ratio. Default off; existing tabs, focus, snapshots, exports and independent data flow preserved. Guard minimum pane sizes and enlarged-font layout. |
| N5 | Bounded private recent recording/profile/view/session list with display labels, rename/remove and Open folder. Stage/open existing tools only; never apply preferences, connect, play or transmit automatically. Existing file formats unchanged. |
| N6 | Bounded session-only event list for A/B recording, connection and TX failures. Coalesce repeats; optional taskbar flashing defaults off. No raw-log copies, trigger recording or automatic hardware actions. |

Implementation batches: message models/UI (N1-N3); shared main layout (N4);
recent/event center and hooks (N5-N6); catalogs and both guides; focused core,
actual native UI/layout and sustained simulated RX verification; formal delivery
and safe owned-artifact retirement. Increment version only once for this batch.

Verification uses fake/injected sources and isolated project-local INIs. Select
message_view, new monitor core tests, uart_receive/ui_language, new Monitor runtime
scope, affected chart/layout/profile callers and timed RX soak. Include Debug MFC
layout/lifetime verification; avoid unrelated protocol/Python/All suites. Record
actual results in existing HANDOFF/SELF_CHECK/latest-validation, then retire
registered fixtures, screenshots, test INIs and rebuildable EXEs/obj files.

Measured receive-load correction: repeated per-span RichEdit formatting caused
an 11,344 ms UI-pump gap with full retained history and two keyword rules. Keep
the same receive/retention thresholds. Generate escaped, bounded RTF from the
existing color layers and import once per changed tail (or full retained display
after eviction), preserving raw bytes, Unicode, exact line endings, owned font
size and Find priority. Verify real controls and repeat the identical quiet soak.
