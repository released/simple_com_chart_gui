# UART analysis expansion

Approved 2026-10-25 following the ten-item competitive analysis. Native MFC only;
preserve all existing uncommitted work, private settings and raw recordings.
No Git publication or hardware operation is implied. No new runtime dependencies.

## Delivery sequence and acceptance

1. [x] Peak-preserving rendering: ordered first/min/max/last per pixel bucket;
   source samples, exports, FFT and cursor values stay unchanged. Test narrow
   positive/negative spikes and dense/constant input; compare rendering cost.
2. [x] Port B text charts: independent filter/extractor/generation/receipt clocks;
   source-qualified numeric keys, default unchecked, separate raw log windows;
   preserve A keys/settings and exclude binary/protocol-owned traffic.
3. [x] Offline binary decoding: manual schema selection, existing indexed replay
   timing, reset partial frames on seek/discontinuity, observable CRC errors;
   no live port/TX and no change to source recordings.
4. [x] Complete workspace: versioned allowlisted A/B, parsers, pacing, framing,
   instruments and analysis settings; stage/preview/explicit idle Apply; preserve
   unrelated private INI entries, exclude commands/paths/arming; legacy profiles
   remain readable. Atomic write and invalid/cancel preservation.
5. [x] Derived channels: bounded independent expression parser, arithmetic and
   EMA, named computed fields; same-frame inputs, no stale/zero fabrication,
   finite results/cycle limits; raw fields remain unchanged and identifiable.
6. [x] Binary inspector: read-only selected bytes, field spans, endian, bitfield/
   enum projection and calculated/received checksum diagnostics; bounded work.
7. [x] Numeric script assertions/report: explicit Start, selected current-step
   numeric range, finite timeout and structured CSV results; preserve old steps,
   TX lease/generation/cancel/no automatic retry or reconnect resume.
8. [x] Recorded-run comparison: bounded background read-only imports, manual
   alignment, matched keys, difference/tolerance summary and plotted overlay;
   no silent interpolation or file mutation.
9. [x] XY plot: selectable X/Y and independent axes, same-frame pairs only,
   bounded display history, no invented pairings; default none.
10. [x] Opt-in A/B forwarding: explicit direction/start/stop, two TX leases,
    bounded queues, fail/partial/disconnect/gap stop, never retry or auto-resume;
    preserve two independent logs and raw recording.

## Verification

- Implement in small modules. Add focused core tests for new algorithms/state and
  isolated native control integration for user entry points/settings/errors.
- Select affected chart, workbench, replay/profile and serial-operation regressions
  only; no unrelated Python/full historical suite or clean audit by default.
- MFC control lifecycle/layout uses Debug/Release and affected enlarged-font views.
- Final delivery: Release x64, static MFC/CRT, version/resource consistency,
  actual retail startup/close, system dependencies/map/size evidence, personal INI
  hash preserved. No performance-neutral claim without comparable measurements.
- Physical UART/board, wire timing, actual DPI switching and manual dwell are
  separately unverified; isolated fake transports do not establish those results.
- Keep product guidance in a guide/README; implementation state/evidence here and
  in HANDOFF/SELF_CHECK. Update checkboxes only after actual verification.

## Acceptance record (2026-10-26)

All ten items passed the scoped core/native/fake-transport acceptance described
above. Formal Release1.0.1.027 is delivered; source/INI preservation, measured
rendering/receive performance, safe final clean and test-data retention receipts
are recorded in HANDOFF/SELF_CHECK. Physical UART/DPI/mouse remain unverified.
No Git commit/push was performed.
