# HANDOFF.md

## Native workflow delivery (1.0.1.031)

- User approved all eight latest recommendations. Contract/acceptance/order:
  docs/UART_WORKFLOW_PLAN.md. Existing .028/.029/.030 work and private INI retained.
- Added bounded view/notes/report/waterfall cores, parameter validation shared
  by CSV and a native grid, profile differences, PNG raster/encoder and five
  Laboratory pages/controllers. CaptureRow parser_prefix identifies exact text
  versus binary descriptions without duplicating raw RX. Lab field/action IDs
  moved to separate 10000/20000 ranges because added pages would overlap old IDs.
- Existing SHA256 implementation moved unchanged into file_digest.cpp to share
  session/notes hashing. CMake must add it after all session targets are declared.
- PASS initial selected cores telemetry_core/uart_session/uart_workflow 3/3;
  final Release ui_language/uart_workflow 2/2 (48 workflow checks). PASS final
  Release EN and Debug TW Font125 Workflow scopes, 55 checks each; affected
  Release Charts/Advanced/AnalysisExpansion/Lab passed. Earlier protocol/Python
  evidence reused, not rerun. Hidden DataPage columns initialize before restore;
  retained latest values repaint without waiting for RX. A mixed-object runtime
  build crashed after a structure edit during compilation; retired 323 exact
  producer-verified GUI objects and rebuilt from stable headers. Consistent
  native checks passed; no user data involved. Do not edit headers during builds.
- Added 79 paired catalog strings (578 total) and matching section 15 to both
  complete guides. Visual QA found TW Font125 profile Apply text too wide for a
  fixed button; measured captions and inherited waterfall font passed final
  checks. Three TW Font125 parameter/profile/waterfall PNGs inspected and retained.
- PASS formal Release TW/EN startup on all three tabs, normal close, recording
  options/cancel and removed-flag rejection. GUI 2,427,392 bytes (+136,192 from
  .030), CLI 434,176; Windows-only imports, static MFC/CRT and x64 ASLR/DEP/high
  entropy preserved. No runtime-test/Ribbon/theme objects in the production map.
- PASS 60,094 ms concurrent waterfall/RX/recording: 5,132,288 produced/RX/raw bytes,
  exact ordered five segments, no raw/line loss; 64 waterfall rows, max pump gap
  125 ms, peak private 15,618,048 bytes, one-core CPU 28.913%. Bounded simulated
  workload only; not physical throughput or a cross-run performance claim.
- Final safe clean removed 807 compiler files / 2,181,392,758 bytes; every retained
  pre-clean hash preserved. Then retired 271 exact owned files / 101,362,095 bytes,
  including test EXEs/caches/recovery, after persisted per-file ownership/hash
  review. build/obj and build/verify absent; formal outputs and personal INI
  unchanged. All 332 entry source paths retained; previous .030 receipt nested.
- Complete local project remains below the previously approved 45 MB cap,
  including 26,473,677 bytes Git. Exact final capacity and evidence locations:
  docs/verification/latest-validation.json. No commit/push, physical UART or
  real monitor-DPI acceptance. Some technical diagnostics remain English. Notes
  locate only identical already-loaded single-file replay; views do not recover
  expired samples. No pending feature work in this approved eight-item batch.

## Native language / bilingual guides (1.0.1.030)

- User requested lang_en.ini / lang_tw.ini, language selection, Traditional Chinese
  default, docs/USER_GUIDE.md as the default Traditional Chinese guide and a full
  docs/USER_GUIDE_EN.md. Both requested guides were absent and have been created;
  existing specialist guides remain. Other feature research is read-only.
- Native UTF-8 catalog and explicit control/choice presentation adapters added;
  499 paired translations. English source fallback; next-launch language save;
  canonical dropdown/user/serial/protocol values preserved. No new dependency.
- PASS 22 focused language checks; Release telemetry_core/ui_language 2/2, then
  final bounded-file-read refinement ui_language 1/1. PASS native Debug TW Font125
  and EN, Release TW/EN (28 checks each), affected Release Session/AnalysisExpansion.
  English/Traditional Chinese formal delivery startup checks passed on all three
  tabs. CRC caption width found by Font125 was shortened and the check passed.
- PASS isolated 60,063 ms simulated RX: 5,160,960 produced/RX/recorded bytes,
  five exact ordered segments, no raw/line loss, 15,212,544 peak private bytes,
  maximum pump gap 328 ms and one-core CPU 43.496%. This is a bounded simulated
  workload, not a hardware throughput or cross-run performance guarantee.
- GUI 2,291,200 bytes; both delivery language INIs total 85,586 bytes. Static
  MFC/CRT, Windows-only imports and x64 ASLR/DEP/high entropy preserved. Source
  entry ZIP had 337 files; no missing paths. Existing .028/.029 changes preserved.
- Exact formal delivery PID 37828 was gracefully closed under the standing user
  rebuild authorization after a linker sharing failure; no other application was
  closed. All existing uncommitted .028/.029 changes preserved.
- Both complete guides have corresponding 14 sections, valid local links and
  escaped Regex pipes in Markdown tables. USER_GUIDE.md is the default Traditional
  Chinese guide; USER_GUIDE_EN.md is the English guide; no duplicate TW guide.
- Final clean PASS: 654 compiler files / 1,944,700,087 bytes removed with every
  remaining pre-clean hash preserved; then 244 exact owned files / 91,556,584 bytes
  retired after source/delivery acceptance and a persisted per-file receipt.
  build/obj and build/verify are absent. Formal GUI/CLI/catalogs and personal INI
  unchanged. Four selected PNGs plus docs/verification/latest-validation.json kept.
  Project about 38.47 MB including 26.47 MB Git, within the previously approved
  45 MB final cap; no Git history deletion. No commit/push; physical UART/monitor
  DPI NOT RUN. Some diagnostics remain English; language applies next launch.

## Session/workflow delivery (2026-10-26, 1.0.1.029)

- Completed all eight approved items using independently written native C++/MFC
  and Windows APIs. Product usage: docs/UART_SESSION_GUIDE.md; contract and scope:
  docs/UART_SESSION_PLAN.md. The existing uncommitted .028 work was preserved.
- Added recording guards/capture-start metadata, relative SHA256 sessions,
  device information, A/B/offline numeric framing, shared custom CRC8/16/32,
  explicit nested JSON paths, retained-data calculus and passive transactions.
  Six optional Laboratory pages retain the three everyday main tabs. Parser and
  Port B pointer settings use v2; normal old formats keep their meanings.
- PASS selected Release cores 10/10; affected parser/profile changes 5/5; final
  uart_session 76 checks. Native Debug Session Font125 at 700x520 plus Release
  Session: 36 checks each. Affected Debug Uart/AnalysisExpansion/Lab passed.
  Formal GUI and CLI deployment checks passed; personal INI hash unchanged.
- PASS isolated guarded 60 s receive: 5,263,360 produced/RX/recorded bytes, six
  correctly ordered segments, zero raw/line loss, 14,110,720 peak private bytes,
  maximum UI pump gap 547 ms (<1000 ms criterion), one-core CPU 37.4873%.
  This is simulated workload evidence, not guaranteed physical throughput/latency.
- Formal GUI 2,272,256 bytes; optional CLI 434,176 bytes. Both are x64 with system
  DLL imports only, ASLR/DEP/high entropy. Delivery excludes runtime tests/demo/
  Ribbon/theme managers. Exact hashes and compact reports: latest-validation.json.
- Final safe clean removed 913 compiler files / 1,932,389,468 bytes with
  1,232 protected hashes unchanged; 261 further proven generated files retired.
  obj/verify/Debug are absent and formal EXE/INI still match. The late retirement
  receipt guard compared a stale set; the extra byte total was not retained.
  Independent residue/boundary/hash checks passed. Details: latest-validation.json. Do not rebuild merely to recreate retired test/obj data.
  The one .028 recovery baseline is retired after verified delivery; no duplicate
  raw archive is needed. Preserve formal EXE/personal INI/Git and user data.
  Normal folder target is 20 MB; reuse the existing explicit 45 MB final decision
  for retained Git history, without requesting approval for temporary build growth.
- NOT RUN: physical UART/board/driver/property completeness, actual monitor DPI/
  mouse/foreground, OS Save As picker and explicit clipboard copying. Fake tests
  do not establish electrical/hardware acceptance. No third-party source/runtime,
  dependency installation, Git commit or push. Python frontend is unchanged.

## UART laboratory delivery (2026-10-26, 1.0.1.028)

- Implemented all eleven approved recommendations in docs/UART_LAB_PLAN.md;
  VERSION 1.0.1.028. No dependency/runtime addition, hardware access, commit or push.
  Working-tree entry was clean at 21e329e; current changes belong to this task.
- New owned UART Laboratory exposes diagnostics, recording range/health/salvage,
  independent typed packet parameters, sequence/response statistics, offline
  parser cases, extended binary schemas/codecs and digital history. Optional
  finite CLI shares native script/transport and GUI session exclusion.
- PASS Release affected core 7/7 (telemetry_core, binary_frames, uart_commands,
  uart_advanced, uart_analysis, uart_lab, uart_lab_performance); final stricter
  assertions/UTF-8 handling selected only telemetry_core/uart_advanced/uart_lab,
  3/3 PASS. Final font-only build passed the bundled telemetry_core baseline.
  Debug enlarged-font and Release Laboratory controls/fake-TX/files, affected
  Debug AnalysisExpansion/Uart, CLI seven checks and formal Release startup pass.
- Resolved laboratory scrollbar/client-width and nested control font propagation;
  final font assertions and visually inspected packet/digital PNGs pass. Existing
  Uart close fixture assumed the
  pre-Workbench close order; it now drains the actual Workbench -> UART -> recorder
  stages. Production shutdown behavior was preserved. CLI verification uses an
  owned Process object rather than PowerShell's unreliable quick-child ExitCode.
- Retention runner now rejects missing sources before creating owned folders.
  Its test builds only the missing session helper, handles absent personal INI,
  and passes all 15 ownership/cleanup/process/path checks after that bootstrap.
- Isolated final 60-second synthetic soak: 5,238,784 produced/RX/recorded bytes,
  raw_lost=lines_lost=0, ordered rotation, 94 ms maximum UI pump gap, peak private
  13,406,208 bytes. This is simulated host-load evidence, not UART wire throughput.
- Final GUI 2,120,704 bytes, SHA256
  64DD6C2732DBA0E6313BBB2F71CA5452EFB046B3D10EA5EBBEC7BF5B3032EF26;
  optional CLI 432,640 bytes, SHA256
  3B644B57FADE712B43A44169E85ABCF53B69CA4014C593C4C5D323E227668DA6.
  Both x64/static CRT, system-only imports and ASLR/DEP/high-entropy flags retained.
  Formal GUI map excludes runtime fixtures/demo/Ribbon/theme-manager symbols.
- User subsequently authorized ordinary temporary build/test excess without
  repeated approval; shared rules/TST-02 now say so. Normal final target is 20 MB;
  the earlier explicit Git/final retained decision for this delivery is recorded
  in the plan (45 MB). Necessary final excess must be reported, not silently ignored.
- Personal delivery INI matches entry SHA256
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  The .027 baseline remains recoverable in Git; the temporary baseline copy may
  retire now that candidate delivery/comparison passed.
- Final cleanup PASS: safe clean removed 752 compiler files / 1,732,813,878 bytes
  while all 71 protected files matched SHA256. Then retired 265 exact proven
  generated files / 85,734,149 bytes and 68 empty directories; all 322 unselected
  non-Git files matched before the intentional final documentation updates.
  build/obj and build/verify are absent; build keeps only formal GUI, CLI and INI.
  Eighteen current/previous reports remain byte/hash-reconstructable in the compact
  docs/verification/latest-validation.json; no raw test copies or retirement script.
- Retained project is about 37.59 MB including Git 26,473,677 bytes, under this
  delivery's approved 45 MB cap; non-Git about 11.12 MB. This uses the existing
  explicit Git-retention exception, not normal 20 MB compliance. Exact final
  post-document measurement is in the canonical receipt. No external deletion,
  broad folder purge, Git-history removal/repack or rebuild after final clean.
- Physical UART/board/driver/monitor-DPI/foreground/manual mouse/wire timing and
  CLI hardware execution are NOT RUN. No commit/push; publication is not implied.

## Four requested build directories retired (2026-10-26)

- Removed the explicitly named build/obj, build/verify, build/Debug and
  build/legacy_native_mfc after ownership/source/cache/publication checks.
  This supersedes the older retention statements below: the .027 source and
  formal EXE are published at 0411a680d22e18d17d300df5a4303ce94d27e9ba;
  no rollback binary, historical map or completed-run cache remains necessary.
- Exact-file deletion removed 3667 files / 74,779,672 bytes, followed by 574
  verified empty directories, including the four roots. All paths stayed below
  the independently selected project root, with no reparse traversal or active
  selected process. No recursive directory purge or external deletion.
