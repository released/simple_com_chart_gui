# SELF_CHECK.md

## Approved R1..R10 improvements (1.0.1.038, 2026-10-31)

Changed behavior and file responsibilities are mapped to R1..R10 in HANDOFF.
The following are new runs for this change, not reused historical acceptance.

| Check | Result and observed evidence |
| --- | --- |
| Release build/core | PASS 6/6: telemetry_core, uart_optimization, uart_session, uart_analysis, uart_workflow, ui_language (7.28 s final selected run). After adding six catalog assertions, ui_language alone passed 1/1. |
| Debug native | PASS Serial, Optimization, Charts, Session, Advanced, AnalysisExpansion, Workbench. |
| Release native | PASS Serial, Optimization, Soak. Optimization rebuilt/retested in both configurations after test-only capture synchronization changes. |
| Capture lifecycle R1 | Manual successful connection clears old extracted origin/Snapshot; device seconds restart at zero. Automatic recovery preserves raw history, capture and Snapshot. Shared reset callers tested by the selected scopes. |
| Keyboard R2 | Manual disable survives unavailable/available and actual file-TX gating; the next connection rearms. Existing ASCII echo, Ctrl+C, paste generation validation and no stale TX remain covered. |
| Apply R3 | Invalid unapplied Regex/Max CH drafts do not interfere with explicit parser/framing Apply. Failed saves keep parser/framing, clocks and captured rows. First invalid filter still requires a valid Apply. |
| Parsing R4/R5 | Exact grouped/parenthesized/bracket maps; clocks excluded; final 128-byte scalar/group/vector/UTF-8 limits; deeply malformed brackets bounded; valid neighboring fields retained. |
| Session R6 | Missing requested readable filenames/counts shown, no manifest emitted; normal adjacent readable inclusion and hashes pass. Native split-location fixture also passes. |
| UTF-8 R9 | Chinese error text survives the real BackgroundJob exception path; invalid bytes use bounded escapes. |
| R10/docs | Complete TW/EN guides, README, source catalogs and contracts agree; catalog translations tested. Python parser parity claim removed; Python unchanged. |
| Formal delivery | PASS exact-hash Release copies in tw/en: three startup tabs, empty histories, recording options cancel, normal close and production test/demo rejection. Personal INI unchanged. |
| TST-01 | PASS static workspace clean audit 107 scripts. Safe clean 1,047 files / 87 directories; 2,355,489,085 bytes removed. Repeat clean zero. All 198 protected build files hash-identical; obj/runtime-bin absent. Owned run receipts cleanup_complete=true. |
| TST-02 | Cleaned size before record updates 57,676,647 bytes; Git 38,162,544. User rejected 65 MB; the prior 45 MB cap remains unmet. Follow-up audit found zero test-only EXE/bin, obj/PDB/lib; 15 cleanup receipts complete with zero retained binary executables. Remaining two EXEs are formal delivery; remaining two .bin files are user recordings. Repeat clean zero; no Git/protected-data changes. Final measured bytes are in latest-validation.json. |

- Commands: `scripts/build.ps1 -Configuration Release -TestRegex
  '^(telemetry_core|uart_optimization|uart_session|uart_analysis|uart_workflow|ui_language)$'
  -ExtraTargets uart_optimization_tests,uart_session_tests,uart_analysis_tests,uart_workflow_tests,ui_language_tests`;
  later `-TestRegex '^ui_language$' -ExtraTargets ui_language_tests`.
- Native: `scripts/verify_runtime.ps1 -Configuration Debug -Scope <selected>`;
  Release uses Serial/Optimization/Soak. Reused the current built binary with
  `-SkipBuild` between scopes when application source had not changed.
- Formal: `scripts/verify_delivery.ps1 -Configuration Release -UiLanguage tw|en`.
  Cleanup: workspace `scripts/verify_clean_scripts.ps1`, then project
  `scripts/clean_mfc.bat` twice. Evidence retained under `build/verify/evidence`.
- All-file read-only audit used the actual `telemetry_tests.exe` with
  `tests/telemetry.txt` plus all eight user paths. Counts below are lines / parsed /
  filtered-numeric / distinct numeric keys. Filters differ per reviewed case;
  teraterm44 deliberately excludes INIT, so zero filtered rows is correct.

| File | Counts | Exact representative map / independent literal filter oracle |
| --- | --- | --- |
| 20261028_140028_1.log | 927 / 894 / 747 / 5 | PASS |
| teraterm2.log | 4101 / 3711 / 1 / 43 | PASS |
| teraterm3.log | 692 / 692 / 692 / 22 | PASS |
| teraterm11.log | 665 / 549 / 549 / 19 | PASS |
| teraterm22.log | 458 / 398 / 1 / 44 | PASS |
| teraterm33.log | 602 / 480 / 4 / 27 | PASS |
| teraterm44.log | 217 / 193 / 0 / 38 | PASS |
| 20261028_104821_1.log | 584 / 584 / 584 / 23 | PASS |

- Paced visible R7: eight reviewed cases x three, on each of Chart and 4 Charts;
  48 exact rows/field maps/raw bytes/source spans per run, eight actual UI filter
  applications, no loss. Requested 50/62/75 ms; final Release actual 50..76 ms.
  Accepted-and-painted host p95 94/410 ms, p99 111/460 ms, maximum pump gap
  47/510 ms. Counts and screenshots pass; timing varies with host load and is not
  physical-display/UART acceptance. Settling before screenshot is outside timing.
- R8 reference: 20,000 retained / 10,001 visible samples; copied samples/update
  70,001 -> 10,001, five-run median for 20 CPU-only updates 72.7077 -> 5.855 ms.
  Range/extrema/latest results match reference; no whole-GUI speed claim.
- Release Soak: Messages page, 60,046 ms, 5,943,296 produced/RX/raw-recorded bytes,
  exact ordered six rotation segments, raw/line loss 0/0, max UI pump gap 532 ms,
  peak private bytes 16,814,080; bounded retention and memory checks PASS.
- Resolved verification-only issues: owner-drawn Snapshot is checked through its
  real command/state/caption, not BM_GETCHECK; screenshot waits for repaint and
  flushes GDI before bitmap readback. No product change was needed for these.
- NOT RUN: real COM/driver/target acceptance, physical display latency, actual DPI
  and cursor dwell. No All/Python, protocol or unrelated full-suite reruns.
- Remaining compatible limit: Include log=Yes requires same-folder readable logs;
  missing files now visibly fail before manifest creation. Details in both guides.

## Eight real-log parser/filter audit (1.0.1.037)

- Changed: generic bracketed numeric extraction, exact readable-RX prefix handling,
  clock-bracket rejection, focused fixtures and optional real-log audit reporting;
  parser contracts, guides and version.
- PASS Release `telemetry_core` 1/1. Initial run correctly failed because generic
  brackets exposed a false clock channel; after excluding exact `HH:mm:ss.mmm`,
  the complete rerun passed. Strict bare scalar, grouped comma and existing parser/
  filter/model/capture/export behavior remain covered.
- PASS direct native audit using literal Include `[` and Exclude `ERR`:
  `20261028_140028_1` 927/879 parsed, 817 filtered-parsed, 3 keys;
  `teraterm2` 4101/3636, 3251, 36; `teraterm3` 692/692, 692, 18;
  `teraterm11` 665/527, 411, 20; `teraterm22` 458/391, 351, 39;
  `teraterm33` 602/478, 413, 25; `teraterm44` 217/189, 176, 35;
  `20261028_104821_1` 584/584, 584, 19. ERR-bearing bracket lines were
  demonstrably rejected; all files retained filter-matching numeric rows.
- Timing read-only audit: 140028 has 927 timestamps, 121 zero deltas, positive
  20..422 ms/median 64 ms and 413 in 50..75 ms. 104821 has 584 timestamps,
  31..102 ms/median 63 ms and 343 in range. Tera Term logs lack timestamps, so
  their 50..75 ms spacing is NOT VERIFIABLE from these files.
- TST-01: clean removed 339 allowlisted files and 35 empty generated directories;
  all 60 protected hashes stayed identical, all eight user logs remain, and
  obj/runtime-bin are absent. Final GUI is 2,451,456 bytes, SHA-256
  `0B8DEC515000749B26356BBDE15574217B9E4638E4920E885C9E62E289AD235F`.
  `git diff --check` passed with line-ending warnings only.
- NOT RUN: real UART, physical timing/keyboard/DPI, unrelated protocols/Python/full
  suite. No commit/push.

## Bare numeric telemetry channel (1.0.1.036)

- Changed: default Key/value parser maps an entire finite scalar line to `value`;
  focused parser tests, contract/user docs and version.
- Why: unnamed scalar streams had no channel key, so Chart had nothing to list or
  select even though Messages showed the values.
- PASS Release `telemetry_core` 1/1: `-8.1039` becomes only `value`; a copied
  `[13:53:20.540] 0.0000` display line remains unparsed rather than guessing.
  Release GUI/CLI rebuilt after gracefully closing exact-path PID 31360 following
  an initial output-lock-only LNK1104. Final GUI is 2,448,384 bytes, SHA-256
  `F95C5F3BC451BDE87D92ED1F85370B1D4D66AF2EB86E8418F12C58AA7C2ADC0C`.
- TST-01: clean removed 338 allowlisted files and 35 empty generated directories;
  all 47 protected hashes remained identical. Obj/runtime-bin are absent while
  retained evidence and recordings remain. `git diff --check` passed with only
  line-ending warnings.
- NOT RUN: physical COM/keyboard/DPI, unrelated runtime/protocol/Python/full suite.
  No commit/push.

## Keyboard TX defaults and real-log parser compatibility (1.0.1.035)

- Changed: keyboard availability/default echo and ASCII preview/echo paths;
  bounded separated `key:,number` telemetry parsing with group-qualified keys;
  focused unit/runtime fixtures, version, guides and behavior contracts.
- Why: keyboard focus appeared inactive because TX required manual arming and echo
  was off/HEX-only. The user's device emits comma-separated key and numeric tokens,
  which the prior same-token parser ignored, leaving only six startup labels.
- PASS Release `telemetry_core`/`uart_operations` 2/2. PASS Debug TW Optimization
  runtime: new-generation auto-arm, same-generation manual-disable retention,
  reconnect re-arm, missing-setting echo-on, ASCII escaped echo, paste/Ctrl+C and
  stale-generation rejection. No physical COM or keyboard automation was used.
- Evidence for the named capture: 176,072 raw bytes, 1,143 continuous indexed RX
  entries and zero offset gaps; therefore the cause is parser grammar, not receive
  speed. Tests cover `[S07],raw:,-19056,...,[Z]:,2.0041` and strict malformed/
  nonnumeric rejection. Formal Release GUI is 2,447,872 bytes with SHA-256
  `5844C81B690583F2F47CF8C103176F61495374FA9E3ABF700013297D85F764F0`.
- TST-01: after the preceding 656-file generated cleanup, the fixed live bottom-up
  pass removed 62 empty generated directories; repeat clean removed 0. `obj` and
  `verify/runtime-bin` are absent, evidence/log folders remain, and all 43 protected
  hashes stayed unchanged. `git diff --check` passed with line-ending warnings only.
- NOT RUN: real UART, physical keyboard/DPI, Python, unrelated protocols/full suite.
  No commit/push.

## Expanded safe clean of generated build/test outputs

- Changed: `clean_intermediates.ps1`, both batch help texts and clean/build docs.
  CMake cleanup requires a cache whose source root exactly matches this project;
  core EXE names come from generated projects. Runtime cleanup is restricted to
  fixed names below `build/verify/runtime-bin/<Configuration>`. Candidate running
  EXEs reject the clean before deletion; cache identity is removed last.
- PASS: both help entries, `clean_build.ps1 -WhatIf`, Python-cache preflight,
  23 protected fixture files/source, repeat clean, and native/source-cache junction
  rejection. Static whole-build deletion audit passed all 6 project clean scripts.
- PASS actual scoped retention: configure-only regenerated both CMake trees;
  `clean_mfc.bat` removed 172 generated files / 4,562,504 bytes and the immediate
  `clean_build.bat` removed 0. All 17 formal/catalog/INI/log/evidence file hashes
  stayed identical; final obj files=0, runtime-bin files=0, evidence files=9,
  recording files=3. Initial diagnosis inventory was 91,769,496 bytes; final build
  is 3,070,847 bytes because the preceding expanded run had already removed the
  large maps, runtime EXE and test EXEs.
- PASS fixed-name non-Release path: four owned Debug GUI/CLI/catalog copies were
  removed, the empty Debug folder was retired, and the following clean removed
  zero files. Root formal files remained outside that exact allowlist.
- The shared preservation harness's fixture/safety phases passed, but its optional
  actual-project comparison still treats every pre-existing CMake project file as
  protected and therefore reports the newly authorized deletion. This is an
  obsolete assertion for the expanded policy, not reported as a complete harness
  PASS. The scoped producer/path/hash/idempotence check above covers actual output.
- NOT RUN: application rebuild/runtime/COM/Python feature suites; scripts and docs
  do not change the executable. `git diff --check` passed. No Git commit/push.

## Split readable/raw recording destinations (1.0.1.034)

- Changed: optional recorder readable base; GUI recording path routing and raw
  folder creation; receive/runtime fixtures; recording documentation and version.
- Why: let users place the primary readable `.log` anywhere while managing raw
  `.bin`, receipt `.idx` and required `.sccmeta` together beside the application.
- Verify: `build.ps1 -Configuration Release -TestRegex '^uart_receive$'
  -ExtraTargets uart_receive_tests`; `verify_runtime.ps1 -Configuration Debug
  -Scope MessageLog -UiLanguage tw`.
- PASS core 1/1: exact split destinations, metadata adjacency, readable content,
  legacy single-base recording, rotation, no-overwrite, overflow and cleanup.
  PASS Debug TW MessageLog runtime: auto-created raw folder, user-selected readable
  log folder, companion separation, stop/drain and affected existing UI/export
  checks. No hardware port was opened. Release GUI/CLI built successfully after
  gracefully closing exact-path GUI PIDs 3760/23980; both initial failures were
  only LNK1104 output locks.
- TST-01: final preflighted safe clean removed 497 allowlisted compiler files;
  173 protected build files remained SHA-256-identical, including EXE/INI, user
  recordings and retained evidence. Final GUI 1.0.1.034 is 2,446,336 bytes,
  SHA-256 `8583AA7B288548AE07BC37AED02BF16F11246964A6DCB2B23DBB5DC9379E1688`;
  private INI SHA-256 remained `11E056B51F9F890C6CE97518FF47A3C1226E35B932C0F5D3AA99C7231DBA8BF5`.
- NOT RUN: physical UART, unrelated protocols, Python, monitor DPI or full suite.
  No Git commit/push performed.

## Messages log controls and localization (1.0.1.033)

- Changed: message_page/options and new message_log_controls; recorder status;
  main dialog export/recording/UI routing; test-only runtime_message_log, runner,
  receive-core fixture ownership, build/version, paired catalogs and both guides.
- Why: translate the reported menus/statistics, separate snapshot Save As from
  continuous recording beside Find, explain empty/error outcomes, mutually gate
  recording buttons and show actual log leaf/KB without exposing paths.
- Verify: build.ps1 -Configuration Release -TestRegex '^(uart_receive|ui_language)$'
  -ExtraTargets uart_receive_tests,ui_language_tests; verify_runtime.ps1
  -Configuration Debug -Scope MessageLog -UiLanguage en; TW with -Font125;
  verify_delivery.ps1 -Configuration Release -UiLanguage tw.
- PASS selected cores 2/2, including exact readable log size/rotation metadata and
  637 paired translations. UART fixture cleanup passes; five disallowed roots
  (workspace parent, sibling, Windows, global user TEMP, volume) reject before
  filesystem mutation. No real ports are opened by these fixtures.
- PASS EN 25/TW 24 native checks: recursively translated menus; visible wrapped
  controls; empty Save As notice; PC-time default; immutable 1536-byte snapshot
  shown as 1.50 KB; failed export preserves prior destination/status; continuous
  300 KB RX independent of Clear; actual readable log size; stop/drain button
  states; existing recording never overwritten and failure visible; layouts at
  760/1120/1500 widths. Affected existing export checks pass in both languages,
  font checks in EN. Initial fixture synchronization failure resolved by waiting
  for UI polling as well as worker completion; complete follow-ups pass.
- PASS actual final Release startup/three tabs/options/cancel/close and removed
  demo/self-test rejection. GUI 2,445,312 bytes, CLI 434,176. Static MFC/CRT,
  system-only imports, ASLR/DEP/high entropy and production test/theme exclusion
  preserved. Exact-target PID 37736 closed gracefully; private INI unchanged from
  entry SHA256 11E056B51F9F890C6CE97518FF47A3C1226E35B932C0F5D3AA99C7231DBA8BF5.
- TST-01/TST-02/BLD-04: six-script audit passes; safe clean removed 515 files /
  1,259,493,458 bytes; retired 210 exact owned files / 93,052,457 bytes. obj/verify
  absent, five protected formal/runtime files unchanged. Compiler/test TEMP/TMP
  stayed inside the owned project run. No external or user-data deletion.
  Two selected PNGs total 110,798 bytes; concise receipt retains .032 evidence.
  Final source/document/hash/capacity checks are recorded in latest-validation.json;
  the existing 45 MB Git-retention exception applies. Prior .032 work preserved.
- NOT RUN: physical UART, actual monitor DPI/keyboard/foreground/elevation,
  Python/full-protocol suites or a new receive-throughput test. Retained earlier
  receive evidence applies to unchanged processing. No Git commit/push performed.

## Editable chart time ranges (1.0.1.032)

- Changed: new chart_time/controls headers; ChannelModel/PlotView minimum and
  fractional X ticks/wheel; main/four-plot controls, Enter/focus/error routing,
  Snapshot pending range, connection reset and view validation; initialization
  guards/final DataPage layout; native/core fixtures, runner scope, three paired
  translations, version and both complete guides/engineering records.
- Why: provide 250 ms, 500 ms, every second 1..300 and direct bounded input,
  preserving independent plots, default none, raw capture and existing zoom.
- Verify: build.ps1 -Configuration Release -TestRegex '^(telemetry_core|uart_workflow)$'
  -ExtraTargets uart_workflow_tests; verify_runtime.ps1 -Configuration Debug
  -Scope Charts; -Scope ChartTime -UiLanguage tw -Font125; final formal delivery.
- PASS selected Release cores 2/2, including all integer choices, units/locale,
  invalid/bounds/nonfinite values, exact 250 ms pruning, reset preservation and
  fractional view round trip; uart_workflow 49 checks. Later changes only affected
  UI initialization/Snapshot routing, so unchanged core results were reused.
- PASS Debug Charts (Y/table/default-none/labels/preferences and 20 time cases)
  and Debug TW Font125 ChartTime 20/20. Tests cover all five editable 302-choice
  controls, Enter/focus loss, bounds, independent 0.25/0.5/299/300 ranges, raw-data
  preservation, 400 ms peak inclusion/exclusion, extracted-clock alignment, wheel
  minimum, fractional PNGs and Snapshot/Live with unchanged-focus zoom retention.
- Initial concurrent startup timeout was probed on an isolated owned process;
  no assertion dialog found. Follow-up exposed missing plot layouts/labels under
  default fonts. Ignoring premature time notifications and laying out all children
  after creation fixed it; complete affected scopes then passed. Probe/test copies
  were retired. Font125 is not physical DPI or physical keyboard acceptance.
- PASS actual final Release TW startup/three tabs/options/cancel/normal close and
  removed demo/self-test flag rejection. GUI 2,432,000 bytes (+4,608); CLI 434,176;
  static MFC/CRT, system-only imports, x64 ASLR/DEP/high entropy and production
  test/theme exclusion preserved. Private INI hash unchanged:
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- PASS matching 15-section TW/EN guides, six consistent Markdown tables each,
  UTF-8/local links, 581 paired catalog translations and diff --check. All 401
  entry tracked paths retained; no unrelated source changes or publication.
