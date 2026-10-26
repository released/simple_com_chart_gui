# UART session and workflow expansion

Approved 2026-10-26: implement all eight recommendations with the existing native
C++/MFC architecture. No third-party source, interpreter, database, dependency
installation, hardware test, Git commit or publication is implied.

## Baseline and preservation

- Entry source/delivery version: 1.0.1.028. Preserve the existing uncommitted
  Laboratory changes; do not restart or replace them. GUI 2,120,704 bytes,
  SHA256 64DD6C2732DBA0E6313BBB2F71CA5452EFB046B3D10EA5EBBEC7BF5B3032EF26.
- Keep personal settings unchanged; entry INI SHA256
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- Normal completed folder target is 20,000,000 bytes including Git. Existing
  explicit Git-retention decision is 45,000,000 bytes; entry about 37.59 MB,
  Git 26,473,677 bytes. Reuse that decision without asking about temporary growth.
- Use one owned temporary source/delivery recovery baseline, retired after final
  verification. Generated test data stays in fresh project-local owned runs and
  is cleaned on pass/fail/cancel. Final safe clean plus exact generated cache/test
  binary retirement; preserve user recordings, formal EXE/INI and Git history.

## Delivery sequence and acceptance

1. [x] Recording limits: total actual output bytes, elapsed time and available
   disk threshold. Stop recording visibly at the limit, preserve written data,
   keep COM monitoring active; never delete old recordings. Reuse writer/rotation.
2. [x] Session manifest: versioned, bounded relative file references, safe parser/
   workspace snapshot, tool version, file sizes and SHA256. Background creation/
   verification, no raw duplication, no private command/history/automatic TX.
   Completed recordings only; explicit preview/stage before offline settings use.
3. [x] Device information: read-only SetupAPI identity/hardware/manufacturer/
   location/driver information, search and explicit copy. No port open, install,
   guessed serial number or change to existing identity/reconnect decisions.
4. [x] Numeric input framing: compatible LF default plus CR/CRLF/custom delimiter
   and PC-receipt idle gap, independent A/B/live/replay configuration. Preserve
   offsets/counts and generation resets; never label receipt gaps as wire timing.
5. [x] Custom checksum: bounded CRC8/16/32 parameters, input range/endianness and
   preview; shared validated core for packet TX and live/offline binary RX.
   Existing definitions/configs keep their meaning; preview never transmits.
6. [x] Nested JSON numeric paths: explicit JSON Pointer selections and bounded
   arrays/depth/work. Keep existing flat mode and finite numeric/missing-is-blank
   semantics; support cases/preview/persistence without a script runtime.
7. [x] Time calculus: offline retained-capture derivative/integral with explicit
   source/key/clock/gap rules, inspectable values/export and plotted results.
   No invented zero, cross-source pair or reconnect bridge; integration assumes
   a documented piecewise-linear signal between adjacent valid samples.
8. [x] Passive transactions: bounded request/reply ID/rule matching over existing
   per-port traffic, source/generation isolation, ambiguity/timeouts/expiry visible,
   exact byte locations and PC response intervals. Observe only; no new TX/retry.

## Integration and verification

- Keep everyday Messages/Chart/4 Charts unchanged. Add measured/scrollable native
  Laboratory pages and existing options forms; keep UI/control logic and cores
  split into focused source files. Preserve static CRT/MFC and Release size flags.
- Reuse existing background-job ownership, profile staging/idle Apply, TX leases,
  replay storage, parser/traffic clocks, immutable snapshots and atomic helpers.
- Select new/affected native cores, Debug enlarged-font and Release new-page
  integration, affected receive/recording/profile/parser paths, final formal
  delivery/CLI identity and separately isolated receive performance evidence.
  Do not rerun All/Python/unaffected historical suites.
- Test malformed/bounded inputs, cancellation/no-overwrite, partial writes,
  generation/offset preservation, missing devices/fields and existing formats.
- Record actual evidence, EXE footprint/dependencies, cleanup and final folder
  size in SELF_CHECK/HANDOFF and compact validation JSON. Hardware UART/driver,
  monitor-DPI/manual interaction and wire timing remain unverified unless tested.

## Verification outcome

All eight native features are implemented in 1.0.1.029. Selected cores, Debug
minimum-size/enlarged-font and Release Session controls, affected existing UART/
analysis/Laboratory controls, formal delivery/CLI and guarded 60-second simulated
receive passed. Exact commands, hashes, limits and cleanup/retention receipts are
in SELF_CHECK.md and docs/verification/latest-validation.json. Physical hardware,
monitor-DPI and wire timing remain separate acceptance.
