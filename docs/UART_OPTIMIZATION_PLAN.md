# Native UART optimization plan

User approval: 2026-10-24. This extends, and does not replace, the completed
`UART_DEBUG_PLAN.md` A/B1/B2/C delivery. Python and Git publication are out of scope.

## Acceptance and sequence

1. Startup/display: session-wide single instance; restore the existing owned
   modal window; per-monitor DPI fonts/layout; usable small-screen advanced
   settings with scrolling. Keep exact-target build closure authorization.
2. Receive safety: bounded regex matching, persistent LIVE/OFFLINE/REC/AUTO and
   timestamp/loss state, repeatable sustained receive/recording measurements.
3. Generic extraction: key/value compatibility, CSV/Tab, flat JSON numeric
   fields and capture-group extraction; preview and portable settings.
4. Channel preferences: search/select results, aliases/units/precision and
   scale/offset for display only; searchable analysis selector.
5. Chart navigation: zoom/pan, draggable A/B, optional fixed Y, return to latest;
   retained table/message/chart correlation with explicit expired-history state.
6. Large recordings: bounded streaming/paged replay, ordered rotation segments,
   cross-segment search/seek. Never load an unbounded file into memory.
7. Optional light terminal: CR/BS/erase-line interpretation, explicit keyboard TX
   and local echo, safe paste confirmation; raw monitor/recording remain RX-only.
8. File TX: explicit text/binary chunking, pacing, progress, pause and cancel;
   generation-bound, no automatic replay/retry or firmware-specific flashing.

## Implementation constraints

- Preserve unrelated edits and the personal production INI. Fixtures use
  `build/verify`, synthetic data and fake transports, not physical COM devices.
- No external dependencies; static MFC/CRT delivery remains portable.
- Prefer separate core modules plus thin native UI integration.
- Raw bytes, capture/export values and actual target effects must not be silently
  changed by display preferences, terminal rendering or navigation.
- Select regression tests by changed paths. Record measured results, not assumed
  passes. Enlarged-font checks are not physical monitor-DPI acceptance.
- Do not mark this plan complete while implementation or required safe
  verification remains. Real UART/USB/flow-control acceptance is separately
  reported when hardware evidence is unavailable.

## Current status

Completed software delivery: native **1.0.1.022**, 2026-10-24. Baseline A/B1/B2/C
was preserved; all eight optimization groups above are implemented and safely
verified. No accepted software feature remains pending.

- [x] Startup/display: stable per-session guard, bounded readiness/activation,
  PMv2 manifest/context, DPI replacement fonts, measured/wrapping controls,
  clipped scroll viewport and narrow-screen 2x2 charts.
- [x] Receive safety/performance: bounded NFA, visible source/provenance/loss,
  non-regex scalar parsing, bounded UI batches/pending input, O(1) latest values
  and deferred hidden-page refresh. Sustained Debug 60 s and Release 300 s pass.
- [x] Generic extraction: CSV/Tab/flat JSON/Regex groups, preview/apply, time
  validation, portable INI/profile schema extensions, compatible key/value.
- [x] Channel search/preferences: stable hidden checkboxes, aliases/units/precision/
  scale/offset at display only, searchable analysis/preferences selectors.
- [x] Chart navigation/correlation: zoom/pan/A-B dragging/fixed Y/latest return,
  original snapshot statistics, exact RX offset/count/sequence mapping and honest
  expired-history notices. Finite extreme axes have bounded tick rendering.
- [x] Large replay: background source reads/search/time seek, disk-paged index,
  numeric rotation order, strict segment/index checks and epoch cancellation.
- [x] Light terminal/keyboard: bounded CR/BS/erase-line view, explicit generation-
  bound arming, cancelable paste preview, separate local TX echo and no RX changes.
- [x] File TX: validated text/exact binary, chunk/pacing/progress/pause/cancel,
  partial-failure stop and no reconnect replay; no hardware used by fixtures.

Final Debug/Release builds pass 6/6 native targets each. Targeted UI scopes and
125%/150% enlarged-font evidence pass; actual PMv2 window context is asserted.
Release 300 s: 30,224,384 produced/RX/recorded bytes, 29 ordered segments with
matching byte-stream hash, zero software byte/line loss, peak private bytes
15,679,488 and maximum UI-pump gap 63 ms. These are local simulated results, not
physical UART throughput or overnight stability claims. Exact commands, reused
evidence after the final time-field-only guard, failures/fixes and hashes are in
`../SELF_CHECK.md` and `../HANDOFF.md`.

NOT RUN boundaries: physical UART/USB/pins/flow/Break, actual monitor DPI changes,
physical cursor/clipboard/file-dialog acceptance, foreground/elevation/session
restrictions, overnight soak and stalled remote-file/driver cancellation. Older
pre-guard binaries cannot be retroactively made cooperative. Personal INI was
unchanged; no cleanup/purge, commit or GitHub push.