- All 290 unselected non-Git files matched immediately after deletion. Formal
  EXE SHA256 remains BB7F4ACF1FA08443329949477644BB28C0390C8FF8A2DB8F91A276224D89B08A;
  private INI remains 1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  build now contains only the Release EXE and portable INI. No source/build-script/
  VERSION changes. Later documentation edits are intentional, outside that hash
  preservation checkpoint.
- Retained five compact previous reports, hashes and cleanup receipt in
  docs/verification/latest-validation.json (10,446 bytes, original encodings
  recorded and all five byte hashes verified). This reuses .027
  evidence; no application tests, performance or hardware checks were rerun.
  Full per-file audit scratch was retired after compacting the result.
- Measured project 111,406,971 -> 36,637,441 bytes before these final document
  edits, including .git 26,333,246 bytes. Non-Git content: 10,304,195 bytes.
  The 20,000,000-byte review threshold is still exceeded. Developer decision on
  retained excess/compression is pending; no 40 MB exception has been approved.
- Read-only Git review: 13 commits, 580 loose objects, no reported garbage.
  Three unreferenced blobs occupy 1,452,684 bytes and remain protected as possible
  recovery data. Fourteen verified stock .sample hooks occupy only 25,827 bytes.
  All-object in-memory packing estimates span 12,724,169..14,580,708 bytes; this
  estimates compression, not a guaranteed final project total. No Git file was
  deleted, repacked, pruned, committed or pushed in this task.
- Updated agent/build/product documentation to describe generated directories
  as recreatable rather than permanent retained outputs. Documentation checks
  and final size/hash/path checks passed. Git fsck found only the same three
  unreferenced blobs and no integrity errors. No app rebuild was needed.

## Final redundant build-output retirement (2026-10-26)

- User explicitly requested removing remaining redundant files from the381MiB
  project. Entry worktree was clean. Published recovery checkpoint is confirmed:
  local HEAD and live origin main both0411a680d22e18d17d300df5a4303ce94d27e9ba,
  VERSION1.0.1.027; local formal EXE's Git blob equals the published tree. This
  resolves the temporary recovery-archive exception in the historical record below.
- Removed43 exact registered files289,050,039B (275.66MiB):140.23MiB expired
  recovery ZIP,85.42MiB obsolete Debug/verification/pre-slimming linker maps and
  50.01MiB compiled CMake test/benchmark EXEs. Target roles were derived from
  committed CMakeLists and project-local cache home directories; full SHA256 was
  captured and rechecked before deletion. No directory/extension-only purge.
- Independent deletion boundary was the resolved selected project root, with
  narrower build-output checks and ancestor reparse rejection. Only files were
  removed; no retry of earlier policy-blocked empty-directory cleanup. No system,
  user-profile, TEMP, sibling-project or Git-history cleanup.
- All3950unselected non-Git files matched before/after SHA256 at retirement,
  before the intentional AGENTS/HANDOFF/SELF_CHECK documentation updates.
  Formal EXE remainsBB7F4ACF1FA08443329949477644BB28C0390C8FF8A2DB8F91A276224D89B08A;
  INI remains1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
- Inventory381.02MiB ->106.24MiB, including25.11MiB .git. Remaining build73.18MiB:
  verify50.83MiB,current/formal Debug9.23MiB,obj5.91MiB,legacy_native_mfc5.34MiB,
  formal Release1.86MiB. Keep current runtime-bin Debug/Release, the current
  production Release map, source backups/needed evidence and unknown data. The
  two legacy native EXEs had no validated embedded identity; they were retained.
- Project AGENTS now explicitly requires review of compiled test EXEs/obsolete
  maps and expiry checkpoints, separately from ordinary clean's delivery-EXE
  preservation. Application source/build scripts/VERSION/EXE/INI unchanged.
- Verification: exact-file manifests/protected hashes, published SHA and delivery
  blob checks, no active selected process and no reparse entries. No application
  rebuild, full tests, hardware operations, commit or push required/performed.
  Doc UTF-8/fences/references/whitespace are the relevant final checks.
- Evidence: build/verify/redundancy-final-{removal-plan,protected,result,
  preservation,inventory}.json. The historical recovery ZIP references below
  describe an artifact intentionally retired after its checkpoint, not a missing
  current requirement.

## Verification artifact cleanup (2026-10-26)

- User requested actual cleanup of redundant build/verify contents, not only Git
  ignore rules. Read-only inventory:639.93MiB,4613 files,99EXEs/45unique hashes;
  byte-identical duplicate EXEs alone200.02MiB. Formal .027/personal INI unchanged.
- Source/metadata/content validation identified1196 disposable old test artifacts:
  94superseded COM Chart/renamed test-helper EXEs, exact fake-soak and300000-x RX
  recordings with validated readable/index counterparts, and synthetic capture/
  Messages exports. All259XLS files matched test workbook signatures and adjacent
  exact generated raw inputs. No directory/extension-only deletion or MCU/user
  recording deletion. Unknowns and two zero-byte clean-preservation EXE sentinels
  remain; current runtime-bin Debug/Release plus one .026 rollback remain.
- Automatic approval review rejected the initial irreversible bulk removal,
  returning only "blocked by policy". Used a materially safer recoverable path:
  archive every planned file, reopen and validate all1196entry SHA256/byte counts,
  then retire exact registered originals. Archive: history/
  completed-verification-20261026.zip,147,043,694B,SHA256
  F68725C3AE8DDAFDD6C98D96A62C41D5E01DC301950540398762F9F80420A242.
  Retain this ONE140.23MiB recovery snapshot until publication/recovery of the
  current .027 source is confirmed; then Codex should revisit this explicit
  retention exception. No additional history archive is a normal per-test output.
- Retired1196 originals614,935,028B, net467,891,334B reclaimed after the archive.
  All3483other verification files matched before/after SHA256 at retirement;
  formal delivery/settings hashes remain those in the .027 record below.
  Final verify size195.81MiB, including140.23MiB archive; other evidence/canonical
  binaries/source backups total55.58MiB. Three nonempty verification EXEs remain.
  New audit receipts account for the difference from the theoretical net saving.
- Batch removal of empty retired directories was separately rejected by automatic
  approval review, also only "blocked by policy". That command did not execute;
  empty historical directories remain. No alternate deletion method was used.
- Prevention: scripts/verify_runtime.ps1 now uses fresh owned runs and
  scripts/test_artifacts.ps1: after the owned process exits, copy bounded latest
  pass/failure results/configuration/images/hash manifest to evidence/<case>/,
  then remove registered scratch including EXE/bin/log/exports. Existing unknown/
  edited evidence blocks overwrite; reparse/parent escapes and active processes
  block deletion. Old history paths below may now refer to archived files;
  the removal plan/archive manifest maps every retired path and exact hash.
- Native test prevention: session_instance_tests uses fresh PID/tick directories,
  closes all private child handles and removes renamed-helper.exe with checked
  RAII; capture_tests/disposable_test_files.h removes eight synthetic export files,
  retaining only a small recipe. Exception unwinding is exercised in both.
- PASS: test_artifacts_tests.ps1 fourteen cases: success/failure/cancel-finally,
  repeated bounded evidence, unknown/edited evidence preservation and retry,
  lower-case SHA normalization, parent/junction/active-process guards, EXE/INI
  preservation. Initial empty-directory array/hash-case test defects were fixed;
  these final results supersede initial failed attempts.
- PASS: scoped Release session_instance and telemetry_core native targets; the
  existing telemetry runner unavoidably bundles parser/filter/model checks, no
  other suite was appended. Actual Release Charts -SkipBuild exercised the new
  runner twice with the unchanged .027 verification binary; last run disposed
  2,750,322B scratch and retained476,392B evidence. No main-app rebuild/version
  change, All/Python/hardware/performance rerun or Git publication.
- PASS: static clean audit24scripts; final clean_mfc.bat removed51 compiler files
  produced by the focused test builds. No rebuild after clean. Git tracks zero
  verify files; archive/evidence both remain ignored. Source changes are tests,
  runner/helper and documentation only; unrelated preexisting work remains.
- Evidence under build/verify: verify-retention-inventory.json,
  verify-artifact-removal-plan.json,verify-recovery-archive.json,
  verify-artifact-retirement-result.json,verify-artifact-preservation-result.json,
  verify-protected-files.json,verify-retention-final-inventory.json,
  verify-retention-{session-test,export-test,mechanism-test,runtime-test,
  clean-audit,final-clean}.log and evidence/Release-charts/last-pass/.

## Analysis expansion delivered / retention cleanup (2026-10-26, 1.0.1.027)

- All ten approved items completed: dense peak-preserving rendering, independent
  B text charts, offline binary replay, full workspace v2, derived values/EMA,
  byte/bit/enum inspector, numeric script assertions/CSV, run comparison, live XY,
  explicit finite A/B forwarding. Guide: docs/UART_ANALYSIS_GUIDE.md; acceptance:
  docs/UART_ANALYSIS_PLAN.md. Existing unrelated work remains; no Git commit/push.
- Formal Release: build/SimpleComChart.exe,1,954,816B,SHA256
  BB7F4ACF1FA08443329949477644BB28C0390C8FF8A2DB8F91A276224D89B08A.
  Personal INI SHA256 remains
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  Static MFC/CRT, system-only imports; retail map excludes runtime fixtures,
  Ribbon/theme manager. Formal Debug was not rebuilt; isolated Debug checks are
  .027. No running formal target needed closure. No hardware/new dependency.
- PASS: selected Release cores6/6; after bounded-loader changes, affected3/3;
  final uart_optimization retention test. Debug Charts/Workbench/Advanced and
  Release Charts, plus final Debug/Release Font125 AnalysisExpansion and Debug
  Advanced after loader changes. Actual native controls, fake RX/TX/leases,
  independent A/B formula names, replay boundaries, profile apply/cancel,
  inspector exact visible byte highlight, XY/comparison and worker teardown pass.
  Enlarged-font PNGs inspected. Inspector fixture pumps initial paint; an earlier
  immediate PrintWindow could return a black client, so that image is superseded.
- Actual retail verification PASS: Messages/Chart/4Charts empty startup, modal
  recording options cancel, normal close, demo/smoke rejection, private INI hash.
  New verify_delivery finally cleanup PASS on success (three EXE copies5,864,448B)
  and injected pre-launch failure (one copy1,954,816B). Canonical hash retained;
  results/settings stay in fresh GUID roots. No unrelated user process operated.
- Performance: same in-memory GDI+ bitmap/quality/width,4x20k points, median
  legacy22.2145ms vs peak-span12.1841ms. Four sequential60s independent-producer
  RX/recording soaks, baseline .026 twice then candidate .027 twice: all exact
  byte/order/loss/retention/memory/UI checks PASS. CPU per MiB4530.65..4667.70ms
  baseline vs3950.31..4170.81ms candidate; one-core CPU37.98..39.10% vs33.10..34.92%.
  Candidate stays within the baseline repeat band; this workload shows no slowdown,
  not a physical-UART/global-performance guarantee. Generated44,099,997B payloads
  and all four copied EXEs were removed after metrics/hashes/recipes were saved.