- TST-01/TST-02/BLD-04: safe clean removed 534 files / 1,271,534,315 bytes with
  remaining pre-clean hashes preserved; retired 226 exact owned files / 92,091,793
  bytes. obj/verify absent; formal EXEs/catalogs/private INI retained. Two selected
  PNGs total 187,600 bytes plus compact receipt; final capacity uses the existing
  45 MB Git-retention exception. Exact manifest/results/capacity are in
  docs/verification/latest-validation.json, with previous .031 evidence nested.
- NOT RUN: physical UART, actual monitor DPI/foreground/elevation, Python or full
  protocol suites. Unchanged .031 default-30-second receive soak evidence reused;
  no new hardware/throughput claim. Expired rolling samples cannot be recovered
  by enlarging the range; ranges can be saved in named views, not a new INI format.

## Native workflow expansion (1.0.1.031)

- Changed: workflow_views/notes/profile_diff/report/waterfall cores, shared packet
  validation, file_digest extraction, native parameter/review grid and PNG/canvas;
  five Laboratory pages/controllers, selected-row provenance, chart/table restore,
  scoped fixtures/build/version, paired catalogs, both complete guides and records.
- Why: all eight approved generic UART workflow features using existing native
  C++/MFC and Windows APIs, bounded file/background work and explicit no-TX edits.
- Verify: build.ps1 -Configuration Release -TestRegex '^(uart_workflow|ui_language)$'
  -ExtraTargets uart_workflow_tests,ui_language_tests; verify_runtime.ps1 -Scope
  Workflow -UiLanguage en|tw (Font125 for layout); affected Charts/Advanced/
  AnalysisExpansion/Lab; formal verify_delivery TW/EN; Soak -WaterfallSoak.
- PASS initial selected telemetry_core/uart_session/uart_workflow 3/3; final
  ui_language/uart_workflow 2/2, 48 workflow checks. Bounds, malformed formats,
  filename aliases/UTF-8, atomic/cancel, exact hashes/byte spans, parser prefixes,
  profile legacy merge, SVG escaping/missing segments and FFT continuity covered.
- PASS final Release EN and Debug TW Font125 native Workflow: 55 checks each,
  actual routing/modal review, view restoration/default none/frozen rejection,
  validated parameter rows, no TX/reopen, real PNG decoding, reviewed parser case,
  SHA256 note locate, escaped HTML, file-work Cancel and waterfall stop/drain.
  PASS affected Release Charts/Advanced/AnalysisExpansion/Lab. Three selected TW
  PNGs inspected. Font125 is enlarged fonts, not physical monitor-DPI acceptance.
- PASS formal Release 1.0.1.031 TW/EN startup/three tabs/options/cancel/normal close
  and demo/self-test flag rejection. GUI 2,427,392 bytes (+136,192); CLI 434,176;
  two delivery catalogs 104,839 bytes, matching 578 paired source translations.
  Static MFC/CRT, Windows-only imports, x64 ASLR/DEP/high entropy and test/theme
  exclusion preserved. No new dependencies. Personal INI SHA256 unchanged:
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- PASS concurrent waterfall/RX/recording for 60,094 ms: exact 5,132,288 raw bytes
  over five ordered segments; zero raw/line loss, 64 waterfall rows, max pump gap
  125 ms, peak private 15,618,048 bytes, one-core CPU 28.913%. No hardware or
  cross-run speed claim. Worker drains and bounded retention passed.
- PASS matching 15-section complete TW/EN guides, six consistent Markdown tables
  each, UTF-8/links/catalog pairing, product-only README and git diff --check.
  All 332 entry source paths preserved. Existing .028/.029/.030 work retained.
- TST-01/TST-02/BLD-04 PASS: safe clean removed 807 files / 2,181,392,758 bytes
  without changing remaining pre-clean hashes; retired 271 exact owned test/cache/
  recovery files / 101,362,095 bytes with persisted per-file hashes, boundary/
  reparse/process checks. obj/verify absent. Formal EXEs/catalogs/private INI kept;
  three new PNGs total 80,444 bytes. Final complete capacity uses existing 45 MB
  Git-retention exception; exact bytes and cleanup manifest are in
  docs/verification/latest-validation.json with previous .030 evidence nested.
- NOT RUN: physical UART, device throughput, real DPI/foreground/elevation,
  unrelated Python/protocol full suites or publication. Prior applicable protocol
  evidence reused. Notes navigation remains single-file; technical diagnostics
  may remain English. No remaining work in the approved eight-feature batch.

## Native language and matching user guides (1.0.1.030)

- Changed: ui_language/ui_localized_controls, native presentation callers, menus,
  measured labels and canonical-choice readers; two UTF-8 source/delivery catalogs;
  CMake packaging/version, scoped native/core/runner checks, bilingual user guides,
  README/agent/engineering records and exact delivery-catalog ignore exceptions.
- Why: default Traditional Chinese and saved English/TW next-launch selection;
  complete Traditional Chinese USER_GUIDE.md with matching USER_GUIDE_EN.md.
- Verify: build.ps1 -TestRegex '^(telemetry_core|ui_language)$' -ExtraTargets
  ui_language_tests; native -Scope Language -UiLanguage tw|en; affected Session/
  AnalysisExpansion; verify_delivery.ps1 -UiLanguage tw|en. Select by impact.
- PASS 22 language-core cases; Release selected 2/2, final bounded-read follow-up
  ui_language 1/1. Missing/invalid catalogs/keys, UTF-8/size limits, INI default,
  persistence/failure/private-key preservation, real captions and canonical enum
  values covered. Raw EDIT content and user keys remain unchanged.
- PASS actual Debug TW Font125/EN and Release TW/EN: 28 checks each, 13 Laboratory
  pages, main labels/tab, parser/source canonical values, fake RX and no implicit
  TX/reconnect/data clearing on language save. An oversized CRC caption failed
  initially; shortened translation and scoped retest passed. Passing TW main and
  English options PNGs inspected. Font125 is not actual monitor-DPI acceptance.
- PASS affected Release Session/AnalysisExpansion; all three formal startup tabs
  in default TW and explicit EN, modal recording options/cancel, removed-flag
  rejection and normal close. Final UI_language input-read refinement changes
  neither controls nor rendering; earlier EN/Debug UI evidence remains applicable.
- PASS 60,063 ms simulated receive: 5,160,960 exact ordered produced/RX/raw bytes,
  five segments, zero raw/line loss, max pump gap 328 ms, peak private 15,212,544
  bytes, one-core CPU 43.496%. No physical throughput or cross-run speed claim.
- GUI 2,291,200 bytes (18,944 above .029); catalogs 42,501/43,085 bytes. Windows-only
  imports, x64 ASLR/DEP/high entropy, static MFC/CRT and stripped production checks
  preserved. Personal INI SHA256 unchanged:
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- PASS both guides have corresponding 14 sections, valid links/Markdown table
  columns and UTF-8 text. README describes product behavior, not development history.
- TST-01/TST-02/BLD-04 PASS: 654 compiler files / 1,944,700,087 bytes removed;
  protected pre-clean contents unchanged. Then retired 244 exact owned files /
  91,556,584 bytes, including caches, runtime/test EXEs and obsolete baseline.
  Reparse/path/hash/process checks and persisted receipt precede deletion.
  build/obj and build/verify absent; formal outputs/private INI unchanged. Kept
  four selected PNGs and the compact receipt. About 38.47 MB including Git uses
  the existing approved 45 MB final exception; Git alone is 26.47 MB. Exact bytes:
  docs/verification/latest-validation.json. No full suite/Python/physical UART
  retest or publication; unchanged protocol/worker evidence reused from .029.

## UART session/workflow (2026-10-26, 1.0.1.029)

- Changed: recording limits/settings/metadata and manifest cores; device catalog;
  source framing, shared CRC, JSON selection/persistence, calculus/traffic spans;
  six native Laboratory pages/controllers and recording options; CMake/version,
  affected runtime/core fixtures, product guides, plans and compact evidence.
- Why: deliver the eight approved generic UART workflow features using the
  existing native/static architecture and TX/profile/background ownership.
- Verify: select commands from docs/verification/latest-validation.json by impact;
  new core is uart_session; new native integration is verify_runtime.ps1 -Scope
  Session (Debug -Font125 / Release). Build/tests are agent responsibilities.
- PASS Release selected cores 10/10, then affected parser/profile cores 5/5.
  Final uart_session: 76 checks, including actual writer byte accounting/metadata,
  quiet timer, standard CRC vectors/ranges/codec/length/corruption, split CR/CRLF/
  delimiter/idle offsets and generation boundaries, bounded JSON/path failures,
  old workspace defaults/new version markers, calculus extremes/gaps, SHA256/
  corruption/cancel/no-overwrite/path limits and passive matching/ambiguity/spans.
- PASS actual native Debug Session -Font125 at 700x520 and Release Session:
  36 checks each. Fields/labels fit with live fonts and scrolling; preview/Apply,
  fake UART RX/confirmed finite TX, limit keeps COM connected, session verification/
  staging/private-INI preservation, calculus/inspect/cancel/worker drain verified.
  Two passing Debug PNGs inspected. Corrected new fixture dropdown/manual-sample
  setup mistakes; no product behavior was changed to accommodate those mistakes.
- PASS affected Debug Uart, AnalysisExpansion and Lab; reused applicable previous
  evidence rather than rerunning All/Python/unaffected transfer protocols.
- PASS formal GUI three startup tabs, normal close, recording modal controls and
  rejection of removed flags; CLI seven read-only/fail-closed checks. GUI/CLI
  2,272,256 / 434,176 bytes; x64, system-only imports, ASLR/DEP/high entropy,
  static CRT/MFC and production map without runtime/demo/Ribbon/theme managers.
- PASS isolated 60,062 ms guarded receive/recording: 5,263,360 exact ordered raw
  bytes, six segments, actual output 11,000,174 bytes including metadata/text/index,
  zero raw/line loss, max UI gap 547 ms (<1000 ms), private peak 14,110,720 bytes,
  one-core CPU 37.4873%. Retention expiry is distinct from loss. Performance runner
  waits for active builders and preserves idle orphan reused MSBuild nodes.
- PASS private INI unchanged: 1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  Source entry archive: 303 files; no missing paths, 256 unchanged before final
  engineering-doc edits. Existing uncommitted .028 work and Python retained.
- Final TST-01/TST-02/BLD-04: actual clean removed 913 compiler files /
  1,932,389,468 bytes; 1,232 protected hashes were unchanged. Additional retirement
  removed 261 proven generated files including test EXEs, evidence and the one
  recovery archive. obj/verify/Debug are absent, formal EXE/INI hashes still match.
  The late retirement receipt guard used a stale path set; its extra byte total
  was not retained. Independent residue/boundary/hash checks passed; no invented
  byte total or outside deletion is claimed. See the clean/preservation/size record
  in docs/verification/latest-validation.json. It preserves .028/.027 evidence,
  adds 22 compact reports including latest core output, and retires proven redundant
  test/cache/baseline data only within this project. No post-clean rebuild.
- NOT RUN: physical UART/board/driver/property completeness, physical monitor DPI/
  mouse/foreground/electrical timing and OS file picker/explicit clipboard action.
  Serialization/no-overwrite/cancel are covered separately. No Git commit/push.

## UART laboratory (2026-10-26, 1.0.1.028)

- Changed: CMake/VERSION/resources; bounded codecs/schema/packet/diagnostics/
  recording/metrics/digital/finite-script cores; native Laboratory and controllers;
  receive/replay/profile metadata; optional CLI; targeted fixtures/build runners;
  product/engineering docs and compact validation record. See UART_LAB_PLAN/GUIDE.
- Why: implement all eleven approved generic UART diagnostic/protocol features
  with existing TX ownership, explicit hardware actions, raw-wire preservation,
  bounded workers/history and no new installation/runtime dependency.
- PASS Release selected cores 7/7: telemetry_core,binary_frames,uart_commands,
  uart_advanced,uart_analysis,uart_lab,uart_lab_performance. Later strict case/UTF-8
  changes: affected telemetry_core/uart_advanced/uart_lab 3/3; font-only final
  Release build: bundled telemetry_core 1/1. Reused unchanged evidence within this
  delivery rather than rerunning All/Python/full historical suites.
- PASS native Debug -Scope Lab -Font125 and Release -Scope Lab: seven actual
  pages, nested font/scroll bounds, preview without TX, fake confirmed/partial/
  generation-changed TX, COBS/ID receive dispatch, diagnostics/profiles, background
  cases/range/health and digital gaps/Reset. Final packet/digital PNGs inspected.
  PASS affected Debug -Scope AnalysisExpansion and -Scope Uart. Fixture close
  order updated to drain Workbench/UART/recorder; production shutdown unchanged.
- PASS formal scripts/verify_cli.ps1 seven checks: help/validate, missing confirm,
  GUI guard under renamed copy, no portable settings or target bytes changed.
  PASS scripts/verify_delivery.ps1 -Configuration Release on final GUI hash:
  three empty startup tabs, recording options/cancel, close and removed flags.
- PASS diagnostics microbenchmark: 5-repeat medians/100,000 lines per case against
  baseline + max(15%,5 ms) chosen before measurement. Numeric 292.367->300.309 ms;
  include miss 2.1367->4.9373; excluded 2.5875->2.9567; no values 26.6703->28.3789;
  parser error 338.051->341.868. Measures incremental diagnostics, not zero cost.
- PASS isolated Release -Scope Soak -SoakSeconds 60: 60,109 ms, 5,238,784 bytes
  produced/RX/recorded, five ordered segments, raw/line loss zero, max UI gap
  94 ms, peak private 13,406,208 bytes, one-core CPU 46.868%. Bounded retention
  expiry is reported separately. No builders/other private fixtures during run.
- PASS Release deployment: GUI 2,120,704 bytes (entry .027: 1,954,816); optional
  CLI 432,640; combined EXEs 2,553,344. System-only imports, x64, static CRT/MFC,
  ASLR/DEP/high entropy; production map contains no runtime-test/demo/Ribbon or
  theme-manager symbols. SHA256 and cleanup/size in latest-validation.json.
- PASS retention mechanism 15 checks: pass/fail/cancel, active process, modified/
  unknown evidence, parent/junction rejection and formal EXE/INI protection.
  Missing source fails before run creation; the test bootstraps only its small
  session helper after a clean checkout, without rebuilding/rerunning the full app.
  Final safe clean/exact generated-output retirement uses the canonical receipt.
  Private INI still matches 1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- PASS final TST-01/TST-02/BLD-04: static clean audit six entries; safe clean
  removed 752 compiler files / 1,732,813,878 bytes, 71 protected hashes unchanged.
  Following report compaction, 265 proven generated files / 85,734,149 bytes
  and 68 empty directories retired inside the independently selected project.
  All 322 unselected non-Git hashes matched at that checkpoint. obj/verify absent,
  no active selected process/reparse path, no generated payload or recovery copy
  retained. Eighteen compact report texts reproduce original bytes/SHA256.
  Final project about 37.59 MB including Git 26,473,677 bytes uses the existing
  45 MB Git-retention decision; normal 20 MB target is not claimed. Exact bytes
  after final document edits are in latest-validation.json. No post-clean rebuild.
- NOT RUN: physical UART/board/driver, physical monitor DPI/mouse/foreground,
  real CLI run and electrical/wire timing. Fake transports/files do not establish
  hardware acceptance. No Git commit/push. Common size-policy edit has document
  checks only: temporary development excess allowed without repeated questions;
  cleanup/final 20 MB target, protected data and explicit final exceptions remain.

## Four requested build directories retired (2026-10-26, TST-01/TST-02)

- Changed: retired proven generated contents of build/obj, build/verify,
  build/Debug and build/legacy_native_mfc; AGENTS/README/MFC_BUILD/HANDOFF/
  SELF_CHECK and compact docs/verification/latest-validation.json only.
- PASS scoped cleanup: 3667 exact registered files / 74,779,672 bytes and 574
  checked empty directories removed. All four roots absent; all deletion within
  the selected project. No reparse traversal, active selected process, outside
  path or directory-name-only deletion. Source/ownership checks included current
  published .027 checkpoint and historical migration documentation.
- PASS preservation: 290 unselected files unchanged at deletion checkpoint;
  delivery EXE/INI SHA256 unchanged (see HANDOFF). build contains only these two
  files. No application source/build-script/VERSION/Git mutation.
- Evidence: docs/verification/latest-validation.json retains five previous
  report texts/hashes and compact cleanup receipt, explicitly reused, not rerun.
  PASS: all five original byte hashes reconstructed using recorded UTF-8,
  UTF-8 BOM or UTF-16 LE BOM encoding; compact evidence is 10,446 bytes.
  Earlier build/verify evidence paths below are historical and were retired.
- Size before final documentation edits: 111,406,971 -> 36,637,441 bytes, of which
  .git is 26,333,246 bytes. 20 MB threshold remains over budget; developer decision
  is pending. Git review found no garbage; no history removal/repacking performed.
- NOT RUN: app rebuild/native/Python/performance/hardware/full suites or repeat
  clean. No application or clean code changed; hashes establish binary identity.
  PASS final verification: UTF-8/fences/local links, git diff whitespace,
  compact evidence hashes/schema, four-root absence, delivery hashes and size.
  Git fsck: no integrity errors, same three unreferenced blobs preserved.

## Final redundant build-output retirement (2026-10-26, TST-01)

- Changed: removed43registered generated build files; AGENTS/HANDOFF/SELF_CHECK
  documentation only. No application source, build scripts, VERSION, EXE or INI
  change; no new archive or backup was created.
- PASS publication/recovery checkpoint: entry worktree clean, local HEAD=live
  origin main0411a680d22e18d17d300df5a4303ce94d27e9ba,VERSION1.0.1.027; delivery
  Git blob matches HEAD. Retention condition for140.23MiB history ZIP completed.
- PASS cleanup:275.66MiB /43files removed, comprising obsolete verified CMake
  test/benchmark executables, generated linker maps and the expired archive.
  Actual source target/cache metadata and per-file SHA256 establish ownership;
  all paths stay below the independent selected project root and build root.
- PASS preservation:3950unselected files bit-identical immediately after removal;
  formal EXE/INI hashes unchanged. .git and unknown files preserved. No active
  selected process/reparse escape. No directory purge or outside-project delete.
- PASS size: project381.02->106.24MiB (new audit metadata included); build347.96->
  73.18MiB,verify195.81->50.83MiB,obj135.70->5.91MiB. Current Release map and
  canonical verification binaries retained. Legacy EXEs without validated
  identity retained5.34MiB. No further new compiler artifacts were generated.
- Commands: git status/log/rev-parse/hash-object/ls-remote; CMake target/cache
  inspection; Get-FileHash and exact registered removal under a fixed project
  boundary. Results/manifests: build/verify/redundancy-final-*.json.
- NOT RUN: app build/native/Python/hardware/full suite/clean script rerun; none
  changed and binary equivalence establishes preservation. Selected verification
  is document/manifest/path/hash/size checking. No commit/push in this task.

## Verification artifact cleanup (2026-10-26, TST-01)

- Changed: scripts/verify_runtime.ps1,test_artifacts.ps1;
  tests/session_instance_tests.cpp,capture_tests.cpp,disposable_test_files.h,
  test_artifacts_tests.ps1; project AGENTS/HANDOFF/SELF_CHECK and shared bounded
  evidence policy/checklist. No product src/EXE/INI/.gitignore/version change.
- Why: old per-scope EXE copies, fake recordings and XLS exports were retained
  after tests. Inventory639.93MiB;99EXEs/45distinct,200.02MiB exact duplicates.
