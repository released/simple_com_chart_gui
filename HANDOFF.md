# HANDOFF.md

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