- Shared retention policy: ../#READ_ME_FIRST#/WORKSPACE_RULES `Test Artifact
  Ownership And Retention`, AGENTS completion rule, SELF_CHECK TST-01 and README.
  Codex owns RAII/finally cleanup on success/fail/cancel plus safe final clean-obj;
  no manual user checklist. uart_optimization_tests uses fresh PID/tick ownership,
  reparse rejection and checked cleanup:74,240,000B bulk +13 small fixtures removed,
  leaving a128B recipe. Intentional exception and cancelled replay cases pass.
- Historical25 large.log fixtures removed only after exact generator/SHA matching;
  total1,856,000,000B. Final receipt records22; an earlier safety-aborted cleanup
  had already removed3 verified files before its active-test guard stopped it.
  Exact names of that first partial batch were not persisted; before/after
  inventory plus guarded implementation establish the aggregate. No wildcard
  deletion of logs/binaries, no real UART data deletion. Zero bulk large.log remain.
- Five further task-owned hash-identical EXE copies removed19,815,936B; canonical
  references remain. Final clean_mfc.bat removed1418 compiler files2,418,893,309B;
  all4809 protected non-intermediate files828,566,900B retained identical hashes.
  The normal clean still preserves every EXE/INI/DLL/log/data/unknown file. Compiler
  intermediates were not rebuilt after clean. Scoped static clean audit PASS24.
- Project disk use about4.601GiB ->0.804GiB (83% lower), including .git. Remaining
  build artifacts include historical verification executables/maps/evidence and
  configuration; they were not indiscriminately purged. Git-tracked bytes8.976MiB;
  verify/obj/Debug/legacy/INI are ignored and untracked. No upload was performed.
- Evidence under build/verify: analysis-{final-cores,bound-cores,final4-debug,
  final4-release,final3-debug-advanced,render-benchmark,delivery-imports}.log;
  analysis-final-delivery.json,analysis-linker-map-check.txt,retention-final-test.log,
  retention-delivery-{success,failure}.log,delivery-failure-recipe.json,
  test-payload-cleanup-result.json,owned-test-copy-cleanup.json,
  pre-clean-protected-files.json,post-clean-result.json,retention-clean-audit.log,
  project-size-{audit,after-clean}.json. Performance plan/metrics/cleanup receipts:
  analysis-performance-0049f9d34b924774af9654582551e29e/.
- NOT RUN: physical UART/board/driver/wire timing, real monitor DPI switching,
  physical mouse/foreground/elevation acceptance, unrelated Python/full suite.
  There is no remaining implementation/build/affected automated-check task here.

## Default-none channel choices (2026-10-25, 1.0.1.026)

- User requested unchecked discovery in Chart and 4 Charts. New entries in both
  checkbox panels now start false, including after Apply/Connect; later fields
  do not alter existing user choices. Main Chart selection remains independent
  of table columns and four small-plot selectors. Explicit saved small-plot
  choices restore normally; capture/export retain hidden fields.
- Release delivery: build/SimpleComChart.exe,1,735,168B,SHA256
  3DBEFE291204F0E5CECCCEFC8AAFF7F17AB7BD30CDF2003AF81F484D41400AF0.
  Resource/file version verified as1.0.1.026. Reconfigure CMake after VERSION
  changes when building a direct cached target; the normal build script already
  configures. The first direct build retained an old version resource; explicit
  configure/build corrected it and final delivery verification passed.
- Scoped checks: Debug/Release Charts (axes, unchecked discovery, opt-in choices,
  later discovery/new capture, table mapping/hidden capture, saved plot choices
  and Reset); Debug Optimization (affected table/search preferences plus existing
  bundled navigation/file-TX fake-transport checks); actual Release delivery
  startup/normal close/removed flags. All passed. No full suite, Python, clean,
  hardware, font variants or publication were selected for this small UI change.
- Evidence: build/verify/default-none-*-charts.log,
  default-none-debug-optimization.log,default-none-release-delivery.log.
  Baseline source/EXE: build/verify/default-none-baseline/20261025_220752.
  Personal INI SHA256 stayed
  1E4E0409F572E46DEF1BCAE3BCBD15EF53AD6ABB14CF72BA69E2D14B10CAE4A3.
  No matching target process needed closure. Formal Debug delivery was not
  rebuilt; both separate Debug/Release verification EXEs are1.0.1.026.
  Unrelated pre-existing work remains uncommitted; no commit/push performed.

## UART workbench delivered (2026-10-25, 1.0.1.025)

- Completed the approved items1..9 plus two simultaneously visible, vertically
  stacked independent COM groups. Checklist: docs/UART_WORKBENCH_PLAN.md; product
  instructions/limits: docs/UART_WORKBENCH_GUIDE.md. Do not restart this expansion.
  Future fixes use change-impact checks only, not the entire historical suite.
- Formal Release: build/SimpleComChart.exe,1,735,168B (about1.65MiB),SHA256
  6E1EF436238C1BBB95F73F72B7263F36DFDA10E92920C32A37813773BE86A00D.
  Formal Debug: build/Debug/SimpleComChart.exe,9,683,456B,SHA256
  2910A42BC42E386910F1A5A9742017F3450FEECE49E74223F70121AD3C4F648E.
  Entry EXE backup/source/status: build/verify/workbench-baseline/20261025_297d2d67.
  completion.json contains final sizes/hashes and evidence-log references.
- Implemented: independent open/RX/TX/record/save/follow/clear, unique device
  ownership/recovery, framed inspection, binary schema/CRC/preview/INI/chart,
  grouped command panel/private definitions/parameter slider, per-step scripts,
  worker-side byte/line pacing, XMODEM checksum/CRC/1K and YMODEM batches, staged
  atomic receive files, read-only Modbus03/04 register decode/chart, monochrome
  VT100-style ASCII screen, Number/Gauge/Bar/LED/FFT.
- Preserve port/generation boundaries and TX leases; no retry of uncertain partial
  writes or execution after reconnect. Protocol completions avoid UI-timer waits.
  Binary/RTU/protocol paths bypass ordinary text parsing while preserving raw byte
  offsets and recording. Profile Apply locks owned settings windows; offline/profile
  work excludes new B opens. B recording-close failures preserve files and warn.
- Verified: formal Debug/Release builds; selected Debug core7/7 and Release7/7,
  then only changed command/display cores refreshed after final bounds/rename.
  Debug/Release native Workbench font125 each76PASS records,0FAIL, covering real
  controls, script confirmation, isolation, exports, raw retention, protocol peer,
  RTU/chart, FFT, profile locks, B-only isolation and close errors. Screenshots
  reviewed under build/verify/{Debug,Release}-font125-workbench.
- Two-second isolated dual-RX smoke:17,112B per port, exact recorded bytes on both
  sides and recorder loss0; final Release/font1252.031s wall,0.703s process CPU.
  This is a bounded smoke measurement, not long-duration or hardware throughput.
- Actual hash-identical retail copies passed three startup-tab choices, no fake
  data, recording-options controls/cancel, normal close and production rejection
  of --demo/--smoke-test. Evidence: delivery-Release-d47a663ddadc460f8a0873dd3055fab9
  and delivery-Debug-0f578b2b468f4d0c90fdc0b535e29589 under build/verify.
- Release grew431,104B for the requested features; static MFC/CRT and size/security
  flags remain. Imports are Windows system DLLs only; link map excludes Ribbon,
  CMFCVisualManager,CDialogEx and runtime test/telemetry fixtures. No new dependency,
  compression wrapper or installer. Final rebuild logs have no compiler warnings.
- Personal build/SimpleComChart.ini remains unchanged (SHA256
  CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33).
  No existing snapshot file removed; Python/unrelated work preserved. No MCU/shared
  workspace rule changes, actual clean/purge, Git commit or push for this task.
- NOT RUN: physical COM/USB/driver/pin/wire timing, real monitor-DPI, long-duration
  device throughput and actual mouse/file-dialog acceptance. Tests used isolated
  INIs/files and owned processes/fake transports only. Terminal is a documented
  subset; receipt clocks and raw Expect matches are not protocol/device proof.

## Everyday production build, no demo (2026-10-24, 1.0.1.024)

- User explicitly requested removal of unneeded demonstration functionality. Production now excludes the generator, its state/timer/retry branch, Help instruction and Save As hint; `--demo` is rejected with exit 2 before normal initialization. Do not restore demo/sample UI without an explicit requirement. Real UART, offline replay and charts remain; Python/unrelated dirty work is untouched.
- Simulated input exists only in `runtime_test_telemetry.cpp`, guarded and listed only for `SCC_ENABLE_RUNTIME_CHECKS=ON`; it is used by isolated smoke checks, not a user option or a shipped runtime source. This preserves safe hardware-free regression tests. Shared portability guidance now says unrequested demo/sample-data generators do not belong in everyday delivery.
- `build/SimpleComChart.exe`: **1,304,064 bytes (1.24 MiB)**, 7 KiB below 1.0.1.023; version 1.0.1.024, SHA-256 `27DC0686C743E44539FC9EA691AF96BD9CDCA8FC6DCA129E79185E881DB85DB6`. Prior 1.0.1.023 is preserved under `build/verify/no-demo`. Ribbon/CMFCVisualManager was already excluded; final map still contains zero such symbols and zero synthetic feeder symbols. No UPX or installer dependency was added.
- PASS: canonical Release plus bundled 6/6 native targets (9.56 s); focused Debug application rebuild; actual delivery copies in both configurations restore all three empty startup pages, stay free of fabricated data, cancel Recording options and close normally; reject `--demo` and production self-test flags. Separate Release Charts and Serial scopes pass, covering relocated input and retry/connection behavior. No All/Python/full matrix/soak rerun. SELF_CHECK has exact hashes, commands/evidence and unchanged acceptance reuse.
- Personal Release INI hash unchanged: `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`. No normal target was running at full-path preflight, no user's process was closed, no real COM/TX/DPI test, no material cleanup/deletion or GitHub push. Project/shared docs updated. Work remains local/uncommitted. No required implementation remains in this removal request.

## Native EXE size work complete (2026-10-24, 1.0.1.023)

- Current authority: user asked to implement EXE-size reduction after static analysis of UartAssist. Scope is SimpleComChart, not a workspace-wide conversion or publication. Existing UART functionality/dirty work, x64, static MFC/CRT, personal settings and Python are preserved. No UPX or new dependency.
- Delivery `build/SimpleComChart.exe`: **1,311,232 bytes**, formerly 3,954,176 bytes (**-66.84%**), version 1.0.1.023, SHA-256 `BBD5988D2782B7B152F04BEEC767C3F3C67F5960CC38A0F15B14DE868E31045C`. Removing UI fixtures and enabling size/LTCG alone reached 3,432,960 bytes; CDialog alone made almost no difference. The major verified saving is `CDialog` plus `_AFX_NO_MFC_CONTROLS_IN_DIALOGS`, excluding unused automatic Feature Pack registration/Ribbon/theme dependencies. Win32 controls and resource DIALOGEX formats remain supported. No CMFC control may be added without reviewing this restriction.
- Build separation: default/normal build explicitly sets `SCC_ENABLE_RUNTIME_CHECKS=OFF`; runtime scripts build ON in independent `build/obj/cmake-runtime` and output only under `build/verify/runtime-bin/<Configuration>`. `runtime_*.cpp` is absent from production, `--smoke-test` there fails closed with exit 2; ordinary `--demo` remains. `verify_runtime.ps1` builds/copies the correct test sibling automatically. New `verify_delivery.ps1` independently tests hash-identical actual delivery copies/fresh private INIs, refuses an existing normal owner and never connects hardware.
- PASS: canonical Release/Debug with bundled 6/6 native targets each; actual delivery three startup tabs/demo rows/Messages text/Recording options modal cancel/normal close/test-entry rejection in both configurations; Debug Startup, Release Uart/Charts/UartOperations/Advanced/Optimization. Selected wider checks are justified by changed shared dialog/build/startup/export behavior, not an automatic full suite. Release 60 s synthetic receive: 5,980,160 produced/RX/recorded bytes, six exact ordered segments, zero raw/line loss, max pump gap 296 ms, peak private 13,193,216 bytes. No All/Python/physical-DPI suite was appended. SELF_CHECK records exact commands/artifacts and historical evidence separately.
- Static/packaging: final link map has zero Feature Pack/Ribbon/VisualManager/CDialogEx symbol matches; dependencies remain Windows DLLs; manifest has one legacy DPI node plus PMv2/asInvoker. Personal release INI hash remains `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`. Clean static audit 88 PASS, no actual purge. Old EXE/intermediate maps and prior recording segments are preserved. No normal target was running at preflight; no user's process was terminated. A generic Afx-only frame-probe failure was retained and replaced by a successful PID/caption-based delivery check, not hidden as an app regression.
- Docs: project AGENTS/README/MFC_BUILD/SELF_CHECK and shared WORKSPACE_RULES now describe safe static-MFC size reduction and separate verification vs delivery. Work remains local/uncommitted; no GitHub push. NOT RUN: physical COM/USB/driver, real DPI/mouse/native file-dialog/foreground/elevation matrix or overnight soak. No additional accepted size item remains; future compression or other-project rollout needs its own scoped measurement/verification, not automatic continuation.

## Native UART optimization complete (2026-10-24, 1.0.1.022)

- Latest authority is user's approval to implement the post-A/B1/B2/C recommendations until complete. All eight groups in `docs/UART_OPTIMIZATION_PLAN.md` are implemented/verified; no accepted SOFTWARE item remains. Do not restart the completed roadmap or interpret older handoffs' remaining items as current. `docs/UART_OPTIMIZATION_GUIDE.md` is the current Traditional Chinese operating/safety guide.
- Added stable per-session single-instance activation before UI/settings/workers, PMv2 awareness/fonts/responsive layout and clipped scrollable seven-page advanced settings; bounded NFA/visible LIVE-OFFLINE/REC/AUTO/provenance/loss; generic CSV/Tab/flat JSON/Regex name-value-time extraction with preview/portable settings; searchable stable channels plus display-only alias/unit/precision/scale/offset; zoom/pan/drag cursors/fixed Y/latest and exact RX offset-count-sequence correlation; background disk-paged replay/rotation/search/time seek; optional light terminal/explicit keyboard/local echo/confirmed paste; finite generation-bound binary/text chunk file TX with pacing/progress/pause/cancel and no partial retry/reconnect replay. Raw RX and exported numeric values remain unchanged by display options. Python/other projects remain untouched.
- Kept separate core/composition modules and added native optimization/instance plus scoped UI/soak fixtures. Shared receive performance now avoids regex scalar parsing, full-history copies for latest values and hidden-page rebuilds. Bound numeric pending input to 2000 lines/4 MiB and dispatch to 64 lines/~8 ms. Fixes include manifest duplicate DPI startup rejection, scroll/large-font clipping, source-boundary/duplicate-time mapping, original-snapshot cursor dragging, finite extreme-axis/tick safety and O(1) terminal line eviction. New reusable DPI-manifest rule added to workspace `#READ_ME_FIRST#/WORKSPACE_RULES.md`.
- Final PASS: canonical Release and Debug builds, 6/6 bundled native targets each (8.23 / 10.22 s), Debug Font150 and Release Font125 Optimization, final Release Advanced, affected Debug Charts/Serial/Uart/UartOperations. Actual PMv2 HWND context passes; final enlarged-font PNGs inspected. Static safe-clean audit 88 PASS, no actual purge/clean. Final review has no known unresolved high/medium finding. SELF_CHECK lists changed files/families, commands, exact evidence reuse and retained failure artifacts; no All/Python suite was appended.
- Sustained simulated RX PASS: Debug 60 s and Release 300 s. Release generated/received/recorded 30,224,384 bytes, 29 segments with matching ordered stream hash, zero software byte/line loss, max UI-pump gap 63 ms and peak private memory 15,679,488 bytes. This is local software evidence, not real UART throughput or overnight stability. The 300 s fixture was built before the final optional extracted-time-only guard; unchanged Key/value/no-time-field receive/storage/render evidence was reused explicitly, with final parser/core/UI checks rerun. See SELF_CHECK for both fixture/final hashes and the preserved first failing soak.
- Delivery: `build/SimpleComChart.exe`, 1.0.1.022, 3,954,176 bytes; SHA-256 `78585A5898D316C079C088C9BE5E13A749E7D1421A7294FF62DA8D1E195B9FD4`. Personal INI entry/final SHA-256 `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`, unchanged. Canonical Release/Debug process checks found none running; no production process was closed/terminated. Work remains local/uncommitted, no staging/push/private-data publication.
- NOT RUN: physical UART/USB/flow/pins/Break/target effects, actual monitor DPI switching, physical cursor/clipboard/file-dialog acceptance, desktop foreground/elevation/session behavior, overnight stability and blocked filesystem/driver cancellation. Cooperative single-instance logic cannot retrofit old pre-guard binaries. No firmware/event-specific recording, implicit flashing/reset or network login was added. No further implementation authority is needed for this completed batch; future requests should begin from this state.