- PASS: PS mechanism14cases, native Release session_instance/telemetry_core and
  actual Release Charts runner with -SkipBuild. Tests exercise cleanup on success,
  failure and cancellation exception, source/INI preservation, duplicate evidence
  replacement, unknown/modified-file protection, path/junction/process guards.
  Existing native telemetry runner bundles some unchanged core checks; no extra
  suites/full GUI/Python/hardware run or app rebuild was selected.
- PASS:1196 exact test artifacts614,935,028B retired only after a recoverable archive
  validated all entry sizes/full SHA256;3483unselected files unchanged. Includes
  all259synthetic XLS and94old EXEs. Current verification EXEs/rollback remain.
  After140.23MiB archive, net446.22MiB of the retired data is reclaimed. Final
  verify195.81MiB; outside the archive55.58MiB. File-level plans/receipts in HANDOFF.
- Retention exception: one140.23MiB snapshot until current .027 source publication/
  recovery checkpoint is confirmed. Initial irreversible mass removal and later
  batch empty-directory removal were blocked by automatic approval review (only
  "blocked by policy"). The recoverable file retirement succeeded; empty-directory
  removal NOT RUN and no alternative was attempted.
- PASS: clean audit24scripts and canonical safe clean removing51compiler files;
  formal EXE remainsBB7F4ACF1FA08443329949477644BB28C0390C8FF8A2DB8F91A276224D89B08A;
  personal INI remains1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  No rebuild after clean. Git tracks zero verify files; archive/evidence ignored.
- Verification commands: tests/test_artifacts_tests.ps1; CTest -C Release -R
  ^session_instance$ / ^telemetry_core$; scripts/verify_runtime.ps1 -Configuration
  Release -Scope Charts -SkipBuild; workspace verify_clean_scripts.ps1 scoped to
  this project; scripts/clean_mfc.bat. Exact logs and archive manifest in HANDOFF.

## Analysis expansion (2026-10-26, 1.0.1.027)

- Changed: CMake/VERSION,plot_envelope/plot_view,text_telemetry and numeric bridge,
  replay_binary/binary_options/uart_offline,workspace_settings/profiles/preferences,
  derived_channels,binary_inspection/inspector,step_script/command panel/window,
  run_analysis/signal_view,serial_forwarder/workbench and native fixtures/core tests,
  verification scripts and engineering/product guides. See newest HANDOFF and
  docs/UART_ANALYSIS_GUIDE.md. Existing Python and unrelated changes preserved.
- Why: complete all ten approved general UART analysis/workspace features with
  independent source ownership, bounded data/worker lifetimes and explicit TX.
- PASS Release core6/6: uart_workbench,binary_frames,uart_commands,uart_advanced,
  uart_optimization,uart_analysis. After adding pre-index cumulative input limits,
  affected uart_advanced/uart_optimization/uart_analysis3/3 passed; final retention
  rerun passed. Source bytes remain unchanged, formulas are finite/same-frame,
  comparisons use no interpolation, forwarding stops on partial/error/generation.
- PASS native Debug Charts/Workbench/Advanced,Release Charts; final Debug/Release
  Font125 AnalysisExpansion,Debug Advanced after loader changes. Real controls and
  isolated files/fake transports cover dual RX, derived source qualification,
  parser/replay/profile transitions, scripts/CSV, byte highlight, XY/compare and
  teardown. Inspected final125%-font PNGs. No claim of physical DPI/mouse testing.
- PASS formal Release scoped app build/static CRT+MFC/system imports/map exclusion,
  version1.0.1.027,1,954,816B; retail startup/close/removed flags. INI hash unchanged.
  Formal Debug remains earlier delivery; separate Debug verification is current.
- PASS rendering microbenchmark median22.2145->12.1841ms,4curves x20k retained
  samples at identical bitmap/quality. Paired60s RX/recording twice per version:
  all byte/order/loss/memory/UI checks pass; normalized candidate CPU3950..4171ms
  per MiB vs4531..4668ms baseline. No regression observed in that simulated load.
  Evidence and hashes are in newest HANDOFF and build/verify performance receipts.
- NOT RUN: actual UART/board/wire timing,physical DPI/mouse/elevation,unrelated
  Python/full suite,Git publication. These are explicit coverage limits, not PASS.

## Test artifact ownership / retention (2026-10-26, TST-01)

- Shared policy and entry/checklist/handoff/README references updated under
  ../#READ_ME_FIRST#. Strict UTF-8/fences and local-link checks passed; no app
  rebuild was performed for those policy-only edits.
- Changed test: tests/uart_optimization_tests.cpp. Fresh owned directory, recorded
  recipe, ancestor reparse rejection, exact-file RAII plus checked success cleanup.
  PASS: normal paged replay/seek, cumulative rotation input limit, intentional
  exception unwinding and real cancelled replay cleanup. Final command: CTest
  Release -R ^uart_optimization$ -V; build/verify/retention-final-test.log.
- Each successful run removes74,240,000B plus13 small synthetic fixtures; only
  fixture-recipe.txt128B remains. User logs/INI/EXE are never wildcard targets.
- Historical bulk cleanup: exact source recipe and full SHA256
  AB0C6441DC17591FE7E564A9DACFB2FF7FDE559B49665DA7022768FDA9FDB7C5;
  no active test, no reparse escape, exact-file removal. Zero large.log payloads
  remain; final22-file receipt plus earlier3 verified removals total1,856,000,000B.
  Evidence: build/verify/test-payload-cleanup-result.json and project-size-audit.json.
  Delivery/personal INI hashes preserved during that cleanup.
- PASS: scripts/verify_delivery.ps1 now removes registered hash-identical EXE
  copies in finally after owned workers exit, with path/reparse/active-process
  checks and canonical preservation. Success removed5,864,448B; injected failure
  removed1,954,816B. Results/INI retained. Evidence: retention-delivery-success.log,
  retention-delivery-failure.log and delivery-failure-recipe.json in build/verify.
- PASS: four paired soak runs cleaned44,099,997B generated recordings and four
  copied EXEs; five more proven duplicate copies removed19,815,936B. Historical
  executables/unknown recordings were preserved, not inferred disposable by path.
- PASS: clean_mfc.bat removed1418 compiler files2,418,893,309B. SHA256 checks kept
  all4809 protected non-intermediate files identical, including formal EXE/personal
  INI; no rebuild after cleaning. Static clean audit PASS24 scripts. Evidence:
  pre-clean-protected-files.json,post-clean-result.json,retention-clean-audit.log.
- PASS: local disk4.601GiB ->0.804GiB; zero generated large.log remain. Git tracked
  bytes8.976MiB; verify/obj/Debug/legacy/INI are ignored and untracked. Evidence:
  project-size-after-clean.json. No commit or GitHub upload.

## Default-none channel choices (2026-10-25, 1.0.1.026)

- Changed: src/mfc_main_dialog.cpp,src/data_page.cpp/.h,VERSION,Release EXE;
  runtime_smoke/runtime_preferences/runtime_optimization fixtures;
  README/AGENTS/HANDOFF/SELF_CHECK. No ChannelModel, parser, UART worker, saved
  small-plot persistence or Python behavior change.
- Why: both checkbox panels must let the user choose fields instead of enabling
  new fields automatically. Chart now creates unchecked UI entries and syncs
  their states before plotting. The table also creates unchecked entries; the
  old first-four/custom-selection flag was removed. Known choices are preserved.
- PASS: Debug and Release `verify_runtime.ps1 -Scope Charts`: unchecked initial
  discovery/new fields/new capture, explicit one-field opt-in, hidden samples
  retained, table All/None/mapping/sparse export, independent labels/axes, saved
  small-plot choices/INI restore/Reset. Axis and preferences fixtures explicitly
  select their required fields instead of depending on the old defaults.
- PASS: Debug `-Scope Optimization`, including affected alias-search All/None
  preserving hidden user choices and table display preferences. Its existing
  bundled navigation/file-TX/fake-transport cases also passed; no extra suites.
- PASS: formal Release configure/build and actual-delivery verification:
  version1.0.1.026,three empty startup pages,normal close,no synthetic data,
  rejected demo/smoke flags. Size remains1,735,168B. Personal INI unchanged.
- PASS: source changes compared against this task's local baseline and scoped
  whitespace checks. Evidence/baseline/hash references are in the newest HANDOFF.
- NOT RUN: physical UART,manual cursor/real monitor DPI,Python,full historical
  suite,clean audit and Git publication; unrelated to selection defaults.
  Only formal Release was updated; isolated Debug/Release checks use1.0.1.026.

## UART workbench expansion (2026-10-25, 1.0.1.025)

- Outcome: all approved1..9 batches and independent stacked A/B groups are delivered.
  Product guide: docs/UART_WORKBENCH_GUIDE.md. Scope/acceptance:
  docs/UART_WORKBENCH_PLAN.md. Existing Python/pre-task features were preserved.
- Changed files: CMakeLists/VERSION/resources; uart_workbench*,workbench_session,
  traffic_store/framer,mfc_main_dialog_workbench and affected main serial/UART/
  keyboard/file/replay/profile/recording integration; binary_frame,command_panel,
  step_script,tx_pacing,block_protocol,transfer_job,modbus_rtu,terminal_screen,
  signal_analysis,uart_instruments,uart_command_window,uart_tool_form,UART queue/
  source mapping; native core/runtime tests,verify_runtime script,.gitignore,
  AGENTS/README/guide/plan/HANDOFF/SELF_CHECK.
- Why: add independent two-port monitoring and reusable UART analysis/operations
  while keeping raw recording, source clocks, transport ownership, private settings
  and preexisting features intact. New modules separate protocol, storage and UI.
- Builds PASS: formal Release/Debug,application target only with runtime checks OFF;
  optimized static MFC/CRT x64. Actual retail startup/close tests PASS. The final
  terminal shadow-name cleanup produced no compiler/linker warnings in final logs.
- Core PASS: selected Debug7/7 and Release7/7 groups: uart_operations,uart_workbench,
  binary_frames,uart_commands,block_protocol,uart_protocol_display,transfer_job.
  Final changed-core refresh PASS: Debug commands36checks and display/protocol107;
  Release display/protocol107. No All/Python/unrelated clean or historical replay.
- Native PASS: Debug/Release Workbench font125 each76PASS entries,0FAIL. Actual
  controls exercise independent connect/log/save/clear/follow, duplicate/busy opens,
  schema preview/apply/persistence/chart/generation boundaries, framed inspection,
  named Send, script Start/confirmation/results, TX exclusion/pacing/partial errors,
  queued file/peer transfer, RTU registers/charts, terminal/raw preservation, FFT,
  owned-window profile locks, B-only offline exclusion and recording-close errors.
  Reviewed dual-com,binary-parser,command-panel,instruments screenshots.
- Isolated dual-RX smoke:17,112B/port over about2s; both raw files exactly equal
  expected input and recorder loss0. Final Release/font125 wall2031ms,process CPU
  703ms. Do not interpret this as a long-duration or physical UART performance test.
- Retail PASS: hash-identical formal Release/Debug copies restore Messages/Chart/
  4Charts, stay free of fake rows/messages, handle recording-options controls/cancel
  and close normally. --demo and --smoke-test both exit2 before generating data.
- Size/import PASS: Release1,735,168B (~1.65MiB),+431,104B from1.0.1.024 for the
  added features. No claim of further slimming. Static runtime and size/security
  options retained; imports are Windows system DLLs only. No Ribbon/theme/CDialogEx
  or runtime fixture/feeder symbols in the retail map; no added install dependency.
- Protection PASS: private INI hash unchanged:
  CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33.
  Entry snapshot has no missing file. Formal EXE SHA256:
  6E1EF436238C1BBB95F73F72B7263F36DFDA10E92920C32A37813773BE86A00D.
  No user process was closed, private settings copied, or actual COM opened by tests.
- Reproduce relevant checks only: verify_runtime.ps1 -Configuration Debug/Release
  -Scope Workbench [-Font125]; targeted CTest names above; verify_delivery.ps1 for
  the affected retail startup path. Codex chooses/reuses evidence by change impact.
- Evidence: build/verify/workbench-baseline/20261025_297d2d67/completion.json and
  final-release-core.log,final-core-refresh.log,native-final-matrix.log,retail-final.log;
  retail fixture directories are recorded in HANDOFF. No Git publication in this task.
- NOT RUN: real UART/USB/driver/pins/wire timing, real monitor-DPI, long-duration
  device throughput or physical mouse/file-dialog acceptance. All device traffic
  was synthetic and confined to separately compiled verification code/fixtures.

## Remove production demo (2026-10-24, 1.0.1.024)

- Request / outcome: this is an everyday tool; remove the unnecessary demo and keep MFC Ribbon/theme support out of delivery. Formal native builds now have no synthetic feeder, demo state, demo Help instruction or demo Save As hint. `--demo` is rejected with exit 2 before session/UI/INI initialization. Real UART reception, explicit offline replay, charts and portable settings are unchanged. Ribbon/CMFCVisualManager had already been excluded in 1.0.1.023; this change re-verifies their absence rather than claiming to remove them twice.
- Changed files: CMakeLists/VERSION; `src/mfc_app.cpp`, `mfc_main_dialog.h/.cpp`, `mfc_main_dialog_filter.cpp`, `mfc_main_dialog_serial.cpp`, `mfc_main_dialog_export.cpp`, `help_dialog.cpp`, new `runtime_test_telemetry.cpp`; `scripts/verify_delivery.ps1`; AGENTS/README/MFC_BUILD/SELF_CHECK/HANDOFF and shared WORKSPACE_RULES. Synthetic input is moved unchanged into a guarded, check-enabled-only source so isolated automated tests still work without hardware. Its declarations/state/timer/retry suppression are under `CHART_RUNTIME_CHECKS`; no normal user session can enable the feeder. Python and unrelated existing work remain unchanged.
- Release PASS: canonical `scripts/build_release.bat`, x64/static MFC/CRT, zero reported warnings/errors, existing bundled native tests 6/6 (9.56 s). Debug PASS: `cmake --build build/obj/cmake --config Debug --target simple_com_chart_gui_mfc --parallel 4`; only the affected application target was rebuilt, not the unchanged Debug core suite. Full-path preflights found no running canonical output; no user process was closed.
- Actual delivery PASS: `scripts/verify_delivery.ps1 -Configuration Release` and `Debug`; hash-identical copies/fresh private INIs restore Messages/Chart/4Charts and remain free of fake rows/messages for at least 1.6 s, then close normally. Recording options modal/edit/checkbox/cancel still passes. Both `--demo` and production `--smoke-test` exit 2. Evidence: `build/verify/delivery-Release-afcea5d78ef643bda439b987367c55b9`, `delivery-Debug-d30ed6b49aa44334993c92e529dae2cf`.
- Focused check-enabled PASS: `verify_runtime.ps1 -Configuration Release -Scope Charts` covers relocated synthetic input and chart/capture behavior; `-Scope Serial` covers the affected retry guard plus fake open/reconnect/cancel. No All/Python/enlarged-font/Advanced/UartOperations/soak rerun was appended. Broader 1.0.1.023 unchanged-function evidence remains historical; its 60 s performance metrics are not presented as a new run for 1.0.1.024. No actual COM open/TX, real DPI or overnight/hardware acceptance was performed.
- Packaging/static PASS: Release **1,304,064 bytes (1.24 MiB)**, 7 KiB below 1.0.1.023. SHA-256 **27DC0686C743E44539FC9EA691AF96BD9CDCA8FC6DCA129E79185E881DB85DB6**; Debug SHA-256 `FD6152B0A1B52935EBCD47ACA82AB61BA926C812D6A5D16B5014E62C21BF076C`. Production project includes zero `runtime_*.cpp`; delivery map has zero `feed_demo@CMainDialog|CMFCRibbon|CMFCToolBar|CMFCVisualManager|CDialogEx` matches. Exact synthetic payload strings are absent from the delivery binary and the filter unit contains neither feeder nor demo sine generation. Native Help contains no demo instruction. No UPX/new dependency/dynamic MFC switch.
- Preservation: original 1.0.1.023 binary is retained at `build/verify/no-demo/before-1.0.1.023.exe`; personal Release INI remains SHA-256 **CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33**. No actual clean/purge, unrelated file removal, commit or push. Existing safe-clean/output-path contracts are unchanged; the previous 88-script static audit is reused, not mislabeled as newly executed.
- Final review PASS: all 17 changed text files decode strictly as UTF-8; revised verifier parses; tracked/new-file whitespace checks pass after removing one extra EOF blank line; current README/VERSION/delivery resource agree; final canonical EXE/INI hashes match the values above. No known unresolved high/medium issue in the selected change. The final whitespace/document-only edits did not repeat application suites. Remaining NOT RUN items are the unaffected physical device/DPI/desktop acceptance listed above, not a claim that fake transports prove hardware behavior.

## Native EXE size reduction (2026-10-24, 1.0.1.023)

- Request / before / after: shrink SimpleComChart while preserving user features, native x64, static MFC/CRT and portable INI. The former 1.0.1.022 EXE was 3,954,176 bytes and embedded UI/soak fixtures plus unused automatic Feature Pack registration. Delivery 1.0.1.023 is **1,311,232 bytes (1.25 MiB), 66.84% smaller**, without UPX or new runtime dependencies. Demo remains; only the developer `--smoke-test` entry moves to the independent check-enabled build (production exits 2). No source folder migration, Python change, staging or GitHub push.
- Changed files: `CMakeLists.txt`, `VERSION`, `src/mfc_app.cpp`, `mfc_main_dialog.h/.cpp`, `mfc_main_dialog_ui.cpp`, `mfc_main_dialog_export.cpp`, `mfc_main_dialog_recording.cpp`, `advanced_tools.h/.cpp`, `uart_console.h/.cpp`; `scripts/build.ps1`, `verify_runtime.ps1`, new `verify_delivery.ps1`; project AGENTS/README/MFC_BUILD/SELF_CHECK/HANDOFF and shared WORKSPACE_RULES portability guidance. Existing dirty feature work was retained. `CDialog` plus `_AFX_NO_MFC_CONTROLS_IN_DIALOGS` is safe only because no CMFC Feature Pack controls are used. The Win32 resource keyword DIALOGEX is unrelated and remains unchanged.

| Measured stage | EXE bytes |
|---|---:|
| Original 1.0.1.022, x64/unpacked | 3,954,176 |
| Exclude runtime UI/soak fixtures, previous speed flags | 3,653,632 |
| Also Release `/O1 /GL /Gw /LTCG /OPT:REF /OPT:ICF` | 3,432,960 |
| `CDialog` alone (still auto-registering Feature Pack controls) | 3,432,448 |
| Final: also omit unused automatic Feature Pack registration | **1,311,232** |

