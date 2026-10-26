# UART diagnostics and protocol laboratory

Approved 2026-10-26: implement all eleven recommendations from the read-only
competitive review. Native MFC/static CRT only; no third-party runtime/code,
firmware implementation, hardware test, Git commit or publication is implied.

## Baseline and retention

- Entry HEAD: 21e329ec6d848c2dc3ac6802ce1fd654192051a0; clean worktree;
  VERSION 1.0.1.027. Release EXE 1,954,816 bytes,
  SHA256 BB7F4ACF1FA08443329949477644BB28C0390C8FF8A2DB8F91A276224D89B08A.
- Private INI SHA256
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- Entry complete project 36,783,874 bytes; .git 26,473,677 bytes.
- Developer approved preserving Git and a final retained maximum 45,000,000 bytes
  for this delivery. Review at final cleanup; not a workspace-wide exception.
  The earlier 800 MB temporary-cap question is superseded by the subsequent
  standing authorization: necessary temporary compiler/test growth does not need
  per-run approval/caps. Growth is still bounded and cleaned/measured before
  completion. The explicit existing Git/final retention decision remains recorded.
- Keep one registered baseline copy only until comparison/recovery is complete.
  Fresh project-local tests own exact generated files and clean on every outcome.
  Preserve formal EXE/INI, user recordings and compact evidence. Final safe
  clean-obj plus metadata-based retirement of this run's generated caches/binaries.

## Implementation sequence and acceptance

1. [x] Mode-aware A/B diagnostics: bounded reason counters/examples; distinguish
   received bytes, framing/CRC/filter/extraction/time failures and unselected data.
2. [x] Background indexed recording range export: explicit record/time bounds,
   fresh .bin/.idx pair, valid offsets/clocks/generations, cancellation cleanup.
3. [x] Multi-parameter packet builder: named typed values/ranges, length/sequence,
   final checksum/ending, preview, private save/load; explicit generation-safe Send.
4. [x] Sequence continuity: user-selected key/step/modulus, observed jumps,
   duplicates/backwards/reset, independent source/generation; no wire-loss claim.
5. [x] PC response-time distribution from acknowledged/matched script steps only;
   bounded samples and p50/p95/p99; driver-only steps excluded, timeouts separate.
6. [x] Offline parser cases: bounded multi-case expected values, shared production
   parser, import/export, visible failures, never COM/TX or automatic Apply.
7. [x] Message-ID schema dispatch: bounded type-specific field layouts; existing
   length/checksum validation retained, unknown IDs visible and raw bytes preserved.
8. [x] Optional COBS/SLIP codecs: streaming bounds/resynchronization, raw recording
   unchanged; explicitly selected encode/decode, no automatic hardware action.
9. [x] Digital state history: selectable bit/enum fields, independent lanes,
   missing/generation gaps, bounded work; reported states, not electrical timing.
10. [x] Recording health report and valid-prefix salvage: originals read-only,
    separate outputs, no fabricated missing bytes/timestamps or overwrite.
11. [x] Optional finite CLI runner sharing script/transport core: explicit run,
    finite limits, CSV/exit status, cancellation/error/no retry/reconnect resume;
    cooperative GUI exclusion and no personal INI side effects.

## Integration and risks

- Use an owned, measured/scrollable laboratory window reached from UART; keep the
  everyday Messages/Chart/4 Charts UI unchanged. Separate core algorithms,
  recording I/O, presentation and controller integration by responsibility.
- Reuse TX leases, generation validation, background jobs, paged replay storage,
  current parsers and safe atomic file helpers. Never auto-open a port or transmit
  because a definition, profile or recording was loaded.
- Schema/codec options must work in live A/B and explicit offline binary replay;
  retain legacy config/profile reading and validate before replacing preferences.
- Test only new and affected core/native UI/receive/replay/command/startup paths.
  Add a dedicated native verification scope; check Debug lifecycle and enlarged
  fonts for new windows, formal Release startup/dependencies/size and selected
  performance paths. No unrelated Python/full historical suite.
- Hardware UART, physical timing, monitor-DPI and manual mouse behavior require
  separate evidence. Report gaps; fake transports do not establish board acceptance.
- Final documentation records actual checks, output sizes, cleanup and limitations;
  product README contains usage/limits only. Do not mark unchecked items complete.

## Acceptance record

Implemented in 1.0.1.028. Selected native core, Debug enlarged-font and Release
Laboratory controls, affected Debug analysis/receive paths, finite CLI exclusion,
formal Release startup and an isolated 60-second RX/recording soak passed. These
checks use fake transports/files, not hardware acceptance. Final hashes, scoped
commands, retained evidence, performance limits and cleanup/size are recorded in
`../SELF_CHECK.md` and `verification/latest-validation.json`.