## Accepted UART plan complete - B2/C (2026-10-24, 1.0.1.021)

- Current authority: user's "continue until complete" and `docs/UART_DEBUG_PLAN.md`. A/B1 already existed and were preserved; delivered all remaining accepted SOFTWARE features. The roadmap now has no unchecked feature item. Excluded firmware-specific recording triggers/network login/implicit target reset remain excluded. Hardware acceptance is separate NOT RUN, not a reason to claim a real UART test occurred. Do not restart A/B1/B2/C or treat older handoffs' "remaining" lines as current status.
- Added a separate advanced-tools window (UART menu) with Offline replay, Profiles, Chart analysis and Automation. Keeps main tabs simple. `docs/ADVANCED_UART_TOOLS.md` is the detailed Traditional Chinese user guide; README/AGENTS/MFC_BUILD/SELF_CHECK/plan align with delivered behavior and limits.
- Replay: background regular-file import, 64 MiB / 200000 scheduling-record cap, paused load, Play/Pause/speed/record seek and explicit source provenance. Validates adjacent recorder index for .bin receipt timing; plain/unindexed bytes use synthetic intervals without fake display/export clock labels. Uses existing parser/filters/capture/charts, never live recording/hardware statistics/TX. Virtual chart clock survives selection/resize/Snapshot/window changes. Seek resets the view; oversized numeric lines pause and resynchronize at LF. Unload without offline data preserves live capture.
- Profiles: named portable .sccprofile + validated import/export, preview/staging and explicit idle-only Apply. Fixed schema/ranges/encoding/allowlist; no hardware-open paths/commands/history/recording data. Background merged atomic INI commit preserves unrelated keys. Lock only main settings child controls/menu commands, keeping main close/cancel available; cancellation keeps a visible waiting surface and cannot undo a committed save. Confirmed imported port/settings are explicit preferences, not proof of successful connection. Main startup still never auto-opens hardware.
- Analysis: retained snapshot chart with capture-relative axes, left/right mouse-message A/B and numeric entry, dT/dY and inclusive sparse min/max/mean/RMS/population SD, optional in-range Y reference and view-end/window navigation. Empty intervals are n/a. This does not disturb live chart axes/selection.
- Automation: explicit confirmed, finite Periodic/Batch/Auto reply, literal raw Text/HEX patterns, endings and binary Sum8/XOR8/CRC16-Modbus before suffix. One at a time, bounded intervals/counts/timeout retries and one-hour cap. Failure/partial acceptance stops without retry; only explicitly selected reply-timeout retries resend. Manual Console TX/control/Break is blocked while running; Stop/window close/disconnect cancels pending automation and never resumes after reconnect. In-flight effects remain uncertain/non-revocable. No command/arming persistence.
- Main changed modules: uart_files/background_job, uart_offline, uart_profiles/uart_profile_ini, uart_automation, chart_analysis, advanced_tools, plot_view_analysis and four main-dialog composition units. Integrated existing main timer/menus/close/receive/settings/recording/Messages/plot/queue paths; updated CMake/resources/VERSION/ignore. No dependency/Python/clean implementation change. New core/runtime fixtures and focused selectors are recorded in SELF_CHECK.
- PASS: Debug and canonical Release builds, bundled native core 4/4 each, final 0.34 / 0.32 s; final Advanced actual-control fixtures in Debug / Release Font125, inspected enlarged-font screenshots at minimum height 900 and widths 1050/1400. Earlier in this turn selected Debug Serial/Uart/UartOperations/Charts regressions passed; unchanged-path evidence reused after the final profile-only UI/layout correction. Twenty-two current reports have zero FAIL. Read-only static clean audit 88 PASS. No full All/Python or actual cleanup/purge. SELF_CHECK maps changes to tests and names all NOT RUN boundaries.
- Preserved first missing-header compile failure as `build/verify/advanced-build-debug-initial-header-failure.txt` and initial visually incomplete page screenshots/results as `advanced-before-tab-focus-fix`. Corrected include, tab selection/focus, child/sibling clipping and redraw; final visibility/caption/geometry and inspected PNGs pass. Final review fixed unsafe no-source Unload, oversized-line tail parsing, INI injection/no-port preferences, and cancellation/lifetime issues; no known high/medium issue remains in this scope.
- Canonical EXE `build/SimpleComChart.exe`: `1.0.1.021`, SHA-256 `083D48B9E4716B0A18A6A21BB28509FCA9C0CAC432D5144B9B0F7F5856DC215A`. Personal INI entry/final `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`, unchanged. Exact-path preflight found no production process to close. Work remains local/uncommitted, no staging/push/private-data publication. Physical UART/pins/Break/timing/USB/throughput, actual DPI, physical-mouse file dialogs and stalled-filesystem cancellation remain NOT RUN. Font125/simulated transport/WM mouse dispatch are software evidence only.

## Approved UART expansion - batch B1 (2026-10-24, 1.0.1.020)

- Continued from the existing uncommitted A implementation, not from HEAD. This checkpoint delivers B1 only: TX / flow-lines / statistics. B2 offline import/replay and named profiles, and C chart analysis/explicit automation remain unchecked in `docs/UART_DEBUG_PLAN.md`. Resume at B2 without redoing A/B1; the accepted remaining scope needs no new approval. Event-specific trigger recording and network login remain excluded.
- Added a separate owned/modeless **UART > Console / Serial options...** window, retaining the three existing main tabs. Literal UTF-8 Text or HEX, None/CR/LF/CRLF suffix, whole-payload 1..4096-byte validation, Send/Ctrl+Enter only, 32 history entries and 8 explicit preset slots. Selection/load never sends. Private history/presets are encoded in portable INI, not encrypted. TX direction/results carry target and operation ID, distinguish partial driver acceptance from device ACK, and are never fabricated as RX in Messages/raw `.bin`.
- Added None/RTS-CTS/XON-XOFF/DSR-DTR controls, manual DTR/RTS ownership, pin status and explicit approximately 250 ms Break. Idle Apply saves next-connect choices; live Apply/Break requires an owned cancelable warning, then validates the original connection generation again. Successful current-generation control completion persists one grouped configuration key and updates recovery settings. Malformed/truncated/unreadable control settings block Connect until reviewed/applied. Reconnect retains confirmed settings but never replays TX/Break. Pin-related reset/boot effects remain possible on open/recovery and require hardware acceptance.
- Independent RX/operations workers share a stable transport. Native I/O uses per-request overlapped storage/events; queue limits are 16 / 64 KiB. Normal disconnect/close cancels queued work and requests native cancellation, retaining the owner until both workers actually finish. GUI polling joins only finished threads; teardown-only lifetime joins remain. A defective driver can keep Stopping pending rather than permit unsafe handle/storage reuse. RX stays live during blocked TX; final reported RX bytes survive a simultaneous read error. Accepted TX prefixes are counted/reported once, not retried. Traffic rates are approximate trailing-one-second payload rates; error totals are driver error-bit observations, not precise physical event counts. Unsupported/disconnected pin state is n/a.
- Production modules: new `uart_operations`, `uart_preferences`, `uart_console`, `mfc_main_dialog_uart`, `serial_control_config`, `serial_manager_operations`, `serial_native_io`; affected `serial_manager`, `serial_connection`, `serial_open_job`, `mfc_main_dialog`/serial/settings and `uart_recorder` integration. Aligned resources, CMake and VERSION. Verification: new `tests/uart_operation_tests.cpp`, `tests/uart_native_io_tests.cpp`, `src/runtime_uart_operations.cpp`; affected receive/serial/runtime fixtures and runtime selector. AGENTS/README/MFC_BUILD/UART_DEBUG_PLAN/SELF_CHECK aligned. No Python/dependency/cleanup-script change, no unrelated refactor.
- PASS: Debug and canonical Release x64 builds, bundled native core 3/3 each; Debug `-Scope UartOperations`, Release `-Font125 -Scope UartOperations`, affected Debug `-Scope Serial` and `-Scope Uart`. Sixteen current scoped reports have zero FAIL. Inspected enlarged-font Console PNG; actual controls/captions fit widths 1000/1400. Tests use injected transports/isolated INIs, including confirmation ownership/generation changes, partial TX, RX during blocked TX, nonblocking stop, queue cancellation and error-plus-final-bytes. Private local named-pipe tests exercise real overlapped helper completion/cancellation without opening COM hardware. SELF_CHECK records commands and limits.
- Earlier existing serial/receive fixtures assumed synchronous PnP/capture shutdown and initially failed after the async-stop change. Preserved those results in `build/verify/uart-020-before-async-fixture-serial` and `uart-020-before-async-fixture-receive`; fixtures now pump until completion and selected checks pass. These are not hidden/claimed passing initial runs.
- PASS read-only workspace clean audit: 88 clean-related scripts, no known whole-build deletion pattern. No actual cleanup/preservation fixture rerun; compiler/test-flow additions do not authorize removing delivery/evidence files. Final documentation-only changes pass changed-text/reference/whitespace checks without another application build.
- Release output `build/SimpleComChart.exe`: FileVersion/ProductVersion `1.0.1.020`, SHA-256 `3D1510500DF4666D5DE68A6FCCFAE094FADC2949387322074FE371152E8BB986`. Current entry/final personal INI SHA-256 `CC075DB440BE0C08E0F4DDBA1BFBC6A4E72ADAA488456F23EE7261A0066E5A33`, unchanged by this work (the older A hash is historical, not this entry baseline). Exact-path Release/Debug preflight found no running production instance; none was closed/terminated. No clean/purge, commit or GitHub push. Full All/Python, physical UART/TX/pins/Break/USB/throughput, actual DPI switching and physical-mouse acceptance remain NOT RUN.

## Approved UART expansion - batch A (2026-10-24, 1.0.1.019)