- Size evidence: `build/verify/exe-size/before-1.0.1.022.exe` retains original SHA-256 `78585A5898D316C079C088C9BE5E13A749E7D1421A7294FF62DA8D1E195B9FD4`; intermediate `checks-removed-speed.exe`, `checks-removed-size-ltcg.exe`, `before-dialog-base.map` and `final-manifest.xml` are retained. Link map `build/obj/cmake/SimpleComChart-Release.map` has zero matching `CMFCRibbon|CMFCToolBar|CMFCVisualManager|CDialogEx` symbol rows, versus 7711 before the dialog change (7713 after CDialog alone). The obj map is generated automatically for future inspections; do not ship maps/fixtures.
- Build PASS: canonical `scripts/build_release.bat` and `scripts/build.ps1 -Configuration Debug`; zero reported compiler/linker warnings/errors. Bundled native core targets passed 6/6 each (Release 10.56 s, Debug 14.83 s). This is the existing build entry's bundled coverage, not an appended Python/All UI suite. Exact-path preflights found no normal running EXE; no user process was closed/terminated.
- Actual delivery PASS: `scripts/verify_delivery.ps1 -Configuration Release` and `Debug`. Hash-identical EXE copies with fresh fixture INIs restore Messages/Chart/4Charts, expose all three tabs, render demo rows and Messages text, and exit normally. The actual Recording options modal creates its edit/checkbox, retains modal ownership and cancels safely without arming recording. Production `--smoke-test` exits 2. Canonical EXE/INI hashes remain unchanged by verification. Final evidence: `build/verify/delivery-Release-c8186c60aeac486f97896d1c7018a966`, `delivery-Debug-c91f4a848b0b432fbfed0dd9cc6b7832`.
- Check-enabled UI PASS: `verify_runtime.ps1 -Configuration Debug -Scope Startup -StartupTab Messages`; Release `-Scope Uart`, `Charts`, `UartOperations`, `Advanced`, `Optimization`. This selection covers changed startup/dialog bases, ordinary/custom controls, UI timer/demo and export completion hooks, the finite worker/close paths and shared optimized code. No All or enlarged-font matrix was appended. Each scope summary/report is under the corresponding `build/verify/<Configuration>-<scope>` directory. Final main / File TX / Console PNGs were inspected: neutral controls, tabs and content remain present, without obvious base-dialog paint regression. This does not claim physical-mouse or monitor-DPI acceptance.
- Release performance PASS: `verify_runtime.ps1 -Configuration Release -Scope Soak -SoakSeconds 60` selected because size-oriented code generation affects shared RX/rendering. Current check-enabled fixture ran 60109 ms: produced/RX/recorded **5,980,160 bytes**, six exact ordered recording segments, zero software raw/line loss, max UI-pump gap 296 ms, peak private memory 13,193,216 bytes. `build/verify/Release-soak/soak-result.txt` is the new result; old 300 s segments were preserved, not deleted or misattributed. This is an instrumented synthetic check, not a paired performance benchmark, physical UART throughput or an overnight run. Old 300 s metrics are historical, not re-run against these compiler flags.
- Packaging/static PASS: production vcxproj excludes every `runtime_*.cpp`; separate `SCC_ENABLE_RUNTIME_CHECKS=ON` build uses `build/obj/cmake-runtime` and `build/verify/runtime-bin/<Configuration>`. Final Release delivery SHA-256 **BBD5988D2782B7B152F04BEEC767C3F3C67F5960CC38A0F15B14DE868E31045C**; check-enabled Release is 1,546,240 bytes and is NOT the delivery artifact. Debug delivery SHA-256 `0A699C787A30BDEFB65E98F4D9FACD0A04CC67640A6F8A25FBA2B3AB558C3B58`. `dumpbin /dependents` lists only Windows system DLLs, no external MFC/CRT/Python/Qt runtime installer. Embedded manifest retains one legacy dpiAware, PerMonitorV2 and asInvoker. Personal release INI SHA-256 remains **CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33**.
- Build-flow clean audit PASS: shared `verify_clean_scripts.ps1` reports 88 scripts with no known whole-build deletion pattern. No actual clean/purge or material deletion was performed; binary baselines and previous test data remain local/ignored. Final validation PASS: strict UTF-8 in all 21 changed text files, parsing all three changed/new PowerShell scripts, scoped git whitespace check, current version/build-path/document consistency and native caption/resource contracts. New verification-script whitespace was checked independently because it is untracked. Final review has no known unresolved high/medium finding; documentation/output-message-only finishing edits did not require an unrelated rebuild/test rerun.
- Retained failed diagnostic: the generic shared `smoke_native_tool.ps1` timed out because it accepts only captioned `Afx*` frames, whereas this application creates an ordinary Dialog frame. It is not application startup evidence. Its fixture is retained at `C:/Users/averyc/AppData/Local/Temp/native-layout-smoke-f9c6d2f148144eca9335964d8ef9d4cd`; the helper closed only its own no-data copy. New `verify_delivery.ps1` matches the owned PID/versioned caption and verifies the actual delivery, both configurations PASS. The shared helper was not broadly rewritten.
- Remaining limits / NOT RUN: real COM/USB/driver behavior, real DPI switching, physical native file-dialog operation, full keyboard/foreground/elevation/session matrix and overnight testing. No size-only change removes the bridge-driver requirement. Reuse earlier unchanged protocol/model acceptance only as historical evidence; there is no claim every hardware combination was re-tested. All requested size changes and selected checks are complete; no known unresolved high/medium finding in this scope.

## UART optimization completed (2026-10-24, 1.0.1.022)

- Scope/outcome: user's approval to implement all recommendations after A/B1/B2/C. All eight groups in `docs/UART_OPTIMIZATION_PLAN.md` are implemented; `docs/UART_OPTIMIZATION_GUIDE.md` describes use and exact limits. Native MFC only. Existing dirty A/B1/B2/C work, Python, personal INI, other projects and remote mappings were preserved. No event/firmware-specific recorder trigger, implicit flashing/reset, network login, commit or push.
- Changed production families: `session_instance` / `mfc_app` / DPI manifest / `dpi_support` / `mfc_main_dialog_dpi`; advanced window viewport/responsive main/data/channel layout; `bounded_regex` / `line_filter` / manual scalar `log_parser` / `field_extractor` / extraction composition; `display_preferences` / channel model/panels/table/end labels / display composition; source offset/count/sequence in `source_mapping`, capture/pending/receive/replay, message and main navigation; plot navigation/fixed-Y/cursor snapshot and finite-extreme axis handling; `replay_storage` / streamed `uart_offline` / background replay composition; `light_terminal`, Messages options/keyboard/local echo and main terminal composition; `file_transfer` / file-TX composition / operation/automation/close gating. Updated CMake, resources, VERSION and canonical EXE. Runtime/core fixtures, selector script and documentation align. No external dependency or clean implementation change.
- PASS build: canonical `scripts/build_release.bat` and `scripts/build.ps1 -Configuration Debug` (NoProfile/Bypass PowerShell). Final logs: `build/verify/optimization-build-release.txt` / `optimization-build-debug.txt`. No reported compiler warning/error. CTest bundled `telemetry_core`, `uart_receive`, `uart_operations`, `uart_advanced`, `uart_optimization`, `session_instance`: 6/6 each; final 8.23 / 10.22 s. Temporary /MP4 environment restored. These are bundled build targets, not a rerun of All/Python application suites.
- PASS new core: NFA capture offsets/adversarial nested repetition/work budget/backreference rejection/invalid-apply preservation; CSV quotes/Tab/JSON numeric-only/captured name-value-time/portable parser; display-only projection/schema; split RX offsets/oversize line resynchronization; CR/BS/CSI K and 256 KiB short-line terminal eviction; >64 MiB and >200000 records with paged random access/time seek; numeric rotation order, boundary-spanning byte search, ordinary uppercase numbered binary; exact binary NUL/non-ASCII and validated text/newline chunking. Tests generate only private local files under `build/verify`.
- PASS instance helper: simultaneous/renamed copies yield one owner, readiness/activation preserves main/modal data and busy state, exit/relaunch, exact-image test-owned crash with a kept mutex object and abandoned-owner recovery, readiness timeout and guard failure fail closed. These are private helper namespaces, not production data-bearing sessions. Stable production identity is path/version/config independent; legacy binaries without this protocol cannot be retrofitted.
- PASS focused final UI: `verify_runtime.ps1 -Configuration Debug -Font150 -Scope Optimization`, Release `-Font125 -Scope Optimization`, Release `-Scope Advanced`. Evidence: `build/verify/Debug-font150-optimization`, `Release-font125-optimization`, `Release-advanced`. Actual controls cover preview vs applied CSV, display-only table/independent chart values, searchable aliases/hidden checkbox preservation, exact table/RX navigation including LF-only boundaries, zoom/fixed-Y/latest, persistent state, owned file/paste confirmations, pause-before-send, exact file TX/pacing/partial stop, Ctrl+C byte 03 (not Break), confirmation generation changes and disconnect. Seven pages fit horizontal bounds at 800/1050 width and 620 height with reachable scrolled actions/status. Actual main PMv2 context passes. Inspected final enlarged-font main/Extraction/File TX/Automation PNGs; baud/parity/All/None captions are not truncated, and scrolled fields do not overpaint fixed tabs. Enlarged fonts are NOT physical monitor-DPI evidence.
- PASS affected regressions: Debug `-Scope Charts` after source-clock/extreme-axis fix; Debug `-Scope Serial` after final measured-combo layout; Debug `-Scope Uart` and `-Scope UartOperations` after shared receive/queue/keyboard/file integration. These scopes passed; later changes outside their tested paths reuse that evidence. Release Advanced covers final background replay/profile/snapshot/automation and continuous cursor dragging. No full All/Python suite, no real COM open/TX.
- PASS sustained receive: `verify_runtime.ps1 -Configuration Debug -Scope Soak` (60 s), Release `-Scope Soak -SoakSeconds 300`. Independent producer, fake serial worker, actual UI timer/history/capture and real local recording/rotation writer. Debug: 6,045,696 produced/RX/recorded bytes, 6 segments, zero pending byte/line loss, max UI-pump gap 532 ms, peak private 18,612,224 bytes. Release: 300047 ms, 30,224,384 produced/RX/recorded bytes, 29 ordered segments, matching stream hash, zero byte/line loss, 108064 rows expired normally, max UI-pump gap 63 ms, peak private 15,679,488 bytes (7,491,584 after warmup), one-core-equivalent CPU 23.0953%. Reports/PNG: `Debug-soak` and `Release-soak`. CPU includes fake producer/hash/recorder; this is not a hardware UART/USB capacity guarantee or overnight soak. Configurable test duration is 60..86400 s, not mandatory on unrelated changes.
- Reused soak evidence precisely: Release 300 s fixture SHA-256 `ADE30DC718E2E117691087AB377AB34D957B5A3FC87E4E7D0BC53DECDAC08B3B`. The final subsequent change only bounds optional extracted field timestamps and subtracts their origin before addition; this path is inactive in the Key/value/no-time-field soak. Receive/render/storage/recording implementation is unchanged. Final core and Optimization/Advanced scopes were rerun for that guard; the 300 s soak is not falsely labeled rerun against the final binary hash.
- Failures retained/fixed: initial fixed-point projection test incorrectly used exact double equality (`optimization-initial-display-test-failure.txt`); duplicate legacy DPI node caused side-by-side startup rejection (`optimization-embedded-manifest.xml`, `optimization-manifest-failure-022.exe`); initial 60 s run kept all raw bytes but lost numeric rows/stalled UI (`optimization-soak-before-performance-fix`), fixed non-regex scalar parsing/latest-value copies/hidden refresh and bounded fixture dispatch; scrolled tab overpaint and All/None clipping preserved as `optimization-before-scroll-viewport.png` / `optimization-before-panel-button-width.png`; chart fixture's virtual source clock/epsilon exposed future-label exclusion (`optimization-before-virtual-source-clock-fix.txt`), fixed by respecting newest model timestamps. Final selected evidence passes, not these historical failures.
- PASS review/delivery: fixed source-range overlap, duplicate-timestamp row selection, cursor drag reset/live-vs-snapshot mixing, unsafe initial invalid-filter widening, terminal quadratic eviction, finite extreme-axis overflow/unbounded ticks, out-of-range fixed-Y labels and time-field magnitude/precision. No known unresolved high/medium finding in this scope. New implementation is split by responsibility; no .cpp exceeds 800 lines. Static workspace clean audit: 88 PASS; no actual purge/clean or deletion of delivery/evidence. Intermediates/verify/personal INI remain ignored and tracked canonical EXE remains eligible. Final embedded manifest has exactly one legacy dpiAware plus PerMonitorV2; dumpbin dependencies are Windows system DLLs, no MFC/VC/Python runtime installer. Final selected evidence directories contain 29 result files with zero FAIL. Strict UTF-8 validation passed for 126 cumulative changed text files, documentation entry points exist and git diff whitespace check passes; existing A/B1/B2/C changes are included in that cumulative count, not newly attributed to this batch.
- Output: `build/SimpleComChart.exe`, 3,954,176 bytes, FileVersion/ProductVersion `1.0.1.022`, SHA-256 `78585A5898D316C079C088C9BE5E13A749E7D1421A7294FF62DA8D1E195B9FD4`. Entry/final personal INI SHA-256 `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`, unchanged. Exact-path canonical Release/Debug preflights found no production instance to close; simulated-crash termination targeted only private helper processes. No staged changes or GitHub publication.
- NOT RUN / limits: physical UART/USB/pins/Break/flow/target effects/throughput; real monitor DPI switches, physical mouse/clipboard/native file dialogs; Windows foreground/elevation/cross-session restrictions; overnight soak; cancellation of a stalled filesystem/driver. The application retains unfinished worker ownership instead of blocking normal UI joins or pretending cancellation undoes side effects. Full VT emulation is not included; raw view/export/recording and long-retention analysis retain their documented distinctions. Final document-only finishing checks do not rebuild the app.

## Remaining UART plan B2/C completed (2026-10-24, 1.0.1.021)

- Requested/before/after: user asks to continue until the accepted UART plan is complete. A/B1 already existed as dirty, uncommitted work and were preserved. Implemented the remaining offline replay, named profiles, retained-capture chart analysis and explicitly enabled finite automation. `docs/UART_DEBUG_PLAN.md` has no remaining unchecked software feature item. Excluded firmware/event-specific recording triggers, network login and implicit target reset remain excluded. The Python frontend is unchanged.
- Changed production files: new `uart_files.h/.cpp` (bounded regular-file reads, CREATE_NEW staging/atomic file saves and strict conversion), `background_job.h` (cancel/poll/finished-worker ownership), `uart_offline.h/.cpp` (recording-index validation and virtual clock), `uart_profiles.h/.cpp` plus `uart_profile_ini.cpp` (fixed-schema profiles and atomic merged INI), `uart_automation.h/.cpp` (finite scheduler/checksums), `chart_analysis.h/.cpp` (sparse snapshot statistics), `advanced_tools.h/.cpp` (separate four-page UI), `plot_view_analysis.cpp`, and `mfc_main_dialog_advanced/replay/profiles/automation.cpp` (composition). Updated main dialog declarations/message/menu/timer/close paths, serial receive/settings integration, manual Console gating/completions, recording source guard, Messages source labels/clock suppression, queue per-ID cancellation, opt-in plot annotations/absolute ticks, and first-zero-time sample handling. Updated resources, VERSION/CMake, `.gitignore` and canonical EXE. No external dependency or cleanup-script implementation change.
- Changed verification/docs: new `tests/uart_advanced_tests.cpp`, `src/runtime_advanced_tools.cpp`; scoped selectors in `runtime_smoke.cpp`/`scripts/verify_runtime.ps1` (`Advanced`, `Charts`). Updated AGENTS/README/MFC_BUILD/UART_DEBUG_PLAN/HANDOFF/SELF_CHECK; added the user guide `docs/ADVANCED_UART_TOOLS.md`. Existing A/B1 source changes remain their own earlier work; the cumulative Git diff is not a B2/C-only patch.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`. Final logs `build/verify/advanced-build-debug.txt` / `advanced-build-release.txt` have no reported compiler warning/error. Bundled CTest `telemetry_core`, `uart_receive`, `uart_operations`, `uart_advanced`: 4/4 each, final elapsed 0.34 / 0.32 s. Temporary `/MP4` compiler environment restored afterward. Bundled old core targets were not an appended full application/Python suite.
- PASS focused UI: `scripts/verify_runtime.ps1 -Configuration Debug -Scope Advanced` and `... -Configuration Release -Font125 -Scope Advanced` via NoProfile/Bypass PowerShell. Final copies test the completed code after the profile-input-lock/close correction. Evidence: `build/verify/Debug-advanced`, `Release-font125-advanced`; 3 reports each. Actual buttons cover background paused Load, Play, Seek/new view, raw Messages/parser/capture integration, synthetic-clock export suppression and virtual chart clock, Unload, named Save/Load staging, cancelable Apply, atomic preferences/empty-port baud retention, main-window close availability during settings lock, analysis A/B/reference rendering, actual mouse-message cursor movement, finite worker TX, generation change during confirmation, raw RX auto reply and disconnect queue cancellation.
- PASS UI geometry/visual: four pages at widths 1050/1400 and minimum window height 900, Debug normal / Release enlarged UI fonts. Every expected field/button is visible with matching selected/focused tab; control containment and measured label/button padding pass. Inspected final Release enlarged-font Automation, Profile preview and analysis PNGs, plus Debug images. Native child/sibling clipping and complete redraw prevent the initially observed missing/overpainted fields. Preserved original screenshots/results under `build/verify/advanced-before-tab-focus-fix`; geometry-only PASS was insufficient and is not treated as initial visual acceptance. Font125 is not a physical monitor-DPI switch.
- PASS core: exact plain/indexed replay bytes and receipt/synthetic timing, pause/speed/seek/end, malformed/out-of-bounds/incomplete index and size/non-file/cancel rejection; strict profile roundtrip/schema/duplicate/unknown/version/device-name/open-path/INI-newline validation, preservation of unrelated INI keys and original destination on cancel/read-only/collision; sparse inclusive min/max/mean/RMS/population SD/cursor deltas/empty intervals and scaled finite-large RMS; Sum8/XOR8/CRC16-Modbus known vectors/ending validation; bounded batch/reply splitting/early response/interval/timeout retry exhaustion, raw auto-reply trigger, partial failure stop, finite count/disconnect no-resume, queue per-ID cancellation and finished-thread polling. Uses generated fixture files only, no COM hardware.
- PASS affected regressions: Debug `-Scope Serial` (shared serial settings/close/timer), `-Scope UartOperations` (queue/manual gating/completion), `-Scope Uart` (Messages receive/clock/export/recording integration), `-Scope Charts` (shared channel/time and default live plot Y/data-page paths). Each was run once after its relevant behavior change. These unchanged-path results were reused after the final profile-only UI-lock and advanced-layout correction, not falsely marked rerun. Current selected directories contain 3 + 3 + 3 + 3 + 7 + 3 = 22 report files, zero FAIL. Full `All`, unrelated Python and physical-hardware suites NOT RUN.
- Earlier compile correction: missing `capture_store.h` include in the new view header caused the first Debug build to fail; preserved `build/verify/advanced-build-debug-initial-header-failure.txt`, added the include, then builds passed. Review also fixed offline live-Unload safety, oversized-line resynchronization, INI text injection, no-port profile baud retention, confirmation generation checks, dialog-return lifetime checks and settings-only input locking with responsive close/cancel. Final targeted review found no known unaddressed high/medium issue in this implemented scope; no unrelated startup/single-instance retrofit was claimed.
- PASS safe-clean/ignore/text: workspace read-only `#READ_ME_FIRST#/scripts/verify_clean_scripts.ps1` reports 88 scripts with no known whole-build deletion pattern. No actual clean/purge or new preservation fixture; earlier allowlist-clean behavior is unchanged. `.sccprofile`, raw `.bin`/`.idx` and portable profiles are ignored; delivery EXE remains eligible/tracked. Final tracked/untracked text/UTF-8/reference/whitespace/version checks cover docs-only finishing edits without another app rebuild.
- Output: `build/SimpleComChart.exe`, FileVersion/ProductVersion `1.0.1.021`, SHA-256 `083D48B9E4716B0A18A6A21BB28509FCA9C0CAC432D5144B9B0F7F5856DC215A`. Entry/final personal INI SHA-256 `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`, unchanged. Exact-path process preflight found no running canonical Release/Debug instance; none closed/terminated. All writes/tests use isolated fixtures; no commit/staging/GitHub push, no private recording/profile publication.
- Limits / NOT RUN: real COM TX/pins/Break/USB/timing/throughput, physical mouse/menu/file-dialog acceptance, actual DPI and cancellation of a stalled remote filesystem/driver. Replay is bounded best-effort scheduling, not bit-accurate timing; synthetic clock labels stay off, retention/seek rules are in the guide. Profiles intentionally omit commands/history and preserve unrelated INI keys. Analysis is a refreshable retained snapshot; out-of-range Y references are not drawn. Automation literal bytes can match echoes, are not protocol ACKs, use explicitly configured timeout retries only, and cannot revoke already-issued hardware effects. Deferred shutdown retains worker lifetime rather than release storage early.

