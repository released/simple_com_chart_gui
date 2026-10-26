# Native workflow expansion

Approved scope (2026-10-26): all eight recommendations from the latest review.
Use independently written native C++/MFC and existing Windows APIs only. Preserve
the uncommitted .028/.029/.030 source, delivery and personal INI. No publication
or firmware work is implied. Default language remains Traditional Chinese.

## Acceptance and implementation order

1. Named views: save/stage/apply chart checked keys, independent four-chart
   selectors/time windows and main plot window/fixed Y. View-only application
   never changes parsing, serial settings, capture, RX/TX or raw recording. New
   keys still default to none; missing saved keys are reported, never substituted.
2. Packet parameter grid: edit bounded name/value/min/max rows with existing
   unique/reserved-name, finite range and packet preview validation. Editing and
   accepting the grid do not transmit; retain existing confirmed Send once.
3. PNG export: main plot or four-plot composite, actual axes/labels/selections,
   bounded dimensions and atomic background output. Snapshot remains freeze.
4. Capture-row parser cases: explicitly copy a selected retained 4 Charts row
   into the existing offline case editor; review expected values before saving.
   Keep exact raw input and CSV escaping; no auto test oracle or parser Apply.
5. Persistent manual notes: separate bounded private sidecar, recording SHA256,
   length and byte spans. Verify before locating an already loaded source. Reject
   mismatched/expired sources and never infer positions from displayed line text.
6. Profile difference review: compare supported typed current/staged preferences
   before the existing explicit idle-only atomic Apply. Preserve unrelated/private
   settings and cancellation; recheck state after the modal review.
7. Self-contained HTML report: explicit selected retained fields/range, summaries
   and bounded static SVG plots; escape user text, no scripts/CDN/browser runtime,
   no duplicated raw archive. Atomic background output; original data unchanged.
8. FFT waterfall: bounded grayscale time/frequency history, existing Hann/mean-
   removed FFT, background snapshots, explicit source/rate and no implicit
   resampling. Reject irregular clocks unless an explicit justified rate is set.
   No calculation while stopped/hidden; gaps/configuration changes clear history.

## Architecture and boundaries

- Add small workflow cores with strict versioned private formats; use existing
  uart_files read/write/cancel and file_sha256/ReplayFile for safe completed files.
- Reuse Laboratory pages for optional workflows, a small native list dialog for
  table editing/review and a custom lightweight waterfall child. Do not expand
  daily main toolbars or add frameworks, Ribbon, demo data or dependencies.
- Limits: 1024 selected keys / four plots / 64 KiB view; 32 packet parameters;
  64 parser cases with <=4096-byte input; 256 notes / 1 MiB sidecar; HTML <=8
  channels and bounded points; PNG <=4096 each dimension / 16 million pixels;
  waterfall <=4096 FFT samples / 256 rows. Keep worker ownership/cancellation.
- INI and private views/notes/cases/profiles/recordings stay out of Git. Source
  English/TW catalogs and both complete user guides evolve together.

## Verification and cleanup

Create a fresh project-local recovery/ownership run before edits. Add focused
core and native Workflow scopes; select affected chart/profile/Lab/language
checks only. Verify exact raw preservation/no TX, data bounds, failure/cancel,
atomic files, actual UI routing and enlarged-font layout. No physical COM.
Reuse unchanged protocol/Python evidence. Keep separate check-enabled binaries.
At final acceptance compact results/hashes/necessary PNGs into the existing
verification receipt, run safe clean-obj, retire exact owned test/cache/baseline
files with project/path/reparse/hash/process checks, then measure all files.
Reuse the existing explicit 45 MB final Git-retention cap; temporary compiler
growth needs no new approval. Do not retain redundant build/verify archives.

Status: all eight features delivered as 1.0.1.031. Final focused core checks,
native EN/TW Font125 workflows, affected regressions, actual formal delivery and
60-second concurrent waterfall/RX/recording passed. Safe clean and exact owned
retirement completed; obj/verify are absent. Commands, hashes, limits, metrics
and final capacity are in verification/latest-validation.json and SELF_CHECK.md.
Physical UART/monitor-DPI and publication were not performed.