- Canonical Release EXE: FileVersion/ProductVersion `1.0.1.019`, SHA-256 `7CF756BAA8A0B69B488D074EBCF9479951BC4BC107D435A741052CEB7283D504`. Display-only final report explicitly excludes recording/serial/export suites; the corresponding actual checks remain in the separate Uart reports.
- User approves all proposed UART features except event-specific trigger recording. The whole scope is NOT finished: batch A (recording/receive display/search) is implemented, while B (TX/Flow Control/statistics/replay/profiles) and C (chart analysis/explicit automation) remain unchecked in `docs/UART_DEBUG_PLAN.md`. Resume at the TX transport/worker contract, preserving the new receive pipeline; no new approval is needed for the already accepted items, but do not silently implement excluded firmware-specific triggers or network login.
- New `UartRecorder` receives exact ReadFile bytes before the bounded pending UI buffer, using one background disk worker. Separate CREATE_NEW `.bin`, `.log`, `.idx` files, size/local-date rotation, explicit queue/disk/flush errors, partial-file preservation, stop/drain and deferred normal close. A recorder input gap stops visibly. Index events never contaminate raw bytes; readable continuous logs deliberately escape non-ASCII/control bytes. Recording is independent of filters, display/Clear, retention and reconnect waits; it does not connect hardware or send commands.
- Messages adds UTF-8/ANSI with code-page selection, PC receipt clock/elapsed/delta, CR/LF/CRLF, wrap, foreground SGR, display pause, search/case/limited safe Regex/highlights and session bookmarks. Shared pure formatting plus bounded receipt metadata preserve raw bytes and immutable Save As behavior; advanced Unicode snapshots use UTF-8 without BOM. Optional system RichEdit does rich display only, while default ASCII/HEX keep the original EDIT. Options persist in INI; search/bookmarks/pause do not. Main pretranslation routes keyboard commands before MFC dialog handling. Settings/Help menu order remains intact.
- PASS: Debug/Release builds, bundled core 2/2, focused Uart runtime (Debug normal and Release enlarged fonts), startup/menu checks for Messages in both configs, and final display-only enlarged-font fixture. Added recording integration proves exact >256 KiB raw recording before hidden UI delivery, independent of rejecting filter/Clear, and close drain protection. Final review has no known high/medium issue in this batch; limitations are documented. SELF_CHECK lists exact affected files, commands, fixtures and NOT RUN boundaries.
- Preserved early failing test state and screenshot under `build/verify/uart-before-fixture-isolation`. Follow-off leakage into older tests and fixed-Y screenshot placement were fixture issues; corrected state isolation/placement and reran only relevant checks. Final display-only selector/report must not claim unrelated recording/serial/export checks ran.
- No matching production EXE was running at build preflight; no user process was closed. Personal INI SHA-256 `43FD93E3D2C8FB4D4829BADDBA5BC649428FD3B185582F7E2C1DE6F1F88C8E9F` is unchanged. Static clean audit 85 PASS; no actual purge/clean, no Python changes, no commit/push. Hardware UART/side-band/timing/power-loss, actual DPI and physical-mouse dialog checks remain NOT RUN. Project remains local/uncommitted.

## Selectable startup tab (2026-10-22, 1.0.1.018)

- User requests a configurable Messages/Chart/4 Charts startup page, saved in INI. Added native Settings > Startup tab radio choices; default/fallback remains Chart. The confirmed named token is saved immediately to `[UI] StartupTab=Messages|Chart|4Charts` and applies next launch. Normal tab clicks do not replace that preference. Choosing a default does not switch the live page, clear capture or open a port. Failed writes keep the previous confirmed choice and show an error.
- New `mfc_main_dialog_preferences.cpp` owns loading, menu checks, safe key-only writes and startup application. New-file UTF-16 BOM uses CREATE_NEW to avoid truncating an existing INI; existing unrelated keys remain. Main init loads the choice separately from capture settings and applies page visibility/focus before first show. RC/resource IDs, private declarations/commands, Help, VERSION/CMake and project docs aligned; no extra toolbar controls/dependencies/Python change. Earlier dirty work/user JPGs retained.
- New `runtime_startup_tab.cpp`, `--smoke-test=startup` and runtime script `-Scope Startup -StartupTab Messages|Chart|4Charts` use separate test-owned INIs/fixtures. Full All scope now includes the regression and uses saved startup state before explicitly entering Chart for older chart tests; full All execution was NOT RUN this turn. The script canonicalizes case-insensitive input names to the stable stored tokens.
- PASS: final Debug and canonical Release builds, no reported compiler warning/error; bundled telemetry_core 1/1 (0.11 / 0.15 s). Focused Startup only: six actual process launches, all three initial pages in both configs; final three-case totals Debug 9.6 s and Release 10.9 s. All 12 current scoped report files have no FAIL. Cases cover initial selected/focused page/control visibility, native menu radio/command routing, immediate persistence/reload, data/current-page preservation, ordinary tab clicks, missing/invalid fallback, UTF-16 BOM, unrelated-key retention and read-only failure/attribute restore. Inspected Release Messages and Debug 4 Charts startup PNGs.
- No Serial/All/Python/chart/export/font suites, extra font variants or clean audit rerun; their unaffected earlier evidence is reused, not marked newly executed. No real COM open or hardware action. Native menu physical-mouse traversal/monitor-DPI switching NOT RUN; startup page evidence comes from real isolated MFC launches and native control/menu assertions.
- Preflight normally closed only canonical EXE PID 88924, created 2026-10-22 18:04:48; no force/unrelated closure. Personal INI unchanged at entry/after-close/final SHA-256 `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F`. No user default was chosen on their behalf; normal unconfigured startup remains Chart until they select a menu option. Final EXE 1.0.1.018, 3271680 bytes, SHA-256 `D27F47D96550A709F01F2F9AE67A1E46FFB234DB8158F99BB78B5E3BE895D47C`. Local/uncommitted, no deletion or GitHub push.

## Actionable manual Connect failure (2026-10-22, 1.0.1.017)

- User reports that opening Tera Term first makes Connect appear unresponsive, without visible guidance. Existing code reported open failure only in the bottom status/log. Added a single owned native OK/warning MessageBox for each failed manual Connect, including requested PORT, original error and steps to disconnect Tera Term/PuTTY/duplicate tool, check USB/Device Manager PORT and serial parameters, then retry. Busy/unavailable is phrased as a possibility, not a detected process owner. No other application is closed; automatic retries and cancelled late completions stay non-modal.
- Changed `mfc_main_dialog_serial.cpp/.h` manual failure handling/notice dispatch, `serial_manager.cpp` OS open-error number plus available FormatMessage description, and `mfc_main_dialog.cpp` timer guard after a modal loop can close its owner. Failure restores idle controls and closes rejected candidates before the prompt, without clearing capture or replacing last successful settings. Validation and pending-old-open errors use the same guidance. The warning dispatcher is copied locally before invocation and no member is touched afterwards. Existing connection layout/retry semantics, Python and unrelated changes retained.
- Extended fake fixture/runtime serial assertions: simulated access denied `(5)`, target/original reason/check text, owner/title/flags, controls idle before prompt, one warning only, success/no extra warning, automatic retry/no warning, cancellation/no late warning, invalid selection/settings, pending-worker explanation and subsequent successful retry. Dispatch is intercepted in automated tests, so native popup rendering/manual clicks and real Tera Term occupancy are NOT RUN. Exact ASCII body is retained in `manual-connection-warning.txt` using binary output to avoid CRCRLF translation. No real COM open or hardware action.
- PASS: final Debug and canonical Release builds, no reported warning/error; existing entry also bundles telemetry_core 1/1 (0.26 / 0.21 s). Focused runtime only: Debug `-Scope Serial` 6.8 s and Release `-Scope Serial` 7.0 s; six current scoped reports contain no FAIL, including existing connection geometry/identity/recovery/cancel checks. No full runtime, enlarged-font, Python, chart/export/font suite or clean audit rerun; unaffected earlier evidence is reused, not newly claimed. Native popup visual acceptance and OS driver failure/FormatMessage behavior with actual Tera Term/USB are unverified.
- Pre-build normal shutdown only for full-path canonical EXE PID 71784, created 2026-10-22 17:47:14; no force/unrelated closure. Entry/after-close/final personal INI SHA-256 stays `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F`. Final `build/SimpleComChart.exe`: 1.0.1.017, 3257344 bytes, SHA-256 `24360CB7CB55E5C7070858ED6B12C26542F7D28297E7FAFBE447E773ABE511B6`. No deletion, commit or push. Project docs and shared actionable-manual-error rule aligned; see SELF_CHECK for selected verification and gaps.

## Connect redraw/geometry correction (2026-10-22, 1.0.1.016)

- User reports visual noise/object redraw when pressing Connect. Code inspection found `update_connection_ui` calling the whole dialog's `layout_ui` at every opening/success/wait/idle transition; that layout measures the current short/long button text and calls synchronous `MoveWindow(..., TRUE)` on all top-level children. A new before-fix probe confirmed geometry changes and 144 unrelated position events per 12 state updates, at both tested widths on each page. This proves the unnecessary layout path, not a reproduction/video of the user's exact visual symptom. Failed baseline reports/settled images retained at `build/verify/connection-redraw-before-1.0.1.016/Debug-serial`.
- Fix: `mfc_main_dialog_ui.cpp` measures and reserves the widest Connect/Cancel/Disconnect caption with the live button font; COM width uses actual fixed serial-row widths/gaps so Stop still fits the minimum/enlarged-font layout. `mfc_main_dialog_serial.cpp` updates only changed caption/enabled states and invalidates the owner-drawn button without background erase; no whole-dialog layout on state changes. Connection/open/identity/INI/parser/export semantics unchanged. Prior dirty-tree work and user JPGs retained.
- Added `runtime_connection_redraw.cpp`, declaration/CMake and `--smoke-test=serial` dispatch. `scripts/verify_runtime.ps1 -Scope Serial` selects only connection geometry plus the existing fake serial persistence/reconnect/cancel checks, under separate `*-serial` fixtures. Normal All scope retains prior behavior plus the new regression, but was NOT RUN this turn. Help/feature behavior does not change; VERSION, project docs and the shared state-only UI-update rule are aligned.
- PASS: final Debug and canonical Release builds, with the existing one-click entry's bundled single telemetry_core target 1/1 (0.12 / 0.16 s), no reported warning/error. Native core is bundled by the existing build entry, not a separately selected broad runtime regression. PASS focused runtime only: Debug normal `-Scope Serial` in 4.8 s, Release `-Font125 -Scope Serial` in 5.2 s. Each checks 72 state updates across all three pages and widths 1120/1500: stable rectangles, all captions fit, correct enabled state, final serial row fits and zero unrelated position events (before: 144 per matrix cell). Six current focused result reports contain no FAIL; before-fix FAIL is intentionally retained separately.
- Inspected Release Font125 Chart opening and Debug Messages opening PNGs; these are settled PrintWindow evidence, not transient flicker capture. Real hardware connection/noise, physical unplug/reinsert and actual monitor-DPI switching NOT RUN. Fake serial tests make no real COM open. No unrelated Python/chart/export/font-suite rerun or clean/audit rerun; their unaffected earlier results are reused, not relabeled as new PASS. Build intermediates remain ignored/regenerable; no files purged.
- No matching canonical/Debug output process needed closure this turn. Personal INI was already SHA-256 `CF8624EEA1700A14970846234E2B2E6262ED3D7D5FE9FE76171D8D429A17773F` at entry and remains unchanged; do not restore older records' settings. Final canonical EXE 1.0.1.016, 3247616 bytes, SHA-256 `86EE087702BBA5300A5C28299E299D4D2D7159A9C5D72013F9BB9D36A1F35315`. Local/uncommitted, no push. See SELF_CHECK for focused reproduction/verification commands and current limits.

## Remembered serial settings and bridge recovery (2026-10-22, 1.0.1.015)