## UART operations batch B1 (2026-10-24, 1.0.1.020)

- Scope/why: continue the approved UART plan with explicit Text/HEX TX/history/presets, flow/lines/Break and statistics. Keep extra controls in a separate modeless Console and preserve receive/capture/recording behavior. This is NOT completion of offline replay/profiles or chart analysis/automation; see `docs/UART_DEBUG_PLAN.md`. B1 resumes the dirty batch A tree; earlier receive-display changes are preserved, not newly attributed to B1.
- Changed production files: new `src/uart_operations.h/.cpp` (payload validation, generation-bound bounded queue, statistics), `uart_preferences.h/.cpp` (validated portable controls/history/presets), `uart_console.h/.cpp` (owned modeless controls/layout), `mfc_main_dialog_uart.cpp` (request/confirmation/completion composition), `serial_control_config.h/.cpp` (DCB mapping), `serial_manager_operations.cpp` and `serial_native_io.h/.cpp` (driver operations/overlapped lifetime). Affected `serial_manager.h/.cpp`, `serial_connection.h/.cpp`, `serial_open_job.cpp`, `mfc_main_dialog.h/.cpp`, `mfc_main_dialog_serial.cpp`, `mfc_main_dialog_serial_settings.cpp`, `uart_recorder.h/.cpp`, resources, CMake and VERSION. Rebuilt `build/SimpleComChart.exe`; no Python source, dependency or cleanup-script change.
- Changed verification/docs: new `tests/uart_operation_tests.cpp`, `tests/uart_native_io_tests.cpp`, `src/runtime_uart_operations.cpp`; affected `tests/uart_receive_tests.cpp`, `src/runtime_serial.cpp`, `runtime_uart_receive.cpp`, `runtime_smoke.cpp`, `scripts/verify_runtime.ps1`; AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK/UART_DEBUG_PLAN. New `-Scope UartOperations` runs Console/operation cases only. Shared RX/stop integration changes justify affected Serial/Uart regression scopes, not All/Python.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`. Final build logs `build/verify/uart-operations-build-debug.txt` and `uart-operations-build-release.txt` show no reported compiler warning/error. Bundled CTest `telemetry_core`, `uart_receive`, `uart_operations`: 3/3 per configuration, final elapsed 0.29 / 0.27 s. Temporary `/MP4` compilation setting was restored; no build-policy/script change. The existing telemetry target is bundled coverage, not an appended full application runtime suite.
- PASS focused RUN commands: `scripts/verify_runtime.ps1 -Configuration Debug -Scope UartOperations`; `... -Configuration Release -Font125 -Scope UartOperations`; affected Debug `-Scope Serial` and `-Scope Uart`, all run via PowerShell with NoProfile/Bypass. Final evidence directories: `build/verify/Debug-uart-operations`, `Release-font125-uart-operations`, `Debug-serial`, `Debug-uart`. The 3 + 3 + 3 + 7 current result files contain zero FAIL. Serial covers layout/state plus fake persistence/recovery; Uart covers existing receive/display/recording/export/font integration. Unchanged prior chart/Python/startup-option evidence is reused, not newly run.
- PASS core: exact UTF-8/HEX/endings, malformed/full-size validation, history/preset Unicode/quotes/invalid schema, unavailable/truncated preferences, grouped control persistence, explicit DCB flags/handshake ownership, full controls forwarded through the open worker and retained in recovery model, queue limits/generation/cancel/late outcome, statistics/rate expiry and RX-only raw recording with separated TX text/byte totals. Actual overlapped helper tests use PRIVATE LOCAL NAMED PIPES: binary read/write, concurrent RX during pending TX, cancellation completion and pre-cancel no-write. These are not UART-driver/hardware tests.
- PASS actual controls/fake integration: Send exactly once on a worker, explicit suffix and driver-counted accepted bytes, no RX/capture fabrication, preset Save/Load never transmits, invalid HEX visible, live-control/Break confirmation owner/flags, successful control persistence, canceled confirmation, connection generation changing during the modal loop, partial-write no-retry, blocked TX while RX continues, disconnect returns without joining blocked I/O, queued cancellation/no replay, final bytes survive an RX error. Console hide-only close, handshake checkbox ownership, measured caption/padding/containment at widths 1000/1400 pass. Inspected `Release-font125-uart-operations/uart-console.png`; Font125 is enlarged UI fonts, NOT real monitor-DPI switching.
- Verification corrections: fixed compile-time mixed LONG/int layout arithmetic. Early serial/receive runtime failures came from old fixture assumptions of synchronous stop; preserved whole earlier directories in `uart-020-before-async-fixture-serial` / `uart-020-before-async-fixture-receive`. Updated fixtures to wait/pump async shutdown and reran only affected scopes, now PASS. Final scans exclude those deliberately preserved failed baselines.
- PASS static clean audit: `powershell -NoProfile -ExecutionPolicy Bypass -File D:\SourceCode\python\#READ_ME_FIRST#\scripts\verify_clean_scripts.ps1` reports 88 clean-related scripts with no known whole-build deletion pattern. This read-only audit follows the CMake/test-flow addition; it is not an actual cleanup or a new deletion-preservation fixture run. PASS final changed-text/reference/whitespace review; no application rebuild for the documentation-only finishing edits.
- NOT RUN: physical COM open/TX/flow/DTR/RTS/Break/USB recovery/timing/throughput, physical-mouse confirmation/menu interaction and actual monitor-DPI switching. Side-band changes can reset or alter target boot state; accepted driver bytes do not prove delivery/ACK. A stalled driver can prolong Stopping; request/event/transport lifetime is preserved rather than released early. No full All/Python suite, actual clean/purge, commit or GitHub push. Builds/tests use isolated fixture INIs and no real COM.
- Delivery: canonical FileVersion/ProductVersion `1.0.1.020`, SHA-256 `3D1510500DF4666D5DE68A6FCCFAE094FADC2949387322074FE371152E8BB986`. Exact-path preflight found no running production Release/Debug instance, so none was closed. Personal INI entry/final hash `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33` unchanged; older A hash below is historical. Final docs-only changes require text/reference/whitespace checks, not another application rebuild.

## UART receive batch A (2026-10-24, 1.0.1.019)

- Scope/why: implement the first receive/recording batch of the user's approved UART comparison, excluding event-specific triggers. Full accepted scope and incomplete B/C items are in `docs/UART_DEBUG_PLAN.md`; this is not completion of TX/Flow Control/replay/profiles/chart analysis/automation.
- Changed production files: `src/message_format.h/.cpp` (bounded decoding/receipt formatting/search), `message_buffer.h/.cpp` (receipt boundaries survive pending/history transfer), `message_page.h/.cpp` and `message_page_options.cpp` (display options, system RichEdit for wrap/colors/highlights, search/bookmarks/shortcuts/persistence), `uart_recorder.h/.cpp` (one bounded disk worker, independent raw/text/index files, rotation/error/stop/drain), `mfc_main_dialog_recording.cpp` (menus/options/status/close), `mfc_main_dialog.h/.cpp`, `mfc_main_dialog_serial.cpp`, `mfc_main_dialog_filter.cpp` (composition, pre-UI receive recording, pending transfer, persistence, shortcut and close routing), `SimpleComChart.rc`, `resource.h`, `VERSION`, `CMakeLists.txt`. Canonical `build/SimpleComChart.exe` rebuilt. No Python source or cleanup-script changes.
- Changed verification/docs: `tests/uart_receive_tests.cpp`, `src/runtime_uart_receive.cpp`, `src/runtime_smoke.cpp`, `scripts/verify_runtime.ps1`; AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK and UART_DEBUG_PLAN. The added `-Scope Uart` selects changed/affected receive/display/export/font/fake-serial paths. `-Scope UartDisplay` selects just the new display controls, useful for fixture-only geometry/screenshot corrections.
- PASS build: canonical `scripts/build_release.bat` and `scripts/build.ps1 -Configuration Debug`, using temporary compiler `/MP4` for build parallelism only (no build-script/config-policy change). Bundled CTest `telemetry_core` plus new `uart_receive`, 2/2 in both configurations. Build logs: `build/verify/uart-build-debug.txt`, `uart-build-release.txt`; no reported compiler warning/error in the final builds. The existing telemetry target is unavoidable bundled core coverage, not an appended full application suite.
- PASS focused runtime: Debug `-Scope Uart`; Release `-Font125 -Scope Uart`; Debug and Release `-Scope Startup -StartupTab Messages` for existing menu/default routing after the UART menu addition. Subsequent fixture screenshot/summary-only changes use Release `-Font125 -Scope UartDisplay`, not a full-suite rerun. Evidence is under `build/verify/Debug-uart`, `Release-font125-uart`, `Release-font125-uart-display`, `Debug-startup-Messages`, `Release-startup-Messages`. All current selected result reports contain no FAIL. Later error-path Unicode-context changes pass the core recorder tests; unchanged UI/serial success-path evidence is reused, not relabeled as a new hardware run.
- PASS core cases: legacy ASCII/split CRLF/HEX offsets, receipt timestamps and transfer, UTF-8/DBCS split decoding, invalid bytes, code page 950, SGR spans, explicit delimiters, literal/case/Regex alternatives, invalid/pathological-group rejection, async start/duplicate prevention/stop drain, exact binary RX versus synthetic metadata, size/date rotation, existing-file preservation, missing/Unicode path reporting and queue overflow auto-stop.
- PASS actual controls/integration: UTF-8 display and UTF-8 export bytes, selection/follow-off, bookmarks/append/clear, real RichEdit match-background and SGR foreground, pause still receiving then catches up, options INI restoration, search toolbar bounds at 1060/1450 widths with normal/enlarged UI fonts; selected original raw Messages, export and font behavior; injected serial recovery. Fake-serial worker records >256 KiB before UI flush while Messages is hidden, with rejecting filter/Clear, and saves exact bytes beyond UI history capacity. Normal close defers to recording drain without destroying the window early.
- PASS visual review: inspected normal and enlarged-font screenshots. The first enlarged-font image exposed a TEST fixture's fixed page Y covering tabs; preserved the prior PNG at `build/verify/uart-before-fixture-isolation/uart-display-font125-before.png`, then changed only fixture positioning/restored real parent layout and inspected `Release-font125-uart-display/uart-display.png`. An earlier test-sequencing failure (Find left follow-off for old tests) is preserved in `uart-before-fixture-isolation`; fixture state now restores, affected checks pass. These are not claimed production flicker/hardware reproductions.
- PASS safety/process: workspace static audit `#READ_ME_FIRST#/scripts/verify_clean_scripts.ps1` reports 85 scripts with no known whole-build deletion pattern. No actual clean/purge or unrelated project/application tests. Full runtime All, Python suites, physical COM open/unplug/timing, real DPI switching, power-loss/disk-full hardware tests and physical-mouse file-dialog interaction are NOT RUN. Font125 means enlarged UI fonts, not a real monitor-DPI change.
- Entry preflight found no running exact canonical Release/Debug target EXE; no process needed closure/termination. Tests use isolated copies/fixture files only, no real COM open or TX. Personal `build/SimpleComChart.ini` SHA-256 remains `43FD93E3D2C8FB4D4829BADDBA5BC649428FD3B185582F7E2C1DE6F1F88C8E9F`. No commit or GitHub push.

## Configurable startup page (2026-10-22, 1.0.1.018)

