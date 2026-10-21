# SELF_CHECK.md

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