- Request: remember the connected PORT in INI and recover when the bridge disconnects. Native `[Serial]` now saves the last successful Port/DeviceInstanceId/Baud/DataBits/Parity/StopBits immediately. Startup restores selections only, not hardware access. Failed choices do not replace the saved working configuration; arbitrary INI interface/file paths are ignored. Python frontend unchanged.
- Added `serial_connection`, `serial_open_job`, `mfc_main_dialog_serial` and `mfc_main_dialog_serial_settings`; adapted SerialManager and main-dialog timer/worker/settings integration. Read error or PnP removal waits for the original unique instance/interface, including COM renumbering, never a reused COM number/other first port. Retry frequency is bounded (2/4/8/10 seconds maximum, 1-second presence scan); logs/status are non-modal with repeated-error rate limiting. Recovery preserves samples/capture/raw history/choices. Manual Disconnect/Cancel stops retries; one pending background open cannot retain a dialog, and late successes are closed/discarded. Manual successful Connect still starts fresh. Unknown/ambiguous identity/interface requires manual reconnection; quiet RX and scan failure are not disconnects.
- Safe tests added in `tests/serial_connection_tests.cpp`, `runtime_serial.cpp` and `runtime_serial_fixture.h`, using injected fake devices and fixture-only INIs; no real COM open/transmit/reset. Existing preferences/font fixtures restore serial preferences after their own loader tests. CMake/test entry, Help, VERSION and project docs aligned; shared recovery rule added. Previous local feature work and user JPGs retained; no dependency added.
- PASS before the subsequent change-impact testing instruction: final Debug and canonical Release builds, CTest telemetry_core 1/1 each (no reported compiler warning/error), Python 5/5 and all four native runtime variants. Final script-inclusive times: Debug 13.5 s / Font125 13.4 s; Release 13.8 s / Font125 16.7 s. All 52 current runtime reports contain no FAIL. Serial fixtures cover remembered config/no startup open, failed/manual connection, read/PnP loss, occupied/reused/renumbered ports, history preservation, single pending worker, cancellation and late completion; core also covers bounded backoff, ambiguous identity, invalid settings and owner-close. Debug waiting and Release Font125 reconnected screenshots inspected. Physical bridge unplug/reinsert, actual COM opens/driver effects and actual monitor-DPI switching NOT RUN.
- Preflight normally closed only canonical EXE PID 83724 (created 2026-10-22 15:01:26), no force/unrelated closure; later build checks found no matching running output. Personal INI unchanged at SHA-256 `52117B8466BEB99BA29174CADB2E49C299F16FD5914AB808965445B5AF9AA12C`. Final actual clean removed 176 regenerable compiler intermediates; all 749 protected build files unchanged by SHA-256. Expanded 23-sentinel/source/help/repeat/cache/native-and-cache-junction tests and static audit of 85 scripts PASS. Clean preserves delivery/runtime data and evidence.
- Final canonical `build/SimpleComChart.exe`: FileVersion/ProductVersion 1.0.1.015, 3237888 bytes, SHA-256 `A7862D7199893D7B2E864A8A5838C6B8E36B213434700746C532153A911EE03B`. Local/uncommitted; no GitHub push. Adapter/target resets caused by existing DTR/RTS-enable behavior remain a hardware risk to verify, not an automatic reset-command feature.
- Follow-up user policy: only modified/affected behavior should be tested, not the whole checklist. Shared AGENTS/WORKSPACE_RULES/SELF_CHECK/README/HANDOFF and this project's verification instructions now say change-impact selection, reuse unchanged evidence, explain scope expansion, and no app rebuild/tests for docs-only edits. The already-finished runs above predate that instruction; they were not repeated for this document update. Future MFC-only edits do not trigger unrelated Python tests or every font variant. Existing monolithic smoke-test runner has not been redesigned in this documentation-only change; report its bundled coverage honestly.

## Messages toolbar alignment (2026-10-22, 1.0.1.014)

- User's screenshot exposed vertical misalignment between Font/point text, +/- buttons and the existing Messages row. Earlier checks covered width/non-overlap but not actual visible heights or text centering. `message_page.cpp` now uses the native combo's closed selection-field height for all eight row controls, with one top/bottom/center line inside the existing row slot; Display/Font/point static text uses vertical centering. Dropdown-list space remains separate. No font-size, INI, capture/export, parser or Python behavior change.
- Updated `runtime_message_font.cpp` to check all eight control rectangles and centered static styles at 1120/1500 widths and body sizes 8/10/32 pt, plus `messages-toolbar-aligned.png`. VERSION, project docs and shared UI-baseline/handoff rule updated. No new dependency, previous local work/user JPGs preserved.
- PASS: Debug and canonical Release builds, CTest 1/1 each, Python 5/5, whitespace, all four final standard 30-second runtime-script variants. Measured final runs: Debug 19.6 s / Font125 16.9 s; Release 14.8 s / Font125 15.2 s (script-inclusive elapsed times). Actual alignment assertions and existing font/INI/data/export checks PASS. Inspected Debug normal and Release Font125 aligned-toolbar screenshots; labels, field, checkbox and buttons are aligned. Physical mouse/COM/actual monitor-DPI switching NOT RUN; Font125 is an enlarged-control-font fixture.
- Verification uncertainty retained: two initial Debug runs hit the unchanged 30 s harness deadline at different progressing stages (message export, then entry to font checks), not a recorded failed alignment assertion. Their report/image snapshots are under `build/verify/messages-row-timeout-1.0.1.014` and `attempt2`; unreached report files may be retained from prior runs, so use timestamps. An unchanged Debug EXE then passed a one-off 60 s diagnostic run in 21.6 s, followed by the final four standard-script PASS runs. Timeout cause is NOT confirmed; no application or harness-deadline change was made to hide it. Earlier 1.0.1.013 toolbar image retained under `build/verify/messages-row-before-1.0.1.014`.
- Preflight normally closed exact canonical EXE PID 85740; no force/unrelated closure. User INI was already SHA-256 `52117B8466BEB99BA29174CADB2E49C299F16FD5914AB808965445B5AF9AA12C` at entry and remains unchanged; do not restore the older 1.0.1.013 baseline. Final actual clean removed 160 regenerable compiler files, all 685 protected build files unchanged by SHA-256; expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction tests and static audit 85 scripts PASS.
- Final `build/SimpleComChart.exe`: FileVersion/ProductVersion 1.0.1.014, 3182080 bytes, SHA-256 `74F6BBF1DD0717084C9323AE9647A873CC793CD088296A1EF5A647FF7643B798`, preserved after clean. Local/uncommitted; no GitHub push.

## Messages font-size handoff (2026-10-22, 1.0.1.013)

- User requested quick font-size -/+ controls inside Messages and INI persistence. Added neutral-grey buttons, Font label/current point-size readout, 1 pt steps, 8..32 pt limits and 10 pt default when missing. Only the Consolas message body changes; ASCII/HEX share the selected size, toolbar/chart fonts and message/export content stay unchanged. Buttons disable at their respective limits.
- `message_page.cpp/.h` owns safe font replacement and scroll/follow preservation, with a parent notification only for user button changes. Main dialog routes that notification through existing validated settings saving; `[Messages] FontSizePt` is saved immediately and restored by normal `load_settings`, including hidden-page startup. Invalid/out-of-range values clamp safely. No new dependency or Python frontend/parser/export-format change; earlier local work and user JPGs preserved.
- New `runtime_message_font.cpp`; CMake, main dialog header/map/settings, runtime sequence, Help, VERSION and project docs updated. PASS: Debug and canonical Release builds, CTest 1/1 each, Python 5/5, all four normal/Font125 runtimes, whitespace and final report scan (no FAIL). Tests use actual BM_CLICK / BN_CLICKED routing, assert real HFONT height changes and immediate INI writes, exercise real load_settings restoration/default/invalid/oversized values, unrelated-key preservation, ASCII/HEX/data/export isolation, follow off/on, repeated replacement GDI counts and resized button bounds. Debug and Release Font125 24 pt ASCII/HEX images inspected. Long HEX lines legitimately use the existing horizontal scrollbar.
- Runtime font tests write only test-owned `build/verify/.../message-font-test.ini`; canonical personal INI is SHA-256-identical at `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447`. Startup/restore tests exercise the real loader in-process, not manual close/relaunch acceptance. Physical mouse interaction, COM timing and actual monitor-DPI switching NOT RUN. The screenshot's unsupported-export-extension status is deliberate residue from the earlier negative export fixture, not a failed font test.
- Preflight normally closed exact canonical EXE PID 81716 (`D:\SourceCode\python\simple_com_chart_gui\build\SimpleComChart.exe`), no forced/unrelated closure. Final clean removed 160 regenerable compiler intermediates, preserving all 604 protected build files by SHA-256; expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks PASS. Static workspace audit 85 scripts PASS. EXE/INI/log/data/evidence retained.
- Final `build/SimpleComChart.exe`: FileVersion/ProductVersion 1.0.1.013, 3178496 bytes, SHA-256 `C179B37988D9414A5414590F572C53CDC0FB62AD035F099D9D4BE3ADD40BEB31`, unchanged after clean. Local/uncommitted; no GitHub push.

## Messages Save As handoff (2026-10-22, 1.0.1.012)

- User approved adding Messages Save As after the 1.0.1.011 local intermediate build below. Save As now works on every tab: Messages saves the currently selected ASCII/HEX text representation to `.log` (default) / `.txt` only; Chart/4 Charts keep their existing filtered capture formats. Editable default filename remains local PC `yyyyMMdd_HHmmss.log`, generated at click. No extra header/BOM/newline, no filter dependency, numeric rows not required. Snapshot after filename confirmation includes retained raw history, not only the visible screen; lost/evicted bytes cannot be recovered, and this is not continuous/binary recording.
- `message_page.cpp/.h` exposes formatted snapshot text/mode; dialog/UI/command/completion routing updated. Extracted the existing atomic destination writer in `capture_export.cpp/.h` and reusable export-job scheduling in `mfc_main_dialog_export.cpp` so both sources share safe replacement, busy/close guards and error reporting. Later receive/mode/Clear/tab changes cannot alter a queued snapshot. No new dependency or Python frontend change; earlier local modifications/user JPGs preserved.
- Added `runtime_message_export.cpp` and native export assertions; updated CMake, runtime tab-state expectations, Help, VERSION and project docs. PASS: final Debug and canonical Release builds, CTest 1/1 each, Python 5/5, all four Debug/Release normal/Font125 runtimes, whitespace and final report scan. Runtime checks cover no numeric capture, exact ASCII/HEX, empty history, immutable snapshots, concurrent-save rejection, tab busy states, close guard, locked-file preservation/replacement, extension failure and retention offsets. Existing chart log/TXT/CSV/CVS/BIFF8 XLS tests still PASS.
- Inspected Release Font125 Messages screenshot (Save As enabled), Debug/Release ASCII/HEX export artifacts and final reports. The screenshot's `Save failed: Messages supports .log and .txt only` status is from the deliberate negative extension fixture, not a failed suite. Actual file-dialog mouse interaction, four-button cursor dwell, physical COM timing and actual monitor-DPI changes remain NOT RUN: computer-use native helper unavailable (os error 2), as recorded below. No alternate UI automation used or claimed.
- PASS clean: final actual clean removed 158 regenerable compiler files, preserving all 572 protected build files by SHA-256; expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks PASS. Shared static audit 85 scripts PASS. Canonical user INI remains `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447`. No additional output-path processes needed closure after the earlier graceful PID 72408 shutdown.
- Final delivery `build/SimpleComChart.exe`: FileVersion/ProductVersion 1.0.1.012, 3165184 bytes, SHA-256 `8B000164800CD7A0804AEC2215F38C86E373B532518825FAC9543D4FF71EE682`, unchanged by clean. Local/uncommitted; no GitHub push.

## Button tooltip routing follow-up (2026-10-22, 1.0.1.011)

- User reports label/filter/Auto Refresh help works, but the four Chart buttons do not show help. Code inspection found all child input was rewritten into parent-only rectangle events. Replaced this with real child-HWND tools (TTF_IDISHWND / TTF_SUBCLASS) plus separate parent-area fallback tools for transparent/disabled children. Removed the custom PreTranslateMessage relay; preserved text, delays, wrapping, compatible TOOLINFO size, command behavior and earlier uncommitted work. Ten logical targets now have 20 native registrations.
- Changed `src/hover_help.cpp/.h`, `src/mfc_main_dialog_help.cpp`, `src/mfc_main_dialog.h`, `src/runtime_ui_help.cpp`, VERSION, AGENTS/README/MFC_BUILD and these verification docs. Shared workspace lifecycle/handoff documents now explicitly distinguish actual cursor-dwell acceptance from forced popup rendering. No parser/export/Python behavior change. User's untracked tab screenshots remain untouched.
- PASS: Debug and canonical Release builds, CTest 1/1 each, Python 5/5, all four normal/Font125 runtime variants and whitespace checks. Updated UI-help tests check each real HWND/subclass registration, separate fallback flags/geometry after resize, unchanged capture after button mouse messages, text/delay and native popup rendering/cleanup. Debug/Release Font125 popup/page images inspected. No final report contains FAIL.
- Actual mouse-dwell acceptance NOT RUN: computer-use skill's native helper reports `Computer Use native pipe is unavailable ... os error 2`; retry and JS-reset/reinitialization also failed. No alternate UI automation was used. The code-level event-routing mismatch is addressed, but do not describe the reported physical-hover symptom as reproduced or visually proven fixed. Physical COM timing and actual monitor-DPI changes also NOT RUN.
- Pre-build normally closed exact canonical EXE PID 72408 (full path `D:\SourceCode\python\simple_com_chart_gui\build\SimpleComChart.exe`); no force/unrelated closure. Entry user INI SHA-256 `8FCF58BD2EE982989A0BFFEC2D620DFF622DFE781A839CD732422E11C564D447` remains unchanged, not restored to an earlier revision's settings.
- PASS clean: actual clean removed 156 regenerable compiler files; all 516 protected build files SHA-256-identical. Expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks PASS; static audit 85 scripts PASS. Final EXE 1.0.1.011, 3141632 bytes, SHA-256 `A242170A6F07D20CD82D75F03742E4886C598F51CF8E9B7CBB4D5E2782AA0801`. No commit/push.
- New user question: Messages cannot Save As because both the UI and command path deliberately restrict export to filtered Chart/4 Charts capture; raw bytes are separate. Explained the current limitation and asked whether to add independent current ASCII/HEX `.log`/`.txt` export with timestamp filename. No reply/implementation yet; Messages Save As remains disabled in this delivery.