- Requested/before/after: startup was hardwired to Chart. Settings > Startup tab now offers Messages, Chart and 4 Charts with one native radio check. Only an explicit menu choice writes `[UI] StartupTab` using stable named tokens; ordinary tab changes do not. Confirmed changes apply next launch without changing the current page or capture. Missing/invalid values fall back to Chart; save failure retains the previous confirmed choice and displays an INI error. Apply the chosen page/focus/visibility before first show. No automatic device connection.
- Changed files: new `src/mfc_main_dialog_preferences.cpp`; `mfc_main_dialog.cpp/.h` startup/command integration; `resource.h`/`SimpleComChart.rc` menu IDs/resources; `help_dialog.cpp`; new `runtime_startup_tab.cpp`; `runtime_smoke.cpp`, CMakeLists, `scripts/verify_runtime.ps1` selector/fixture setup, VERSION and AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK. No new dependency, output-folder/clean implementation, parser/export/Python behavior change or unrelated worktree reversal.
- Impact/scope: only preference persistence, actual startup lifecycle and matching page/menu/control routing changed. Selected `-Scope Startup` with each initial page in Debug and Release, because MFC startup changed; did not run Serial/full All/Python/chart/export/font suites, all font variants or clean audits. Existing untouched coverage is reused, not reported as a fresh run. Full All entry was adjusted for a user-selected startup page and compiles, but full execution is not claimed.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`, no reported compiler warning/error. Their existing entries bundle telemetry_core 1/1, final elapsed 0.11 / 0.15 s; that incidental unit target is distinct from the selected Startup verification.
- PASS focused RUN: for Debug and Release, `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1 -Configuration <config> -Scope Startup -StartupTab <Messages|Chart|4Charts>`. Six separate actual MFC launches; final three-case totals Debug 9.6 s / Release 10.9 s. All 12 current `*result.txt` files in `build/verify/<config>-startup-<choice>` have no FAIL. Initial page/focus/visibility/control state matches the independently seeded INI/expected CLI choice, not a page selected manually after startup. No real COM open.
- PASS assertions: exactly one matching native menu radio; real command routing saves each named token immediately; production loader restores each page; choosing a default preserves current live page/rows/raw bytes; ordinary TCN_SELCHANGE changes pages without changing the default; missing/invalid/99/-1 values fall back to Chart. Test-owned new INIs have UTF-16 BOM and preserve unrelated sentinel keys. Read-only failure preserves the prior choice/radio/value and gives a visible error, then original attributes restore. Capture/raw data remain unchanged. The script seeds only ignored fixture INIs, never the personal delivery INI.
- PASS evidence review: Release Messages and Debug 4 Charts `startup-page.png` inspected; Settings/Help menu and correct initial active page/control states are visible. Per-case `startup-tab-result.txt`, `smoke-result.txt`, main fixture INI and `startup-tab-test.ini` retained. NOT RUN: physical menu mouse traversal or real monitor-DPI switching; not needed to claim untested hardware behavior. No firmware/device/other-app action.
- Preflight normally closed only exact canonical EXE PID 88924 (created 2026-10-22 18:04:48), no force/unrelated closure. Personal INI SHA-256 remains `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F`. Final `build/SimpleComChart.exe` FileVersion/ProductVersion 1.0.1.018, 3271680 bytes, SHA-256 `D27F47D96550A709F01F2F9AE67A1E46FFB234DB8158F99BB78B5E3BE895D47C`. No file purge/commit/push. Final edited-text/reference/whitespace and selector syntax review applies without another app-test rerun for documentation edits.

## Manual Connect warning and troubleshooting (2026-10-22, 1.0.1.017)

- Request/before/after: user sees Connect appear inactive when Tera Term is already open. Before, the asynchronous failure only updated bottom status/log. Now a failed manual attempt displays one owned `Connection failed` MessageBox with PORT, exact error and actionable checks; background retries remain status/log only. Guidance says the port may be occupied/unavailable, suggests disconnecting the same PORT in Tera Term/PuTTY/another tool, checking USB/Device Manager and serial settings, then retrying; it never claims actual process ownership or closes other applications.
- Changed files: `src/mfc_main_dialog_serial.cpp` central manual failure routing and candidate release before prompt; `mfc_main_dialog.h` declaration and injectable native MessageBox dispatcher; `serial_manager.cpp` retains Win32 open error number and optional system description; `mfc_main_dialog.cpp` guards UI work after the modal loop can close its owner. Tests: `runtime_serial.cpp` and `runtime_serial_fixture.h` error injection/notice assertions. VERSION and AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK plus shared modal-error rule aligned. No parser, export, Python, new dependency or unrelated worktree reversal.
- Impact selection: manual failure/validation, idle-state restoration, cancelled pending open, automatic retry suppression and native UI startup/teardown callsite are affected. Used existing `-Scope Serial` only, including its bundled connection-layout guard. Did not rerun full All runtime, all font variants, Python/chart/export/font suites or clean/audit; existing unchanged evidence remains historical, not a new PASS. Current one-click build entries still bundle the single telemetry_core unit target and that incidental execution is reported separately.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`, no reported warning/error. Bundled CTest telemetry_core 1/1 in both configurations, final elapsed 0.26 / 0.21 s.
- PASS focused RUN: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1 -Configuration Debug -Scope Serial` (6.8 s) and same with Release (7.0 s). Six current result files under `build/verify/Debug-serial` / `Release-serial` contain no FAIL. New assertions intercept warning dispatch and verify owner/main HWND, exact title/OK-warning flags, restored idle controls before warning, COM3 plus simulated `Open failed (5): Access is denied.` and Tera Term/Device Manager/check/retry text, one warning on manual failure and none on later polls/success/background retry/cancelled completion. Missing port, invalid settings and a pending cancelled driver call explain why Connect cannot start. Successful retry and last-successful INI preservation still pass; capture/layout/recovery checks remain in the same scoped fixture.
- Evidence inspected: `serial-connection-result.txt`, scope-specific `smoke-result.txt` and `manual-connection-warning.txt` (exact simulated ASCII message, binary-written to preserve CRLF). Test hook restores the real MessageBox function after the fixture; no real COM open. NOT RUN: actual native popup rendering/physical OK click, closing the owner while the modal loop runs, real Tera Term occupancy, USB/driver failures or actual Windows system-error translation from such an open. The modal teardown guard is code-reviewed but not advertised as a physical teardown test. No hardware/other-app action was performed to simulate occupancy.
- Preflight gracefully closed only canonical target PID 71784 (full path `D:\SourceCode\python\simple_com_chart_gui\build\SimpleComChart.exe`, created 2026-10-22 17:47:14); no forced/unrelated closure. Personal INI SHA-256 remains `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F`. Final EXE 1.0.1.017, 3257344 bytes, SHA-256 `24360CB7CB55E5C7070858ED6B12C26542F7D28297E7FAFBE447E773ABE511B6`. No clean/purge/commit/push. Final edited-document/diff checks apply without another app-test rerun.

## Connect state-only redraw (2026-10-22, 1.0.1.016)

- Requested/current comparison: pressing Connect reportedly creates visual noise/repainting. Before: every connection-state update recalculates current-caption width and relayouts all pages using immediate-repainting MoveWindow calls. After: normal layout reserves the widest Connect/Cancel/Disconnect caption; serial-row COM width accounts for actual fixed widths/gaps; state refresh changes only the required caption/enabled/button-redraw properties, leaving all child geometry unchanged. No serial/persistence/capture/parser/Python behavior redesign.
- Changed files: `src/mfc_main_dialog_ui.cpp` (measured stable width/row budget), `src/mfc_main_dialog_serial.cpp` (remove whole-dialog layout; conditional text/enabled updates; button-only no-erase invalidation), `src/runtime_connection_redraw.cpp` (new focused probes/matrix), `src/mfc_main_dialog.h`, `src/runtime_smoke.cpp`, CMakeLists, `scripts/verify_runtime.ps1` (Serial selector/isolated evidence), VERSION. AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK and shared state-only UI guidance aligned; unrelated existing changes preserved.
- Before-fix evidence: Debug `scripts/verify_runtime.ps1 -Configuration Debug -Scope Serial` exited 1 as expected after instrumenting the old layout path. Each width/page cell recorded 144 unrelated WM_WINDOWPOSCHANGING events over 12 state updates and failed stable-geometry checks; fake serial functional cases still completed. Preserved the fixture at `build/verify/connection-redraw-before-1.0.1.016/Debug-serial` before applying the fix. This is a verified code/layout defect, not a physical reproduction of the user's unprovided screenshot/video.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`. No reported compiler warning/error. Their existing build entry also bundles CTest telemetry_core 1/1, final elapsed 0.12 / 0.16 s; this bundled unit target was not presented as focused UI coverage. No separate broad suite was appended.
- PASS focused RUN: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1 -Configuration Debug -Scope Serial` (4.8 s) and `... -Configuration Release -Font125 -Scope Serial` (5.2 s). Debug is required for the changed MFC layout path; the Release enlarged-font case validates caption reservation/row budget in the affected geometry, without rerunning all four variants. Both check all three pages, widths 1120/1500, three repetitions of opening/connected/waiting/idle: 72 updates per run, fixed top-level rectangles, all captions fully fitting, correct parameter/Scan enabled states, Stop field within the client and zero unrelated position events in every cell. Fake serial INI/reconnect/renumber/cancel/late-result regressions also PASS. All six current result files in `Debug-serial` and `Release-font125-serial` contain no FAIL; excluded intentional failed baseline from that final scan.
- PASS evidence review: Release Font125 Chart and Debug Messages opening PNGs inspected. Evidence: `build/verify/<configuration>[-font125]-serial/connection-redraw-result.txt`, `connection-opening-page-0/1/2.png`, `serial-connection-result.txt` and the scope-specific `smoke-result.txt`. PrintWindow captures settle/force rendering; they do not alone prove absence of transient physical flicker. NOT RUN: actual bridge connection/noise, physical USB actions, real monitor-DPI change. No real COM open by fixtures.
- Scope exclusions/reuse: full All runtime, Python tests, chart/filter/export/font suites and clean/audit were not rerun for this localized state/layout change. No cleanup/build-flow implementation changed and no publication requested. Their unaffected 1.0.1.015 evidence is reused, not marked newly executed. Current test-selector All path compiles but its full execution is not claimed.
- Output: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.016, 3247616 bytes, SHA-256 `86EE087702BBA5300A5C28299E299D4D2D7159A9C5D72013F9BB9D36A1F35315`. Personal INI SHA-256 `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F` unchanged from entry (already different from the older 1.0.1.015 record). No matching target instance needed closure, no file purge, no GitHub push. PASS final text/reference review, PowerShell selector syntax and changed-text whitespace checks; no application tests repeated for the final documentation edits.

## PORT persistence and bridge recovery (2026-10-22, 1.0.1.015)

- Changed/why: user requested remembered PORT and automatic bridge reconnection. New `src/serial_connection.h/.cpp`, `serial_open_job.h/.cpp`, `mfc_main_dialog_serial.cpp`, `mfc_main_dialog_serial_settings.cpp`; adapted `serial_manager.h/.cpp`, main-dialog `.h/.cpp` and filter/settings integration. Config persists only successful serial choices; startup restores without opening. Stable instance/interface matching prevents a reused COM number from impersonating the bridge. Recovery preserves charts/capture/Messages; manual cancel and dialog teardown override late background results. Backoff/status/logging are bounded/non-modal; no firmware commands or replay. Existing manual Connect/new-capture behavior retained.
- Added `src/runtime_serial.cpp`, `runtime_serial_fixture.h`, `tests/serial_connection_tests.cpp`; updated runtime smoke, preference/font-fixture restore, native test entry, CMake, Help and VERSION. Project AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK and shared recovery rules aligned. Python implementation unchanged; previous dirty-tree work/user JPGs preserved.
- PASS BLD/core (performed before the later targeted-testing policy request): `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`, telemetry_core 1/1 each, no reported compiler warning/error. Core verifies validation/identity ambiguity/reused port/renumbering, no startup auto-open, bounded 2/4/8/10-second backoff, manual cancellation, single pending worker, late-success disposal and owner-close safety. PASS Python 5/5 (`python -B -m unittest discover -s tests -v`); not a future requirement for an unrelated MFC-only edit.
- PASS RUN: `scripts/verify_runtime.ps1 -Configuration Debug` / Release, each normal and `-Font125`; final times 13.5 / 13.4 / 13.8 / 16.7 seconds respectively, within the unchanged 30-second harness bound. All 52 current `*result.txt` reports contain no FAIL. The actual connection commands/INI loader run through fake transport/scanner: COM4 selection/230400-7-EVEN-2 restoration with zero startup opens, failed choices retaining prior config, immediate save on success, enumerated interface use, quiet stream/failed scan, read loss and queued-tail drain, wrong bridge reusing COM4 rejected, busy retry, original bridge returning as COM9, preserved history/new reader appending, PnP loss, manual cancel, blocked second worker and cancelled late success. INI paths/malformed config are rejected safely. Only ignored test-owned INIs are written by fixtures, not personal INI.
- PASS VISUAL: Debug `serial-reconnect-waiting.png` (Cancel/retained raw bytes) and Release Font125 `serial-reconnected.png` (COM9, readable status, Disconnect, continued messages) inspected. Evidence: `build/verify/<configuration>[-font125]/serial-connection-result.txt` and the two screenshots. NOT RUN: actual COM open, physical USB unplug/reinsert, driver/identity/DTR/RTS/target effects, manual close/relaunch and real monitor-DPI switching. Font125 is only an enlarged-control-font fixture; simulated PASS is not hardware acceptance.
- PASS CLEAN: actual `scripts/clean_mfc.bat` through workspace `verify_clean_preservation.ps1 -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`; 176 regenerable compiler files removed and all 749 protected build files SHA-256-identical. Expanded 23-sentinel/source/help/repeat/cache/junction tests PASS; static audit 85 scripts PASS. Canonical EXE/INI/data/logs/evidence retained. Preflight graceful full-path closure PID 83724 only, no force/unrelated termination. Personal INI SHA-256 remains `52117B8466BEB99BA29174CADB2E49C299F16FD5914AB808965445B5AF9AA12C`.
- Delivery: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.015, 3237888 bytes, SHA-256 `A7862D7199893D7B2E864A8A5838C6B8E36B213434700746C532153A911EE03B`. Local/uncommitted; no push. EXE and personal INI hashes rechecked after actual clean, identical to the recorded values.
- Documentation-only follow-up: user's new shared policy limits testing to modified/affected behavior; old local unconditional four-variant instructions have been aligned. PASS: edited-text/reference/consistency review, policy references in all five shared documents, shared-document whitespace and `git -c core.safecrlf=false diff --check`. No compile/runtime/clean rerun for this follow-up. Prior results above remain actual earlier evidence, not new runs. Future checks map to the change; expanded/full-suite coverage needs an explicit reason, and unrelated checklist entries are out of scope, not FAIL.

## Messages row alignment correction (2026-10-22, 1.0.1.014)

- Scope/why: user's screenshot shows Font/point text and +/- controls not aligned with the rest of the row. Before: static labels drew at the top, buttons centered text in a larger requested height, and the native combo used its own closed-field height. Width-only tests in 1.0.1.013 did not detect this. After: all eight controls use the actual closed combo selection-field top/bottom/center; single-line Display/Font/point statics are vertically centered. Keep dropdown extent separate from visible row geometry. Font/INI/data/export behavior unchanged; no dependencies or unrelated changes added.
- Changed code: `src/message_page.cpp` (three static styles and row layout), `src/runtime_message_font.cpp` (complete alignment assertions/screenshot), VERSION. Updated AGENTS/README/docs/MFC_BUILD/HANDOFF/SELF_CHECK and shared workspace UI baseline/handoff. All pre-existing local modifications/user JPGs preserved.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`; native CTest telemetry_core 1/1 each, no reported compiler warning/error. PASS Python: `python -B -m unittest discover -s tests -v`, 5/5. PASS whitespace: `git -c core.safecrlf=false diff --check`.
- PASS final RUN: unchanged `scripts/verify_runtime.ps1 -Configuration Debug` / Release, each also `-Font125`; all four finish PASS within their standard 30 s bound. Script-inclusive elapsed times: Debug 19.6 s, Debug Font125 16.9 s, Release 14.8 s, Release Font125 15.2 s. `message-font-result.txt` verifies all eight controls' exact shared top/bottom/center and page containment, static-label centering, closed-field (not popup) height at widths 1120/1500 and body sizes 8/10/32 pt. Existing font size, INI persistence/restore, scroll, data/export, filter/chart/UI checks remain PASS; final current result scan has no FAIL.
- PASS VISUAL: Debug normal and Release Font125 `messages-toolbar-aligned.png` inspected, as well as the current Debug ASCII view. The visible row now shares one alignment; normal and enlarged-control-font renderings preserve compact neutral controls. Font125 is not physical monitor-DPI switching. Unsupported-extension status in fixture screenshots remains intentional residue from a negative export test, not an alignment failure.
- Initial verification gap: two Debug standard-harness timeouts at 30 s, at different progressing stages before the new alignment checks completed. Preserved snapshots under `build/verify/messages-row-timeout-1.0.1.014` / `attempt2`; some unreached files are old retained evidence, not fresh results. Identical Debug EXE SHA-256 `6D2082303FEDAC85AC6F39306328E0A468CA9F44C3FD725E6625BE07A97B80AF` passed a one-off bounded 60 s diagnostic in 21.6 s, then the four final standard runs passed without source/deadline changes. Timeout cause NOT confirmed; do not claim a timing root-cause fix. Old 1.0.1.013 row image retained in `build/verify/messages-row-before-1.0.1.014`. Physical mouse interaction, COM/timing and actual monitor-DPI switching NOT RUN.
- PASS CLEAN: `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`; removed 160 regenerable compiler intermediates, all 685 protected build files SHA-256-identical. Expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks PASS; static `verify_clean_scripts.ps1` audit 85 scripts PASS. EXE/INI/log/data/evidence preserved.
- Preflight gracefully closed only exact canonical EXE PID 85740, no force/unrelated closure. User INI SHA-256 `52117B8466BEB99BA29174CADB2E49C299F16FD5914AB808965445B5AF9AA12C` already at entry, unchanged after close/build/clean (not restored to older settings). Final `build/SimpleComChart.exe` FileVersion/ProductVersion 1.0.1.014, 3182080 bytes, SHA-256 `74F6BBF1DD0717084C9323AE9647A873CC793CD088296A1EF5A647FF7643B798`. Local/uncommitted; no GitHub push.

## Messages font -/+ and INI (2026-10-22, 1.0.1.013)

- Scope/why: user asked for quick message font-size controls with persistent settings. Added Font - / current pt / + inside Messages, 1 pt per click, 8..32 pt, missing-setting default 10 pt. ASCII/HEX share one size; only body text changes, preserving raw/history/export data and toolbar/chart fonts. Persist `[Messages] FontSizePt` immediately on successful user changes, load through the existing startup settings path, clamp invalid/out-of-range values. Preserve inspected row where possible with follow off and newest data with follow on. No new dependency or Python/frontend/parser/export-format change; previous work/user JPGs untouched.
- Changed files: `src/message_page.cpp/.h` (controls, point-size replacement, owned-font lifetime, scroll/layout); `src/mfc_main_dialog.cpp/.h` (font-change message routing and test declaration); `src/mfc_main_dialog_filter.cpp` (portable preference read/write); new `src/runtime_message_font.cpp` plus `runtime_smoke.cpp`/CMake integration; `src/help_dialog.cpp`, VERSION, AGENTS, README, docs/MFC_BUILD, HANDOFF and this document. Existing native UI/export changes were preserved, not reimplemented.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`, native CTest telemetry_core 1/1 each, no reported compiler warning/error. PASS Python: `python -B -m unittest discover -s tests -v`, 5/5. PASS whitespace: `git -c core.safecrlf=false diff --check`.
- PASS RUN: `scripts/verify_runtime.ps1 -Configuration Debug` / Release, each also `-Font125`, all four final variants finish PASS. `message-font-result.txt` verifies real native button notifications/actual font heights, 1 pt changes, immediate INI writes and real loader restoration, missing/invalid/1000 pt settings, limits/button disabling, unrelated INI keys, ASCII/HEX font sharing, hidden-page restore, follow off/on, data/log-content isolation, repeated replacement without GDI-object accumulation, measured non-overlapping buttons at widths 1120/1500. Final current `*result.txt` scan contains no FAIL; existing chart/filter/data/settings/Save As/UI tests remain PASS.
- PASS VISUAL: inspected Debug `messages-font-large-ascii.png` / `messages-font-large-hex.png` and both Release Font125 equivalents. Complete Font +/- readout/control row, green-on-black 24 pt content and unchanged surrounding UI. HEX lines wider than the viewport retain horizontal scrolling; no automatic line-wrap change. Unsupported-export-extension status in these fixtures is expected negative-test residue. Font125 enlarges the shared control font, not an actual monitor-DPI switch; explicit body point size is independent.
- Evidence: `build/verify/<configuration>[-font125]/message-font-result.txt`, `message-font-test.ini`, `messages-font-large-ascii.png`, `messages-font-large-hex.png`. INI restoration uses the actual application loader in-process; manual physical button clicks / separate close-and-relaunch acceptance NOT RUN. Physical COM/timing and actual monitor-DPI switching NOT RUN; no hardware action was taken for this font-only change.
- PASS CLEAN: `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`; 160 regenerable compiler files removed, 604 protected build files SHA-256-identical. Expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks PASS. Shared static `verify_clean_scripts.ps1` audit 85 scripts PASS. EXE/INI/log/data/evidence kept.
- Build preflight normally closed only exact canonical target PID 81716; no force/unrelated closure. Canonical personal INI unchanged before/after close/build/clean: `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447`. Final EXE `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.013, 3178496 bytes, SHA-256 `C179B37988D9414A5414590F572C53CDC0FB62AD035F099D9D4BE3ADD40BEB31`, verified after clean. Local/uncommitted, no GitHub push.

## Messages text Save As (2026-10-22, 1.0.1.012)

- Scope/why: user approved independent Messages text export after asking why Save As was disabled. Preserved all earlier work, including the 1.0.1.011 tooltip changes and user tab JPGs. Messages now saves its current ASCII/HEX retained history to `.log` / `.txt` without requiring numeric/filtered capture. Chart formats and Python frontend unchanged. Default filename remains PC-local `yyyyMMdd_HHmmss.log`; empty history reports no data. Exports are snapshots after confirmation, not continuous recording, binary export, screen-only selection or recovery of dropped bytes.
- Changed sources: `src/message_page.cpp/.h` provides the current formatted text snapshot; `src/mfc_main_dialog.cpp`, `mfc_main_dialog.h`, `mfc_main_dialog_ui.cpp`, `mfc_main_dialog_export.cpp` route by tab, enable Save As on Messages and share worker/busy/close/completion handling. `src/capture_export.cpp/.h` factors the existing atomic writer and adds verbatim `.log`/`.txt` output; no header/BOM or extra trailing newline. `src/help_dialog.cpp` describes the new behavior. New `src/runtime_message_export.cpp`; updated `runtime_messages.cpp`, `runtime_ui_help.cpp`, `runtime_smoke.cpp`, `tests/capture_tests.cpp`, CMake, VERSION, AGENTS/README/MFC_BUILD/HANDOFF and this document. No new dependencies.
- PASS BLD: final `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`, native CTest telemetry_core 1/1 each. An initial wchar-to-char narrowing warning in formatting was removed with explicit conversion of the guaranteed 7-bit display text; final builds have no reported warning/error. Core tests retain all five chart export formats and check Messages exact ASCII escapes/CRLF/partial-line/no-BOM, HEX text, uppercase `.LOG`, rejected CSV preserving an existing file, locked destination and replacement.
- PASS RUN: final `scripts/verify_runtime.ps1 -Configuration Debug` / Release, each also `-Font125`, all four finish PASS. `message-export-result.txt` checks empty history, filtered capture empty, ASCII/HEX snapshots, no fabricated chart rows, later receive/mode/Clear isolation, concurrent-save rejection, disabled Save As after all tab changes while busy, close guard, success/failure button restore, locked destination preservation/replacement, extension rejection and retained HEX offsets. Final `*result.txt` scan in the four current evidence directories contains no FAIL. Existing filter/sort/Y-axis/data/preferences/tooltips/chart export regressions stay PASS.
- PASS other: `python -B -m unittest discover -s tests -v`, 5/5; `git -c core.safecrlf=false diff --check`. Inspected Release Font125 `messages-green.png` with enabled Save As and ASCII/HEX saved artifacts; the visible unsupported-extension status is intentional negative-test residue. Evidence lives under `build/verify/<configuration>[-font125]`, including `messages-saved-ascii.log`, `messages-saved-hex.txt`, `messages-locked.log` and `message-export-result.txt`.
- NOT RUN: actual native file-dialog mouse interaction and physical cursor-dwell acceptance for the four toolbar buttons, because the computer-use skill helper cannot connect its native pipe (os error 2), including recovery retry/reset. No UI automation fallback. Physical COM timing/throughput and real monitor-DPI switching also NOT RUN; Font125 is only enlarged-font regression.
- PASS clean: `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`; final actual clean removed 158 regenerable compiler files, all 572 protected build files SHA-256-identical. Expanded 23-sentinel/source/help/repeat/bytecode/native-and-cache-junction checks PASS; `verify_clean_scripts.ps1` static audit 85 scripts PASS. Delivery files, INI, logs and test evidence retained.
- Delivery after clean: `build/SimpleComChart.exe` FileVersion/ProductVersion 1.0.1.012, 3165184 bytes, SHA-256 `8B000164800CD7A0804AEC2215F38C86E373B532518825FAC9543D4FF71EE682`. User INI unchanged at `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447`. No further matching output process after the earlier exact-path graceful PID 72408 close; no force/unrelated closure. No Git commit/push.

## Button hover follow-up (2026-10-22, 1.0.1.011)

- Scope/files: `src/hover_help.cpp/.h` now registers actual child HWND tools with native automatic subclass routing plus separate parent-area fallbacks; `mfc_main_dialog.h` and `mfc_main_dialog_help.cpp` remove the custom parent-rewriting PreTranslateMessage relay. `runtime_ui_help.cpp` checks 20 registrations for 10 logical targets, native/fallback flags, resized bounds, texts/delays, mouse-message command/capture preservation and deterministic popup cleanup. Updated VERSION, project AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK and shared workspace lifecycle/handoff rules. Preserved all earlier local changes and user tab JPGs. No new dependency, parser/export/Python behavior change.
- Reason: user saw label/filter help but no four-button help; parent-only tools did not represent the actual enabled BUTTON windows under the pointer. Actual child registration avoids that event/hit-window mismatch; parent rectangles remain for parent-delivered transparent-label/disabled-child input. Forced popup screenshots alone were insufficient acceptance in 1.0.1.010.
- PASS BLD: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and `scripts/build_release.bat`; native CTest telemetry_core 1/1 each. PASS Python: `python -B -m unittest discover -s tests -v`, 5/5. PASS whitespace: `git -c core.safecrlf=false diff --check`.
- PASS RUN: `scripts/verify_runtime.ps1 -Configuration Debug` and Release, each also `-Font125`, all four finish PASS. Inspected `ui-help-result.txt` and other final report files for FAIL (none). Inspected Debug Font125 `auto-refresh-tooltip.png`/`tabs-selected.png` and Release Font125 `auto-refresh-tooltip.png`/`messages-green.png`; complete readable popup text and existing page styling preserved. These are deterministic popup/native-contract checks, not cursor-dwell acceptance.
- NOT RUN: actual pointer hover over the four buttons. The computer-use skill helper failed at list_apps with native pipe unavailable / system cannot find file / os error 2, including retry and reset/reinitialization. Stopped UI automation, no PowerShell UIA fallback. Physical COM/timing/throughput and actual monitor-DPI switching also NOT RUN. Normal and enlarged-font native fixtures are not hardware evidence.
- PASS clean: `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`; 156 compiler intermediates removed, 516 protected build files unchanged by SHA-256. Expanded 23-sentinel/source/help/repeat/cache/junction checks PASS. Shared `verify_clean_scripts.ps1`: 85 scripts PASS. EXE, INI, logs, exports and test evidence retained.
- Preflight gracefully closed only full-path canonical target PID 72408, without force. Personal INI SHA-256 at entry/after close/build/clean: `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447`. Final `build/SimpleComChart.exe` 1.0.1.011, 3141632 bytes, SHA-256 `A242170A6F07D20CD82D75F03742E4886C598F51CF8E9B7CBB4D5E2782AA0801`. Local/uncommitted, no GitHub push.
- Messages Save As question: source confirms intentional disable/routing guards in `mfc_main_dialog_ui.cpp`, `mfc_main_dialog.cpp` and `mfc_main_dialog_export.cpp`. Current export snapshot is filtered capture, not the independent raw message buffer. Answered the reason; adding raw ASCII/HEX export awaits the user's choice and was NOT implemented or claimed tested here.

## Tab order, green Messages and hover help (2026-10-22, 1.0.1.010)

- Incremental scope: preserved the existing uncommitted 1.0.1.009 work atop Git `75a8694`. New `hover_help.cpp/.h` owns native tooltip registration/geometry/child-to-parent mouse relay; `mfc_main_dialog_help.cpp` defines explanations, measured owner-draw tabs and the PreTranslateMessage hook; `runtime_ui_help.cpp` checks routing/colors/help. Updated `mfc_main_dialog.h/.cpp`, UI/filter/export integration, `message_page.cpp`, existing runtime tests, CMake, VERSION, Help and local documentation. No parser/data format/INI preference/Python frontend change or new dependency.
- Behavior: Messages / Chart / 4 Charts order; startup Chart; named IDs in all page-sensitive command/export paths. Dark-grey/white selected tab, neutral inactive tabs. Readonly message EDIT responds to both WM_CTLCOLORSTATIC and WM_CTLCOLOREDIT with green text/stock black background, shared by ASCII/HEX. Only the permanent Include/Exclude paragraphs were removed; applied/error status remains visible. Ten tooltip targets cover button explanations, Auto Refresh and Include/Exclude labels/fields. Mouse relay uses parent-client rectangles, including disabled children, and rectangles follow layout. Hover does not trigger application commands or clear capture.
- Fixed during verification: latest-size TOOLINFO registered no tools on the current control activation, although the tooltip window itself existed. Verified the installed MFC source's old-prefix strategy and switched registration/query/hit-test structures to TTTOOLINFOW_V1_SIZE. Native registration/text/geometry/popup checks then PASS; no global manifest change. Failed report/images retained under `build/verify/tooltip-registration-before-1.0.1.010`. Initial immediate Font125 popup capture was blank; tooltip-local animation/fading is now disabled and the fixture forces repaint before capture. Final normal/enlarged popup images are readable; do not infer successful pixels from PrintWindow's return alone.
- Reference for native relay/rectangle/tool lifetime semantics: [Microsoft tooltip controls](https://learn.microsoft.com/en-us/windows/win32/controls/tooltip-controls), [TOOLINFO](https://learn.microsoft.com/en-us/windows/win32/api/commctrl/ns-commctrl-tttoolinfow). Shared WORKSPACE_RULES now requires compatible structure sizes and checking successful registration/data/popup behavior, not only window creation.
- PASS BLD: final `scripts/build.ps1 -Configuration Debug`, canonical `scripts/build_release.bat` Release x64 and CTest telemetry_core 1/1 each. Final builds report no warnings/errors. `python -B -m unittest discover -s tests -v` 5/5 and patch whitespace PASS. Affected-change review found no outstanding confirmed high/medium finding; the historical 1.0.1.009 isolated repaint observation remains documented below, not claimed fixed.
- PASS RUN/UI: all four final `scripts/verify_runtime.ps1` variants (Debug/Release, normal/Font125). Startup asserts Chart selected before any test switches pages. `ui-help-result.txt` verifies exact tab order, real TCN_SELCHANGE routing/visibility, Save As/Chart control gating, dark selected fill/white text versus inactive grey, and black/green readonly EDIT color handling. Ten tooltip targets pass native text/hit tests after resizing to 1120/1500; general layout also covers 1180 and all three pages. Button explanations, short Regex-pipe Include/Exclude examples and Auto Refresh's timed-clear/not-receive-frequency meaning are asserted. A child mouse message passes through the actual PreTranslateMessage relay without executing commands or resetting capture. Native tracking popup is rendered/hidden/deleted without moving the cursor; delay 600 ms and no fade/animation are checked. Existing filter/capture/sort/axes/latest-label/persistence/export checks remain PASS.
- PASS VISUAL: Debug and Release Font125 `messages-green.png`, `tabs-selected.png`, `auto-refresh-tooltip.png` inspected; green-on-black content, prominent active tab, no permanent Include/Exclude paragraphs, readable complete popup text. Evidence is in `build/verify/<configuration>[-font125]`. Font125 is an enlarged-font fixture, not a physical monitor-DPI switch; popup visibility/rendering is forced within the test rather than a physical cursor dwell.
- PASS CLEAN: shared `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches` ran the actual clean. Removed 156 reproducible allowlisted compiler files; all 504 protected build files unchanged by SHA-256. Expanded 23-sentinel/source/help/repeat/native-and-cache-junction/bytecode checks PASS. Static workspace clean audit: 85 scripts PASS. EXE/INI/log/data/unknowns/evidence retained.
- Build preflight normally closed exact canonical Release EXE PID 28784; no force termination/unrelated closure. Later checks found no matching output EXEs. This turn's user INI hash was already `59A6CCC8D9F1BC1C4CF7808C810D0757F3E5A4BA88F04267A944CF9DA9ADA7B7` at entry and remains identical after closure/build/clean; did not restore the older 1.0.1.009 hash.
- Final delivery after clean: `build/SimpleComChart.exe` FileVersion/ProductVersion 1.0.1.010, 3141120 bytes, SHA-256 `38C507D779D333047B05690740893239E0735A14E07CE50EDAEE45549E892B0A`. Physical COM/timing/throughput, actual monitor-DPI switching and physical mouse dwell are NOT RUN. No Git commit/push; all earlier local changes preserved.

## Messages, compact/persisted charts and neutral toolbar (2026-10-22, 1.0.1.009)

- Baseline: clean user-committed `75a8694`; user requests accumulated in one revision. No unrelated edits reverted, new dependencies or Python frontend changes. No commit/push performed.
- Changed receive/display files: `serial_manager.cpp/.h` returns exact raw bytes independently of existing LF/CRLF line parsing; `mfc_main_dialog.cpp/.h` bounds/drains the pending queue and integrates the third page; `mfc_main_dialog_ui.cpp` adds Messages/4 Charts and page-specific control states. New `message_buffer.cpp/.h` and `message_page.cpp/.h` implement bounded ASCII/HEX monitoring and controls. New `data_page_table.cpp` plus `data_page.cpp/.h` implement left-aligned measured/sample-bounded widths, explicit none defaults, exact remembered fields and Reset.
- Changed settings/export/style files: `mfc_main_dialog_filter.cpp` persists four names in `[FourCharts]`, preserving Unicode/quoted names via `key:` prefixes and rejecting invalid saved controls; Apply/Connect retain choices, but reset samples. `capture_filename.h` and `mfc_main_dialog_export.cpp` suggest local `yyyyMMdd_HHmmss.log`, keep user-editable filename/format/overwrite behavior, and leave filtered Save As disabled on raw Messages. New `button_feedback.h` plus main dialog drawing/command dispatch implement neutral grey faces, normal-weight text, straight borders and a single temporary red accent; only connected Connect stays red.
- Changed verification/docs: `tests/message_tests.cpp`, `tests/telemetry_tests.cpp`, `runtime_messages.cpp`, `runtime_preferences.cpp`, `runtime_buttons.cpp`, `runtime_capture.h` and `runtime_smoke.cpp`; CMake includes these modules; VERSION, Help, AGENTS/README/MFC_BUILD/HANDOFF/SELF_CHECK synchronized. Runtime budget increased from 15 to a bounded 30 s because expanded three-page repaint/screenshot tests exceeded 15 s (initial timeout reached sorting, not a startup assertion).
- Fixed during verification: an ASCII/HEX monitor follow toggle initially failed to scroll because WM_SETREDRAW(FALSE) temporarily removed edit visibility. Re-enable redraw before EM_SCROLLCARET; the failing report and context images are retained under `build/verify/messages-before-scroll-fix-1.0.1.009`. Later normal/enlarged-font raw follow tests pass. Remembered fields were also explicitly constrained to capture-accepted keys so persistence cannot bypass Max CH.
- PASS BLD: final `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug` and canonical `scripts/build_release.bat`; CTest telemetry_core 1/1 in each configuration. No reported final compiler warnings/errors. `python -B -m unittest discover -s tests -v` 5/5. Patch whitespace check PASS. Affected-code review found no confirmed outstanding high/medium source defect; the isolated repaint uncertainty below remains explicitly open, not silently marked fixed.
- PASS RUN: `scripts/verify_runtime.ps1 -Configuration Debug` / `Release`, each also with `-Font125` (four final variants). `messages-result.txt` verifies raw NUL/FF/CRLF, no-newline and hidden/minimized updates, filter independence, mode history and incremental HEX rows, follow on/off, Clear/capture separation, queued-byte disconnect retention, 256 KiB burst bounds/input gaps and font lifetime. `chart-preferences-result.txt` verifies explicit none defaults/no auto assignment, actual selector-triggered INI writes, exact/Unicode/quoted-name restore, late matching discovery, Max CH, Apply preservation, persisted Reset, measured left-aligned widths and zero-padded local timestamp filenames. INI restore exercises the actual startup load routine; COM reconnect is not exercised against hardware.
- PASS UI: `button-theme-result.txt` checks the actual owner-draw handler's background pixels: UI-grey idle/completed, single red pressed/executing, grey disabled override, connected palette red, preserved Snapshot/Overlay actions and safe feedback destruction. Debug and Release Font125 `button-theme.png`, Messages ASCII/HEX and compact-table images inspected. Layout tests cover 1120/1180/1500 window widths on all three pages with repeated repaint/live-font checks. Font125 is enlarged text, not actual monitor-DPI switching.
- Verification uncertainty: one Release Font125 run showed missing whole-window painting and stale latest-label assertions, not failed model/axis/settings/button logic. Preserved `data-display-result.txt`, failed screenshots and final smoke report under `build/verify/release-font125-before-repaint-fix-1.0.1.009`. Same unchanged EXE then passed that exact runtime variant three consecutive times. Root cause NOT confirmed; directory naming does not imply a root-cause fix. No speculative drawing change was made. All four current report sets finish PASS; monitor this if reproduced.
- PASS CLEAN: final shared `verify_clean_preservation.ps1 -ProjectRoot <project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches` executed the real clean. Removed 150 reproducible allowlisted compiler files; all 436 protected build files unchanged by SHA-256. 23 protected sentinels plus source, non-mutating help, repeat clean, root/source bytecode removal and native/cache junction rejection PASS. Static workspace `verify_clean_scripts.ps1`: 85 scripts PASS. Earlier base clean removed 148 compiler files, which were regenerated for the later theme change; no runtime folders were purged.
- Build preflight gracefully closed exact canonical Release EXE PID 65124 under standing authorization, without force or unrelated closure; later checks found no matching output process. Canonical user INI retains pre-build SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- Final delivery after clean: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.009, 3124736 bytes, SHA-256 `A51D4F1260EE0F76B30CEBCA2A3A57F4BB6144C333D88E3E90353106A4DD46CD`. Physical COM byte delivery/timing/connection-color transitions, sustained hardware throughput and actual monitor-DPI acceptance NOT RUN. Messages are in-memory only; Save As remains filtered capture export. No commit/push.

## Channel header sorting (2026-10-21, 1.0.1.008)

- Baseline: clean worktree at `17b3e8e`. Changed `src/channel_panel.cpp/.h` to handle Channel-header clicks, toggle natural case-insensitive ascending/descending order, and display A-Z/Z-A in the header. Native sort flags alone were not visibly clear in the first screenshot; explicit text was added before final verification. Both panels retain their own direction for new channels/capture resets within the session; Value-header sorting and persistent sort preferences are not added.
- Stable ListView item IDs bind names, values, colors and checkbox states; row positions are never channel identities after sorting. Checked keys are captured/restored with notifications suppressed. Cache wide names for comparison; coalesce new-channel sorting at the end of discovery batches rather than sorting on every value update. Existing capture/table row order, four chart selectors, parsers and export formats are unchanged. No new library/dependency or Python frontend change. Also updated dialog test declaration, `runtime_smoke.cpp`, Help, VERSION and local docs.
- PASS: final Debug via `scripts/build.ps1 -Configuration Debug`, canonical Release via `scripts/build_release.bat`, CTest telemetry_core 1/1 each; no reported final warnings/errors. Python `python -B -m unittest discover -s tests -v` 5/5; patch whitespace PASS. Affected-code review found no outstanding high/medium issue.
- PASS: four final `scripts/verify_runtime.ps1` runs (Debug/Release, each normal/Font125). `channel-sort-result.txt` sends actual header mouse-down/up messages, checks ascending/descending and A-Z/Z-A labels, numeric ordering P2/P10, case handling and P02 tie-break, checkbox/value/custom-draw-color preservation, live sorted-row updates, insertion of new channels, post-sort waveform checkbox control and All/None. Second-tab sorting remains independent; selecting P4 after sorting produces its correct table column/value 44, later P5 remains unchecked, and Apply/new discovery retains each panel's direction. Existing data/filter/layout/axis/export checks remain PASS.
- PASS: Debug normal sorted-channel screenshot, Debug Font125 sorted data-field screenshot and final Release Font125 sorted-channel screenshot visually inspected. Direction labels, values and checkbox/color mappings visible. Font125 is an enlarged-font fixture, not a physical monitor-DPI switch. Evidence under `build/verify/<configuration>[-font125]/channel-sort-result.txt`, `channel-sorted.png`, `data-fields-sorted.png`.
- Build preflight: gracefully closed canonical `D:\SourceCode\python\simple_com_chart_gui\build\SimpleComChart.exe`, PID 75440, under the standing authorization; normal exit confirmed, no force termination. No matching Debug instance; no unrelated process closed. Canonical user INI retains pre-build SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- PASS: actual clean removed 134 regenerable compiler files and preserved all 348 protected build files by SHA-256. Expanded 23-sentinel/source/help/repeat/native-and-cache junction tests PASS; workspace static clean audit 85 scripts PASS. Source/configuration/delivery/evidence retained.
- Delivery: `build/SimpleComChart.exe` 1.0.1.008, 3076096 bytes, SHA-256 `A0DE7E4B08AF7B0D56651CF8FB73C4A5A27030BCC30BD5CE40FE7385C2D9C9B0`. Real hardware/throughput and actual monitor-DPI switching NOT RUN. Changes local/uncommitted, no GitHub push.

## Concise filter hints (2026-10-21, 1.0.1.007)

- Changed only the two on-screen hint strings in `src/mfc_main_dialog_filter.cpp`, their exact-caption assertions in `src/runtime_smoke.cpp`, VERSION and local AGENTS/README/HANDOFF/SELF_CHECK. Each hint now says Set Regex, use `|`, and gives one example. Detailed semantics remain in Help/README. No filter, default-mode, capture or plot behavior change; prior uncommitted changes preserved.
- PASS: `scripts/build.ps1 -Configuration Debug`, `scripts/build_release.bat` and CTest telemetry_core 1/1 each. Four `scripts/verify_runtime.ps1` runs (Debug/Release, normal/Font125) PASS, including exact concise captions, widths 1120/1180/1500 on both tabs, invalid-Apply preservation and all existing chart/data/export checks. Debug and Release Font125 guidance screenshots inspected; no clipping. Python unittest 5/5 and patch whitespace PASS. No matching running target EXE at preflight; no process closed.
- PASS: actual clean removed 134 regenerable compiler files, preserving all 312 protected build files by SHA-256. Expanded 23-sentinel/source/help/repeat/native-and-cache junction fixtures PASS; workspace static audit 85 scripts PASS. Canonical INI retains SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- Output: `build/SimpleComChart.exe` 1.0.1.007, 3054592 bytes, SHA-256 `4E51DCC80CDA9158AA37E731DEAF0EEB49E8C7F623B0A977AACF4B6E4E3C33B7`. Physical COM and actual monitor-DPI switching NOT RUN for this text-only revision. No commit/push performed.

## Filter examples and separated guidance (2026-10-21, 1.0.1.006)

- Scope: continued from the existing uncommitted 1.0.1.005 work without reverting it. No filter grammar change: Text is one literal substring; Regex `|` is OR; comma/semicolon are not list separators. Filtering still accepts/rejects whole lines before numeric parsing, and Exclude wins.
- Changed `src/mfc_main_dialog_filter.cpp`, `src/mfc_main_dialog.h` and `src/mfc_main_dialog_ui.cpp`: separate applied-mode/status, Include example and Exclude example controls. `set_filter_status` updates status only and relayouts on both success/error. `src/ui_metrics.h` measures wrapping using the actual borrowed UI font. Updated help, VERSION, AGENTS and README; extended `src/runtime_smoke.cpp`, `tests/telemetry_tests.cpp` and `tests/test_telemetry.py`. No parser/serial/export/Python frontend change or new dependency.
- PASS: `scripts/build.ps1 -Configuration Debug`, canonical `scripts/build_release.bat` Release x64, CTest telemetry_core 1/1 in each; final builds have no reported warnings/errors. Both exact-path process preflights found no matching running target, so no user process was closed.
- PASS: `scripts/verify_runtime.ps1 -Configuration Debug` / `Release`, each also with `-Font125` (four runs). New `filter-guidance-result.txt` checks Include `fan,|ac_adc_mv:` and Exclude `error|timeout` through the actual receive/capture pipeline: both line types accepted, both exclusions rejected, uppercase Error retained, unrelated lines rejected, no synthetic numeric zeros. Invalid Regex and Max CH keep the capture and both help controls; Text status is restored afterward.
- PASS: existing layout regression now checks all three help paragraphs are fully measured, separate and above the tabs at widths 1120/1180/1500 on both pages, with repeated repaint and live fonts. Actual Debug normal/enlarged-font and final Release enlarged-font `filter-guidance.png` screenshots inspected: all three guidance lines visible without clipping. Existing autoscale/data/retention/export/startup regressions remain PASS. These are enlarged-font fixtures, not physical DPI switching.
- PASS: Python tests `python -B -m unittest discover -s tests -v`, 5/5 including the new multi-string examples. Native tests also verify comma/semicolon/Text-pipe literal behavior, explicit Regex OR, exclusion priority, case sensitivity and empty filters. `git diff --check` PASS; affected-change review found no outstanding high/medium issue.
- PASS: actual clean through shared `verify_clean_preservation.ps1 -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches` removed 134 regenerable compiler files; all 300 protected build files unchanged by SHA-256. 23 protected sentinels plus source, help/repeat and both junction rejection tests PASS. Static workspace scan: 85 scripts PASS. User INI retains SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- Delivery: `build/SimpleComChart.exe` FileVersion/ProductVersion 1.0.1.006, 3055104 bytes; SHA-256 `B95BA25CBCF0B7F5265FD75A49F91EEC58AEB0AB8BC6DC5626B16A3FE9421257`. Hardware COM/throughput and actual monitor-DPI switching NOT RUN. Changes remain local/uncommitted; no GitHub push.

## Selectable data columns and latest-point labels (2026-10-21, 1.0.1.005)

- Scope/baseline: clean worktree at `08d159b` on entry. Native MFC UI revision only; preserve the previous autoscale/font fixes. `data_page.cpp/.h` adds an independent field checkbox panel, default first four table columns, explicit visible-to-capture indices and a channel-limit-bounded latest-value cache. `channel_panel.cpp/.h` fits columns to the actual client area including scrollbars. Hidden fields remain captured/exported; sparse cells stay blank, and the four chart selectors stay independent.
- `plot_view.cpp/.h` plus new `plot_view_labels.cpp` draw latest-point markers/live labels, including singleton series; exclude expired/empty series and shorten only long names. Both tabs share the default-on Show name checkbox in `mfc_main_dialog*` / `resource.h`; off retains numbers. Compact large Y ticks use readable scientific notation; X captions are bounded by the client bottom margin. CMake, VERSION, help and local docs synchronized. No Python UI, parser, capture/export format or serial protocol changes; no new dependency.
- PASS: final Debug build via `scripts/build.ps1 -Configuration Debug` and canonical Release via `scripts/build_release.bat`, each with CTest telemetry_core 1/1. No reported warnings/errors in final builds. Initial test compilation hit C2664 (MFC member/global GetScrollInfo lookup); corrected to `::GetScrollInfo` and all checks rerun.
- PASS: four final `scripts/verify_runtime.ps1` runs: Debug / Release, each normal and `-Font125`. Each `data-display-result.txt` covers All/None, arbitrary non-leading columns and original-index mapping, sparse blanks, complete export snapshot, no horizontal scroll for three selected fields, actual painted labels/markers and numeric suffixes on all four plots, per-receive updates and independent scales, both name modes on both tabs, long-name numeric preservation, `(none)`, expiry and Apply reset. A 100-distinct-field fixture with Max CH=2 retained only two latest-value cache entries. Existing layout/Y-axis/retention/async-export/INI checks also PASS, including column changes with Auto-scroll off.
- PASS: final Release enlarged-font named-label screenshot and Release long-name screenshot visually inspected; earlier Debug normal/enlarged-font named and numeric-only captures also inspected. Latest dots/values, Show name, sparse table headers/columns and Time (s) captions are visible; large negative ticks retain their sign using scientific notation. Reports/images: `build/verify/<configuration>[-font125]/data-display-result.txt`, `data-selected-names.png`, `data-selected-values.png`, `data-long-name.png`.
- PASS: Python core `python -B -m unittest discover -s tests -v`, 4/4. `git diff --check` PASS. Review found an initially unbounded latest-value cache; fixed before final verification and added the Max CH regression. No outstanding high/medium issue identified in the affected changes.
- PASS: actual `scripts/clean_mfc.bat` via shared `verify_clean_preservation.ps1 -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`: 134 regenerable compiler files removed; all 280 protected build files unchanged by SHA-256. Expanded 23-sentinel/source, help/repeat, native and source/cache junction checks PASS. Workspace static audit: 85 scripts PASS. Canonical INI retains SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- Delivery: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.005, 3043328 bytes; SHA-256 `EDECCD85E8E94FF03DBD36FE1D442E4FA95748C177082B6B9D19A9006447981F`. Exact-path Release/Debug process preflight found no matching running instance, so no process was closed and closure authorization was not exercised.
- Limits: physical COM/throughput and actual monitor-DPI switching NOT RUN; enlarged-font checks are not physical DPI acceptance. Display choices are session-only, not new persisted preferences. This revision is local/uncommitted; no GitHub push requested/performed.

## Automatic Chart Y-axis (2026-10-21, 1.0.1.004)

- Scope: native MFC drawing only; preserve the uncommitted 1.0.1.003 toolbar/font fix. Changed `src/plot_view.cpp/.h` to share automatic/Fit data-window range calculation, reset empty axes and use data-derived overlay headroom. Removed native temporary-fit/expand-only state and call site in `src/mfc_main_dialog.cpp`; added `verify_y_axis_autoscale` in header/runtime smoke, updated help/version/docs. No parser/export/protocol or Python frontend changes.
- PASS: normal Release and Debug x64 builds via `scripts/build.ps1 -Configuration Release` / `Debug`, CTest telemetry_core 1/1 in each; final builds have no reported warnings/errors. Exact-path preflights found no matching running Release/Debug instance; no application was closed and closure automation was not exercised.
- PASS: both `scripts/verify_runtime.ps1` configurations, each also with `-Font125` (four process runs). `y-axis-result.txt` checks actual PlotView axis bounds through the actual receive/filter pipeline: initial 1000 -> [999,1001]; Apply resets [0,50]; excluded 1000 plus accepted 200/201 -> [199.8,201.2]; invalid Apply preserves range; overlay adds bounded headroom and stays identical across 20 updates. These are observed fixture ranges, not universal fixed limits.
- PASS: actual channel unchecking shrinks 1000/201 to approximately [200.799,201.201]; changing 30s to 5s excludes an old 1000 peak; Snapshot freezes the axis while live 2000 arrives, Fit still uses frozen samples, Live resumes expansion. Negative decimal window [-.0025,-.0015] stays negative/narrow; Refresh/no selected data restore [0,50]; constant zero has a finite nonzero span. Existing layout/font/table/scroll/export/close-guard checks remain PASS.
- PASS: `build/verify/Release/y-axis-201.png` visually inspected: Include fan filter, selected value 201, Y ticks around 200/200.5/201, not pinned to 50 or 1000. `python -B -m unittest discover -s tests -v`: 4/4 existing Python groups PASS.
- PASS: actual clean via shared preservation verifier removed 132 regenerable compiler files, retaining all 221 protected build files by SHA-256; expanded 23-sentinel/source/help/repeat and native/cache junction checks PASS. Canonical user INI independently matches pre-build SHA-256 `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`.
- Output: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.004, 3015680 bytes; SHA-256 `D6C5D1B613DE1D5F2C756138A33A7C0E5DFF8310B62E5D9B23719E7CA4ECED95`. Patch whitespace PASS; no obsolete native temporary-fit/overlay-expansion call sites remain.
- Limits: actual monitor-DPI switching and real COM hardware/throughput acceptance NOT RUN. Enlarged-font fixtures are not physical DPI verification. Native changes remain local/uncommitted and are not pushed to GitHub.

## Toolbar font-lifetime hotfix (2026-10-21, 1.0.1.003)

- Primary evidence: user screenshots of Auto Refresh / Time Window unit text wrapping/clipping and Save As clipping on 1.0.1.002. Root cause verified in `draw_owner_button`: shared `ui_font_` was deleted during repaint while controls still borrowed it. Moved release to `OnDestroy`; unit captions use single-line static style. No capture/parser/export/protocol changes.
- PASS (negative control): added the regression first and ran the unfixed Debug program. Runtime exited 1; `layout-result.txt` reported `live-fonts=0`, `font=0`, `fits=0`. Before-fix reports and two screenshots preserved under `build/verify/font-lifecycle-before-1.0.1.002`. Earlier startup-only PASS records did not establish repaint/font-lifetime safety.
- PASS: corrected Debug and Release x64 builds, CTest telemetry_core 1/1 for each; no warnings/errors in final builds. The canonical Release build ran after the user closed the old application; no user process was force-closed.
- PASS: four corrected runtime checks: `scripts/verify_runtime.ps1 -Configuration Debug` / `Release`, each also with `-Font125`. Every run validates live fonts and six full caption boxes at 1120/1180/1500-pixel widths on both tabs after three explicit repaint rounds; label/combo gaps and right-edge bounds checked. Existing filter/table/scroll/independent-plot/export/close-guard tests also PASS.
- PASS: actual corrected Debug/Release normal and enlarged-font chart captures and Release enlarged-font data-tab capture inspected. Auto Refresh (s), Time Window (s), Save As and tab captions remain visible after repaint/tab switching. Font enlargement is a fixture, NOT an actual monitor-DPI switch.
- Output: `build/SimpleComChart.exe`, version 1.0.1.003, 3009536 bytes; SHA-256 `DED9E95D4E3451B831072A7F602EA32A6A10B67E44E5BB01080A9FDD6F7EC33D`.
- PASS: actual clean through the shared preservation verifier removed 132 regenerable compiler files and kept all 195 protected build files unchanged by SHA-256. Isolated 23-sentinel/source, help/repeat and native/cache junction checks PASS. Workspace static audit PASS for 85 clean-related scripts. No EXE/INI/log/evidence deletion.
- `git diff --check` PASS. New patch remains local/uncommitted; no GitHub push performed. Real monitor-DPI switching and physical COM hardware NOT RUN for this UI-only hotfix.

## Native data tab / Save As revision (2026-10-21, 1.0.1.002)

- Scope: native MFC UI only; Python frontend unchanged. These local feature changes supersede the older published UI, but have not been committed/pushed in this revision.
- PASS: `scripts/build.ps1 -Configuration Release` and `Debug`, including CTest `telemetry_core` in each. Parser/filter/model compatibility plus bounded sparse capture, immutable snapshots, all five suffixes, decimal/Unicode/blank-cell exports, formula-safe CSV headers, locked destination preservation, 255-field XLS rejection and 254-field XLS success. Initial mixed-type auto declaration compile error was corrected before the passing runs.
- PASS: `scripts/verify_runtime.ps1 -Configuration Release` / `Debug`, each also with `-Font125` (four actual process runs). Controls/startup/close, measured Include width and gap, tab visibility, virtual row/header counts, frozen header while scrolling, follow on/off, surviving/expired anchors through 10000-row eviction, independent field/time selections, homepage checkbox independence, Refresh preservation, minimized receive processing, Apply reset/invalid-regex preservation, portable INI, asynchronous Save As completion and close-during-save guard. No assertion/timeout observed.
- PASS: generated `build/verify/Release[-font125]/chart-page.png` and `data-page.png` visually inspected. Clipped Include corrected; four distinct numeric ranges visible. Replaced stock-font fallback with system UI font and removed compact vertical Value labels which collided in an earlier local screenshot. Debug counterparts are also generated.
- PASS: `python -B scripts/verify_exports.py --reader-path <isolated-temp-xlrd-folder>` independently read generated BIFF8/CSV using xlrd 2.0.2. Checked 256-column workbook, multi-record shared-string table, Unicode headers, signed decimals, numeric types, missing-cell blanks and CSV/CVS / LOG/TXT aliases. Reader installed only in a unique temporary directory; no application dependency added.
- PASS: `python -B -m unittest discover -s tests -v`, 4 existing Python groups.
- PASS: actual `scripts/clean_mfc.bat` through workspace `verify_clean_preservation.ps1 -CleanActualProject -CheckNestedDelivery -CheckReparseRejection -CheckPythonCaches`: 132 allowlisted intermediate files removed; all 169 protected build files unchanged by SHA-256. Isolated 23-sentinel/source, repeat/help and native/cache-junction rejection checks PASS. Intermediate files are regenerable by build.
- PASS: workspace `verify_clean_scripts.ps1`: 85 scripts, no known whole-build deletion pattern. `git diff --check` PASS.
- PASS: after actual clean, reran normal Release and Debug MFC runtime checks successfully. `dumpbin /dependents build/SimpleComChart.exe` lists Windows system libraries only (including ole32); no Python/Qt/dynamic MFC/VC redistributable dependency.
- Version/output: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.002, 3004928 bytes, SHA-256 `2B0D7FFB99D3943A5CC053FE29C4B7DDB1864F984077D4FBDAE9DE528994568B`.
- Limits: 10000 lines / 32 MiB retained payload; discarded rows are visible and not recoverable from Save As. This is snapshot export, not continuous disk logging. XLS maximum 254 telemetry columns; choose CSV/log beyond it. New data-tab choices/follow/time selections are session UI state, not added to the portable INI.
- NOT RUN: physical COM device/throughput soak, actual monitor DPI switching (125% font fixture is not that), manual Windows file-dialog interactions, Excel GUI open/repair-prompt acceptance and a separate clean Windows PC. Binary XLS was independently read with xlrd; no claim that Excel GUI was exercised.

Earlier records below are historical; the new version is local until separately authorized publication.

## Authorized publication preflight (2026-10-21)

- The user now authorizes publishing this repository, superseding the earlier deferred-push note. Scope includes the preserved feature work, layout/clean changes, documentation and reviewed native delivery; no unrelated repository is included.
- Build/tests: Debug build, CTest telemetry_core, and Debug MFC smoke PASS. Initial normal Release relink failed LNK1104 while the canonical EXE was running; isolated Release compile/runtime/CTest passed with 2 MSB8029 temporary-directory warnings. After the lock was no longer present, the normal Release build to build/SimpleComChart.exe, Release CTest and native runtime smoke all PASS. No user instance was force-closed; the freshly built canonical version 1.0.1.001 EXE is published. Python core 4/4 PASS.
- Clean PASS: actual scripts/clean_mfc.bat; 102 protected output files unchanged by SHA-256. Expanded fixtures retained 23 delivery/runtime/cache sentinels plus source; object removal, help, repeat clean and reparse rejection PASS. Both native and source/cache junctions reject before native object deletion.
- Privacy/index: Ten pyc index removals retained. Former native_mfc delivery paths migrate to one canonical EXE; duplicate historical EXEs and the local 7z remain ignored locally, not added.
- Firmware/limits: N/A: no firmware in this repository.
- Candidate text/EXE checks found no tested private-key, access-token or personal-profile-path patterns; this is not a full secret/license audit. Only ordinary non-force publication to the verified existing main is authorized; exact final commit/remote SHA is recorded in the workspace migration ledger.


## Workspace layout / clean audit (2026-10-21)

- Final Python-cache follow-up PASS: 23 protected fixture files plus source, including EXE/INI/unknowns in root/source __pycache__; obj/pyc/PYO removed, help/repeat checks pass, both native and cache junctions reject before native object deletion; actual 102 protected output files unchanged. No recursive cache purge remains.

- Scope: Moved Python modules/icon to `src/python` and nine screenshots to `docs/images`, with no-overwrite SHA-256 checks. Root `main.pyw` remains compatible; native source stays in `src`. Removed ten already-generated pyc paths from the Git index only; no feature changes staged.
- Build/tests: PASS: `scripts/build.ps1 -Configuration Release` and `-Configuration Debug`; CTest telemetry_core 1/1 in each configuration. `scripts/verify_runtime.ps1` PASS for both configurations (actual MFC controls, floats, Apply, invalid-regex recovery, portable INI). Python telemetry unittest 4 groups and isolated offscreen MainWindow startup/close PASS.
- PASS: shared `verify_clean_preservation.ps1 -ProjectRoot <this-project> -CleanBatch scripts\clean_mfc.bat -CleanActualProject -CheckNestedDelivery -CheckReparseRejection` and `-CheckPythonCaches`. Non-destructive help, 17 delivery/runtime sentinels plus source, known object removal, repeat clean, and junction rejection passed; 102 actual protected output files unchanged by SHA-256.
- Additional evidence: PASS: `clean_build.ps1 -WhatIf` preserved every build file; root/source Python cache removal also checked. The combined wrapper preflights Python paths before native deletion.
- Limits/publication: NOT RUN: physical COM traffic, exhaustive visual/DPI acceptance and separate clean Windows PC. Earlier feature implementation remains uncommitted; User confirmed this batch's push is deferred; preserve all uncommitted feature work and local results.


Verification record for 2026-10-20, version `1.0.1.001`.

## Automated checks

| Check | Command / evidence | Result |
|---|---|---|
| Python parser/filter/model | `python -B -m unittest discover -s tests -v` | PASS: 4 test groups, screenshot fixture, invalid values, floats, hex, slash lists, limits and filter recovery |
| Python GUI integration | Offscreen QApplication/MainWindow, feed `tests/telemetry.txt`, Apply fan filter, reject invalid regex | PASS |
| Python syntax (original check before relocation) | AST parse of former root `core/*.py` and `ui/*.py`; current paths are `src/python/core` and `src/python/ui` | PASS |
| Release x64 | `scripts\build_release.bat` / underlying `scripts\build.ps1 -Configuration Release` | PASS |
| Debug x64 | `scripts\build.ps1 -Configuration Debug` | PASS |
| Native core tests | CTest `telemetry_core` in both configurations | PASS |
| Release startup/UI logic | `scripts\verify_runtime.ps1 -Configuration Release` | PASS: actual MFC controls, >16 channels, signed floats, Apply, invalid regex recovery and portable INI |
| Debug startup/UI logic | `scripts\verify_runtime.ps1 -Configuration Debug` | PASS: completed and exited without assertion timeout |
| Portable dependencies | `dumpbin /dependents build\SimpleComChart.exe` | PASS: Windows system DLLs only; no Python, Qt, dynamic MFC or VC runtime DLL |
| Version resource | EXE FileVersion/ProductVersion/OriginalFilename | PASS: `1.0.1.001` / `SimpleComChart.exe` |
| Clean preservation | Copy cleaner into `build\verify\clean_fixture`, seed obj/PDB and delivery sentinels | PASS: obj/PDB removed; EXE/INI/DLL/DATA/unknown files retained |
| Workspace clean audit | `..\#READ_ME_FIRST#\scripts\verify_clean_scripts.ps1` (Codex-run) | PASS |
| Patch whitespace | `git diff --check` | PASS |

Runtime reports are generated under `build\verify\Release` and `build\verify\Debug`. These are reproducible, ignored test outputs. The clean fixture is retained as evidence; production build data was not used for deletion tests.

## Root-layout migration verification (2026-10-20)

- Root `src/`, `scripts/build`, `scripts/setup` and `CMakeLists.txt`: PASS; the former nested source container was removed after confirming it was empty.
- Native resources `src/SimpleComChart.rc` and `src/app.ico`: PASS; the native icon hash matches the retained Python icon.
- CMake source directory: PASS; new `build/obj/cmake/CMakeCache.txt` contains `CMAKE_HOME_DIRECTORY:INTERNAL=D:/SourceCode/python/simple_com_chart_gui`.
- Fresh Release and Debug x64 builds from the new root: PASS, including `telemetry_core` in each configuration.
- Alternate build wrapper `cmd.exe /d /c scripts\build\build_all.bat`: PASS.
- Release and Debug startup via `scripts\verify_runtime.ps1`: PASS after relocation.
- Python core regression: PASS, all 4 groups; Python paths were retained.
- Moved clean wrapper: PASS in `build/verify/layout_clean_fixture`; obj/PDB removed, EXE/INI/DLL/DATA/unknown sentinels preserved.
- PowerShell syntax and active-source old-path scan: PASS.
- Release dependency inspection: PASS, Windows system components only.
- Historical binaries: PASS, both preserved under `build/legacy_native_mfc`; previous generated cache preserved under `build/obj/cmake_pre_layout`.
- Initial combined move/empty-directory-cleanup command was rejected before execution; separate checked moves and empty-only removals completed successfully.

This relocation verification does not change the hardware/DPI verification limits below.

## Manual / unavailable checks

- Real COM device traffic, hot unplug and sustained throughput: NOT RUN; no target stream supplied.
- Interactive visual review and 100%/125% DPI layouts: NOT RUN. Computer Use failed after retry and session reset with `Computer Use native pipe is unavailable ... os error 2`.
- A separate clean Windows PC: NOT RUN. Import-table inspection and local isolated launches support the portable build claim; USB-UART adapter drivers remain hardware-dependent.

## Earlier failures resolved

- First parser tests exposed invalid `0xGG` falling back to decimal zero. Both parsers now reject it and tests pass.
- Initial integration harness compilation exposed a `MapWindowPoints`/MFC member-name collision; explicit `::MapWindowPoints` fixed it. A runtime attempt using the older binary timed out before the corrected build; both final configurations passed afterward.
- Initial clean fixture command was rejected before execution due to sandbox recursive deletion. The narrower test retained its fixture and passed.

Re-run affected checks after any subsequent changes; do not inherit these PASS results without execution.