## Latest tab/hover handoff (2026-10-22, 1.0.1.010)

- Continued the existing local/uncommitted 1.0.1.009 changes without reverting them. Order is now Messages / Chart / 4 Charts, but startup explicitly selects Chart. Named IDs replace positional assumptions throughout UI/export/runtime checks. Selected tabs use dark-grey/white/underline; Messages ASCII and HEX content use green on black.
- Removed only the permanent Include/Exclude help paragraphs; applied/error status remains visible and wrapping. Native hover help covers the four Chart buttons, Auto Refresh label/combo and Include/Exclude labels/fields. Auto Refresh help explicitly means periodic Chart sample clearing, not receive frequency, and explains capture/Message preservation. No parser/export/Python frontend/dependency change.
- New `hover_help.cpp/.h`, `mfc_main_dialog_help.cpp` and `runtime_ui_help.cpp`; updated dialog/message/theme/test integration, Help/VERSION/CMake and local docs. Registration initially failed with the newest TOOLINFO size under the current native control activation. The compatible V1 prefix (same old-prefix strategy as installed MFC tooltip code) fixes registration without a global manifest/theme change. Failed reports/images retained in `build/verify/tooltip-registration-before-1.0.1.010`.
- PASS: final Debug and canonical Release builds, CTest telemetry_core 1/1 each; all four normal/Font125 native runtime variants, Python 5/5 and patch whitespace. The native tooltip is non-animated/non-fading; final popup captures are fully painted/readable. New `ui-help-result.txt` checks startup/order, notifications, page-specific controls, selected-tab/message colors, ten tooltip targets/text/resized hit rectangles/delay, child mouse relay without capture mutation, popup rendering/hide/cleanup. Debug and Release Font125 green-message/selected-tab/popup screenshots inspected. Existing capture/sort/axes/settings/export checks remain PASS.
- PASS: actual clean removed 156 regenerable allowlisted compiler files, preserving all 504 protected build files by SHA-256. Expanded 23-sentinel/source/help/repeat/Python-cache/native-and-cache-junction checks and workspace static audit of 85 scripts PASS. No EXE/INI/log/data/evidence purge.
- Final `build/SimpleComChart.exe`: FileVersion/ProductVersion 1.0.1.010, 3141120 bytes, SHA-256 `38C507D779D333047B05690740893239E0735A14E07CE50EDAEE45549E892B0A`, unchanged after clean. Local/uncommitted, no GitHub push. Physical mouse dwell, actual monitor-DPI switching and COM timing/hardware are NOT RUN; native popup rendering is deterministic without moving the cursor. The isolated 1.0.1.009 whole-window repaint observation below was not reproduced in this revision and is not claimed root-cause-fixed.
- Preflight normally closed exact canonical Release EXE PID 28784; no force termination/unrelated closure. This turn's canonical user INI SHA-256 is `59A6CCC8D9F1BC1C4CF7808C810D0757F3E5A4BA88F04267A944CF9DA9ADA7B7`, unchanged by normal close. It differs from the older record because it was already changed at entry; it was not restored/overwritten.

## Latest UI/receive handoff (2026-10-22, 1.0.1.009)

- Started from clean user-committed `75a8694`; no prior work reverted. User additions in this task: raw ASCII/HEX Messages tab; rename second tab to 4 Charts; none defaults and remembered plot choices/reset; left-aligned content-sized table; local timestamp Save As name; neutral engineering-style toolbar. Python frontend and export formats remain unchanged. No new dependency or GitHub push.
- Raw ReadFile bytes feed a separate 256 KiB pending/UI buffer before line splitting/filters. ASCII escapes binary bytes; HEX retains CR/LF/NUL, mode changes keep history. Messages has Clear and default-on follow; Apply leaves raw data intact, successful Connect resets it, Disconnect drains queued bytes. Save As still exports filtered chart/data capture, not raw Messages.
- Four plots default none only without saved selections. Exact names persist in portable INI, wait for discovery, respect Max CH and survive Apply/Connect. Reset immediately persists four none choices, without deleting table/log data. Table columns use actual-font header/recent-value widths with bounded sampling, not equal viewport fill. Save As suggests local `yyyyMMdd_HHmmss.log` and remains editable.
- Toolbar now uses system-grey, normal text and straight borders. Press/execution uses one red accent; completion restores grey even for Snapshot/Overlay toggles. Connect remains red while connected and returns grey after disconnect/failure. Disabled controls take priority. Feedback uses per-HWND lifetime tokens rather than dereferencing an owner after synchronous dispatch; regression includes destroyed-source cleanup.
- PASS: final Debug build and canonical Release build, CTest telemetry_core 1/1 each, all four normal/Font125 runtime variants, Python 5/5 and whitespace review. Actual owner-draw pixel checks cover idle/pressed/executing/completed/disabled colors and preserved toggles. Final Debug/Release Font125 neutral-toolbar images inspected. Native raw/data/filter/sort/axis/export checks remain PASS.
- One final Release Font125 run had whole-window missing repaint/stale latest-label failures; the identical EXE subsequently passed that variant three consecutive times. Cause is NOT confirmed; no speculative application change was made for it. Failed report/images remain under `build/verify/release-font125-before-repaint-fix-1.0.1.009` (directory name records investigation context, not a claimed root-cause fix). Track if it recurs.
- Final actual clean removed 150 regenerable compiler files; all 436 protected build files remain SHA-256-identical, including EXE/INI/evidence. Expanded 23-sentinel/source/help/repeat/native-and-cache junction checks PASS; workspace static clean audit 85 scripts PASS. Earlier base clean removed 148 compiler files before the final toolbar rebuild; neither pass purged folders/runtime files.
- Final delivery: `build/SimpleComChart.exe`, FileVersion/ProductVersion 1.0.1.009, 3124736 bytes, SHA-256 `A51D4F1260EE0F76B30CEBCA2A3A57F4BB6144C333D88E3E90353106A4DD46CD`. Protected hash verified after clean. Changes remain local/uncommitted; no GitHub push.
- Build preflight gracefully closed exact canonical Release EXE PID 65124; no force termination/unrelated closure. Subsequent output-path checks found no matching instances. Canonical personal INI SHA-256 remains `05F7A8DB418DAB8A35B9F29FB26A2193F5A1C3C599CFEDDC96FB90C6B6E9BC16`. Physical COM/timing and actual monitor-DPI switching NOT RUN; connected palette is tested without opening a device.

## Latest Channel-sorting handoff (2026-10-21, 1.0.1.008)

- User requested clicking Channel to sort. Started from clean `17b3e8e`. Shared ChannelPanel now toggles natural case-insensitive A-Z/Z-A order with explicit visible direction text; P2 precedes P10, exact-name tie-break handles equal collation spellings. Both panels sort independently and retain session directions across new channels/Apply. No Value sorting, INI sort preference or chronological data-table sorting.
- `channel_panel.cpp/.h`: stable item IDs replace row-index identity for checkbox/value/color lookup; restore checked keys under suppressed notifications and batch new-channel sorts. Dialog header/runtime smoke add actual header mouse-message regression; Help, version and local docs synchronized. No parser/export/protocol/Python frontend/dependency changes.
- PASS: final Debug/Release canonical builds, native CTest 1/1 each, four runtime variants and Python 5/5; existing UI/capture/export checks remain PASS. New sorting evidence covers direction labels, numeric/case/tie order, identity preservation, live values/new channels, post-sort checkbox waveform control, All/None and independent second-tab column/value mapping. Initial native arrow was not visually clear, so final labels explicitly say Channel A-Z/Z-A. Final main/data screenshots inspected; details in SELF_CHECK.
- Under standing authorization, gracefully closed exact canonical Release EXE PID 75440 before build; no forced/unrelated closure and no Debug target running. Actual clean removed 134 regenerable compiler files; all 348 protected build files, including EXE/INI/evidence, retain SHA-256. Expanded fixtures and 85-script static audit PASS.
- Delivery: `build/SimpleComChart.exe` 1.0.1.008, 3076096 bytes, SHA-256 `A0DE7E4B08AF7B0D56651CF8FB73C4A5A27030BCC30BD5CE40FE7385C2D9C9B0`. Local/uncommitted, no GitHub push. Physical COM/throughput and actual monitor-DPI acceptance NOT RUN.

## Latest concise-hints handoff (2026-10-21, 1.0.1.007)

- User requested less on-screen prose. Shortened Include/Exclude hints to Set Regex, use `|`, and one example each; kept the separate status line and detailed Help/README semantics. Only hint strings, exact-caption runtime assertions, version and local docs changed in this revision; previous uncommitted features preserved. No filter/default-mode/plot changes.
- PASS: Debug/Release builds and CTest 1/1 each, four runtime configurations (normal/Font125), Python 5/5 and whitespace. Debug/Release Font125 screenshots inspected. Actual safe clean removed 134 regenerable compiler files, preserving 312 protected build files and the canonical INI hash; expanded fixtures and workspace 85-script audit PASS. No exact-path target process was running or closed.
- Delivery: `build/SimpleComChart.exe` 1.0.1.007, 3054592 bytes, SHA-256 `4E51DCC80CDA9158AA37E731DEAF0EEB49E8C7F623B0A977AACF4B6E4E3C33B7`. Physical COM and actual monitor-DPI acceptance NOT RUN. Local/uncommitted, no GitHub push.

## Latest filter-guidance handoff (2026-10-21, 1.0.1.006)

- User asked how to enter multiple Include/Exclude strings and requested separate status/Include/Exclude lines with examples. Inspected `line_filter.cpp` first: existing Regex alternation `|` already supports OR; Text uses one literal substring. Kept this grammar intact rather than treating comma/semicolon as new separators. Prior uncommitted 1.0.1.005 features remain preserved.
- Changed dialog filter/header/layout and `ui_metrics.h`: three separate font-measured/wrapping help controls, applied Text/Regex mode in status, examples `fan,|ac_adc_mv:` and `error|timeout`, explicit literal separator explanation. Invalid Apply changes status but retains both help paragraphs and existing capture. Help/README/AGENTS/VERSION and native/Python/runtime tests synchronized. No dependencies, filter parser or export-format changes.
- PASS: Debug/Release canonical builds, CTest 1/1 each; four complete native runtime checks (both configs, normal/enlarged fonts); Python 5/5 and whitespace. `filter-guidance-result.txt` exercises actual receive-path examples, priority/case/invalid recovery; extended layout checks cover widths 1120/1180/1500 on both tabs. Debug normal/enlarged and final Release enlarged-font screenshots inspected. Existing chart/data/export regressions remain PASS.
- PASS: actual safe clean removed 134 regenerable compiler files; 300 protected build files unchanged by SHA-256, 23-sentinel/source/help/repeat/native-and-cache junction fixtures PASS; workspace static 85 scripts PASS. User INI retains pre-build hash. No exact-path target was running, so no user process was closed.
- Delivery: `build/SimpleComChart.exe` 1.0.1.006, 3055104 bytes, SHA-256 `B95BA25CBCF0B7F5265FD75A49F91EEC58AEB0AB8BC6DC5626B16A3FE9421257`. This revision and the previous 1.0.1.005 remain local/uncommitted; no upload requested. Physical COM and actual monitor-DPI acceptance NOT RUN.

## Latest data-display handoff (2026-10-21, 1.0.1.005)

- User request: second-tab latest-point values, selectable table fields instead of expanding every field, and default-on name:value versus numeric-only labels. Entry worktree was clean at `08d159b`; prior 1.0.1.004 autoscale and font-lifetime fixes retained. No migration or publication requested.
- Implemented: `data_page.cpp/.h` reuses ChannelPanel as a separate All/None/checkbox selector, default first four table fields. Explicit visible-column indices preserve sparse cell mapping. Later fields stay unchecked after user selection; table display does not restrict capture/export or four-chart selections. The latest-value cache is restricted to accepted fields/Max CH, not arbitrary rejected input keys. Column changes preserve the inspected row when follow is off.
- Implemented: shared `plot_view_labels.cpp` draws latest dots and live labels on all four small plots, including singleton samples, with independent automatic Y scales. Show name beside the tabs controls both tabs, default checked; off shows only numbers. Main Overlay still controls first-tab label/marker visibility. Long names shorten without shortening numeric text; none/expired series have no stale label. Compact large Y ticks use scientific notation; X captions remain inside their bottom margin. Choices remain session-only.
- Files: data/plot/channel panel headers and implementations, dialog UI/command/header, resource ID, runtime smoke, help, CMake, VERSION and AGENTS/README/SELF_CHECK/build notes. No new dependencies or Python frontend/parser/export/protocol change.
- PASS: final Debug and canonical Release builds, CTest 1/1 each; four complete runtime checks (both configs, normal/enlarged fonts), Python 4/4, patch whitespace. New `data-display-result.txt` validates selection/mapping/blanks/full capture, both name modes, real painted label bounds/numeric text, per-receive updates, long names, none/expiry, reset and Max CH=2 against 100 distinct inputs. Final Release enlarged-font named and normal long-name screenshots inspected. Detailed commands/results/initial corrected build error are in SELF_CHECK.
- PASS: actual safe clean removed 134 regenerable compiler files while retaining 280 protected build files by SHA-256; 23-sentinel/source/help/repeat and both junction checks PASS; workspace audit 85 scripts PASS. EXE/INI/logs/evidence remain. User INI retains its pre-build hash. No exact-path target was running during build preflight; no user process was closed.
- Delivery: `build/SimpleComChart.exe` 1.0.1.005, 3043328 bytes, SHA-256 `EDECCD85E8E94FF03DBD36FE1D442E4FA95748C177082B6B9D19A9006447981F`. Changes are local/uncommitted, not pushed. Hardware COM/throughput and actual monitor-DPI acceptance NOT RUN; font enlargement is only a layout fixture.

## Latest Y-axis handoff (2026-10-21, 1.0.1.004)

- User authorized completing automatic Y scaling after review found the Chart page expanded only. Existing uncommitted 1.0.1.003 toolbar/font-lifetime work was preserved; no folder migration or publication was requested.
- `plot_view.cpp/.h`: automatic update and Fit now share range calculation over enabled samples in the displayed window, supporting expansion/shrink and negative/decimal values. Reset/empty selection defaults to 0..50; overlay headroom derives from current data rather than previous bounds. Frozen Fit uses snapshot data. Removed obsolete native temporary-fit timer/expand-only overlay helper and the caller in `mfc_main_dialog.cpp`.
- `runtime_smoke.cpp` and dialog header add actual-axis/filter/checkbox/window/Snapshot tests and `y-axis-result.txt` / `y-axis-201.png`; VERSION/help/AGENTS/README/SELF_CHECK synchronized. No new dependencies, parser/export changes or Python UI changes.
- Verification PASS: Release/Debug builds and native core tests; four full runtime self-checks (both configs, normal and enlarged fonts); Python 4/4. Observed 1000 -> filtered 200/201 shrink [199.8,201.2], stable overlay headroom, unchecked large channel, old-peak window removal, frozen Fit/Live, negative decimals, zero/empty and reset cases. Release 201-range screenshot inspected; hardware and actual monitor-DPI switching NOT RUN.
- Build preflight followed the new standing authorization but found no exact-path-matched Release/Debug process running, so no PID was closed; do not claim closure was exercised. Canonical output built normally, not in an isolated substitute directory.
- Actual clean PASS: 132 regenerable compiler files removed; all 221 protected build files unchanged. Expanded 23-sentinel/source/help/repeat and native/cache junction tests PASS. User INI hash preserved independently.
- Delivery: `build/SimpleComChart.exe` version 1.0.1.004, SHA-256 `D6C5D1B613DE1D5F2C756138A33A7C0E5DFF8310B62E5D9B23719E7CA4ECED95`. This and the earlier font fix remain local/uncommitted; no GitHub push performed.

## Latest toolbar hotfix handoff (2026-10-21, 1.0.1.003)

- Baseline at entry: clean worktree at `de486fd` (`update`), canonical 1.0.1.002 EXE running. User requested Auto Refresh / Time Window / Save As clipping fix, with screenshots. Preserve all prior features; this is not another layout migration or publication.
- Verified root cause: `draw_owner_button` deleted `ui_font_` on repaint, leaving invalid borrowed handles in static/button/tab/list controls. The previous startup capture/tests missed the lifetime issue. New regression on unfixed Debug failed with `live-fonts=0`; before-fix evidence retained in `build/verify/font-lifecycle-before-1.0.1.002`.
- Changed: `src/mfc_main_dialog.cpp` moves shared font deletion to teardown; `src/mfc_main_dialog_ui.cpp` makes the two toolbar unit labels single-line; header/runtime smoke adds repaint/font/text-bound regression; VERSION increments to 1.0.1.003. AGENTS/README/SELF_CHECK synchronized. Shared WORKSPACE_RULES adds generic borrowed-font lifetime/repaint-verification guidance; no project-specific rule added there.
- Verification PASS: Debug/Release builds and native tests; both runtime configurations with normal and enlarged-125%-font fixtures. At widths 1120/1180/1500, both tabs survive repeated explicit repaint with valid fonts and full Auto Refresh / Time Window / Save As / Include / Exclude / Apply text bounds. Corrected chart/data captures inspected. Existing capture/filter/export tests remain PASS.
- The user closed the old canonical EXE; normal Release build then updated `build/SimpleComChart.exe` successfully. No user instance was force-closed. Version 1.0.1.003, SHA-256 `DED9E95D4E3451B831072A7F602EA32A6A10B67E44E5BB01080A9FDD6F7EC33D`.
- Final actual clean/preservation PASS: 132 regenerable compiler files removed, all 195 protected build files unchanged by hash. Expanded 23-sentinel/source, help/repeat and junction tests PASS; workspace static scan PASS for 85 clean-related scripts.
- No data/parser/serial protocol changes, no folder deletion and no GitHub push. Actual monitor-DPI switching and hardware tests remain NOT RUN; font fixtures do not imply those checks.

## Latest feature handoff (2026-10-21, 1.0.1.002)

- Request completed locally for the screenshot's native MFC implementation: unclipped Include/Exclude, shared Save As, existing Chart tab plus spreadsheet-style Data / 4 Charts tab, frozen header/newest-bottom rows, default-on follow checkbox and four separately selectable/scaled/time-windowed charts.
- Changed modules: `mfc_main_dialog_ui` / `ui_metrics` (measured labels and tab layout), `data_page` (virtual table/four plots), `capture_store` (sparse retained rows), `capture_export` (text/CSV/BIFF8 and safe temporary replacement), `mfc_main_dialog_export` (file dialog/worker/close guard), compact `plot_view`, main/filter integration, help/version/CMake and tests. README/AGENTS/SELF_CHECK/build notes synchronized.
- The first page retains its controls. Refresh/Auto Refresh, Snapshot, Time Window and channel checkboxes do not erase capture or change the four small plots. Minimized capture no longer stalls the receive queue. Apply/connect reset capture; invalid Apply does not. Missing per-line values stay blank, not stale or zero.
- Capture/save contract: max 10000 rows / 32 MiB retained payload, discarded count visible. Save confirms a filename then snapshots retained data; it is not continuous logging. Default plain .log, optional .txt, UTF-8 CSV/.cvs alias and real BIFF8 .xls (254 numeric columns plus Row/Time). Background export leaves old destination intact on failure/lock/change and guards normal close while saving. New tab UI selections are per-session.
- Verification: Release/Debug builds and native regression PASS; four isolated MFC runtime checks (both configs, normal and enlarged-font fixtures) PASS; Python core 4/4 PASS; independent xlrd readback PASS including Unicode/blanks/negative decimals and 256-column SST continuation fixture. Post-clean normal Release/Debug runtime rechecks also PASS; dumpbin shows Windows system dependencies only. Detailed evidence/limits in SELF_CHECK.
- Screenshots: `build/verify/Release/chart-page.png`, `data-page.png`, and `Release-font125` counterparts inspected. Initial stock-font/compact Value-label appearance improved; Include fully visible. Actual monitor-DPI switching remains unverified; do not label font enlargement as physical DPI testing.
- Actual safe clean PASS: 132 regenerable compiler intermediates removed, 169 protected build files unchanged (SHA-256), 23 fixture sentinels plus source retained, native/cache junction rejection PASS. Static audit PASS for 85 scripts. No EXE/INI/log/export/evidence purge.
- Local delivery: `build/SimpleComChart.exe` version 1.0.1.002, SHA-256 `2B0D7FFB99D3943A5CC053FE29C4B7DDB1864F984077D4FBDAE9DE528994568B`. The Python frontend was deliberately not changed. No new runtime library/package requirement.
- NOT RUN: hardware COM soak, actual monitor DPI changes, manual Save As dialog clicks, Excel GUI compatibility acceptance and separate clean-PC deployment. Native file-format readback/worker/UI self-checks are not those manual tests.
- Publication: this feature revision is intentionally uncommitted/unpushed. Earlier 1.0.1.001 publication remains historical; do not redo the completed workspace migration or push this work without an upload request.

Earlier handoff records below describe the previously published baseline.

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


## Date / Workspace

- Date: 2026-10-20
- Project: `D:\SourceCode\python\simple_com_chart_gui`
- Version: `1.0.1.001`, source `VERSION`
- Base commit observed: `3c4ee62` (`update`). Changes remain uncommitted.

## Current Authority

- User-approved goal: arbitrary text filtering and dynamic numeric telemetry plotting, with a portable native MFC delivery.
- Local rules: `AGENTS.md`; acceptance evidence: `SELF_CHECK.md`.
- Workspace entry: `..\#READ_ME_FIRST#\AGENTS.md`.

## Implemented

- Extended Python and C++ parsers to signed doubles, decimal/scientific values, percent magnitude, hex, ASCII units, `:`/`=` fields and slash list expansion.
- Raw-line Include/Exclude filters with Text/Regex modes, validation, Apply/new-capture behavior and error recovery.
- Configurable channel cap 1..1024 (default 256), bounded histories and dynamic channel checkboxes.
- Float/signed model, latest labels, hover values, Y-axis ticks and range handling; channels share a common receipt-time axis.
- Extended the existing native MFC implementation with static MFC/CRT and root one-click build. Canonical Release output is `build\SimpleComChart.exe`.
- Native filter/limit preferences are saved beside the EXE in `SimpleComChart.ini`; new files use a Unicode BOM.
- Kept GUI, layout, filtering, model, serial I/O and parsing in separate source units; serial shared state now has synchronization.
- Added demo input, native startup self-check, native/Python regression tests, and safe clean scripts.
- Build/clean entries under `scripts/build` delegate to the main scripts; clean preserves delivery/runtime files.
- Flattened the native implementation into root `src/` and `scripts/`, with root `CMakeLists.txt` and `src/SimpleComChart.rc` / `src/app.ico`.
- Preserved the two historical native EXEs in `build/legacy_native_mfc`, and preserved the previous CMake cache in `build/obj/cmake_pre_layout` before generating a fresh cache from the root source directory.

## Main Files

- Python: `src/python/core/log_parser.py`, `src/python/core/line_filter.py`, `src/python/core/channel_model.py`, `src/python/ui/*`; root `main.pyw` remains compatible.
- Native: root `src/log_parser.*`, `line_filter.*`, `channel_model.*`, `channel_panel.*`, `plot_view.*`, `mfc_main_dialog*`, `serial_manager.*`, `runtime_smoke.cpp`, help/version resources.
- Build/test: `scripts/*`, root `CMakeLists.txt`, alternate batch wrappers, `tests/*`, `.gitignore`, `VERSION`.
- Docs: project README/AGENTS/SELF_CHECK/HANDOFF and `docs/MFC_BUILD.md`.

## Verification

- Release and Debug x64 builds and CTest: PASS.
- Python core tests (4 groups), syntax and offscreen GUI integration: PASS.
- Both MFC startup self-checks: PASS; reports under `build\verify`.
- Dependency inspection: Windows system DLLs only; no Python/Qt/MFC/VC runtime DLL requirement.
- EXE version resource: `1.0.1.001`.
- Isolated clean preservation and Codex workspace clean audit: PASS.
- After the root-layout migration, reran fresh Release/Debug builds, both native core suites and both MFC startup checks: PASS. Verified `scripts/build/build_all.bat` and the moved clean wrapper (isolated delivery sentinels): PASS.
- New CMake cache points at the project root; no active source/build script references the former nested source paths. Setup helpers passed syntax inspection; installer scripts were not executed.
- Earlier hex-validation and integration-harness failures were corrected and rerun; see `SELF_CHECK.md` for history and remaining verification gaps.

## Open Issues / Risks

- Real hardware COM flow and sustained receive rates remain unverified.
- Interactive visual/DPI review was unavailable because the Windows Computer Use helper could not connect.
- Output targets Windows 10/11 x64, not macOS/Linux/ARM-native Windows. Adapter drivers may be required.
- Parsing expects comma-delimited numeric key/value telemetry. Arbitrary prose can be filtered but needs named numeric fields to plot. Offline log import is not included.
- Python-specific regex extensions are not a cross-implementation contract; use the common ECMAScript-style subset.

## Next Step

Run the Release EXE against the intended MCU COM stream, select related channels and Fit. Record real hardware and DPI evidence here when available.

## Document Sync

AGENTS, README, SELF_CHECK, HANDOFF and `docs/MFC_BUILD.md` updated. Shared workspace rules unchanged because this implementation follows the existing baseline.
