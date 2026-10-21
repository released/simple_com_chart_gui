# AGENTS.md

## Workspace Layout And Safe Clean

Use the hybrid profile already described here. Screenshots live under docs/images. Both clean entries use the preflighted wrapper; a file inside obj/tests is not automatically disposable.

Python cache cleanup is file-only (.pyc/.pyo), not recursive cache-directory deletion. Preserve EXE/INI/unknown cache contents too; expanded fixtures must check both native and source/cache junctions before native deletion.


## Project Scope

- Serial text telemetry monitor with a native MFC delivery and a maintained Python implementation.
- Native output: `build\SimpleComChart.exe` for Windows 10/11 x64.
- Use static MFC and static CRT; execution must not require Python or the VC redistributable.
- `VERSION` is the version source; CMake generates native title/resource version definitions.
- Keep the native layout consistent with sibling MFC tools: root `src/`, `scripts/`, `build/`, `tests/` and `CMakeLists.txt`. Do not recreate a nested native source tree.
- `src/SimpleComChart.rc` and `src/app.ico` are the native resources. The maintained Python implementation is under `src/python` (core/ui/utils/main.pyw/app.ico); root `main.pyw` is the compatible launcher. Documentation screenshots are under `docs/images`.

## Contracts

- Discover channels from comma-delimited `key:value` or `key=value`; no firmware-specific key catalogue.
- Preserve finite signed double values, decimal/scientific numbers, hex, ASCII units and percent magnitude.
- Slash-separated values become `key[0]`, `key[1]`, etc. Skip malformed fields without fabricating zeros.
- Raw-line Include/Exclude filters run before parsing. Text matching is case-sensitive; Regex uses the common Python/ECMAScript subset.
- Preserve filter syntax: multiple alternatives require explicit Regex mode with `|`; comma/semicolon do not split patterns, and Text treats the whole input literally. Keep status, Include help and Exclude help in separate measured/wrapping paragraphs, including after invalid Apply. Exclusion wins even when Include matches; do not imply line filters select individual chart fields.
- Keep the on-screen Include/Exclude hints brief: Set Regex, use `|`, and one example each. Put detailed syntax/priority explanations in Help/README, not additional prose in the main window.
- Applying valid filters/limits starts a new capture; invalid input retains the active filter/data.
- Channel limit defaults to 256, configurable 1..1024. Keep buffers bounded.
- MFC capture settings stay in `SimpleComChart.ini` beside the EXE.
- MFC has Chart and Data / 4 Charts tabs. Keep the data capture independent of the first page's selection, Refresh, Snapshot and time window.
- Capture retains up to 10000 accepted raw lines / 32 MiB payload; expose eviction count. Apply/connect reset capture; invalid Apply must not.
- Data rows are sparse (missing is blank, never zero); keep headers fixed and preserve the visible row when Auto-scroll is off, including retention eviction.
- Four small plots own their channel model, time window and automatic Y scale. Do not share selection/axes with the first page.
- Data-page field checkboxes / All / None control visible table columns only, independently of both chart selections. Initially show the first four discovered fields; newly discovered fields after a user selection stay unchecked. Keep explicit visible-to-capture column mapping, sparse blanks, bounded latest-value caches and scroll anchors. Hidden fields remain in capture/export.
- Both tabs show latest-point markers and live labels, including singleton series; omit stale labels outside the time window or for `(none)`. The shared default-on Show name checkbox switches name:value versus numeric-only labels, not label visibility. Preserve numeric text when shortening long names; selections and this option are per-session UI state.
- The Chart page automatically expands AND shrinks Y using enabled, filtered samples in its current time window. Fit uses the same calculation; Snapshot/Fit must use frozen samples, not incoming live values. Apply/Refresh/no visible data reset to the empty 0..50 fallback. Never retain a previous capture's axis bounds or accumulate overlay padding.
- Save As defaults to plain .log; .txt is raw text, .csv/.cvs is escaped UTF-8 table, .xls is binary BIFF8 (254 numeric fields maximum). Export immutable snapshots on a worker, preserve the destination on failure, and guard close while exporting.
- UI/layout must guard uncreated HWNDs. Preserve decimal values in model, hover, labels and Y-axis.
- Keep control fonts alive until dialog teardown; never delete a shared font inside paint handlers. Runtime checks must verify live fonts and full caption bounds after repeated resize/repaint/tab switches, not only startup.
- Serial I/O runs in a worker; synchronize handle/overflow state with the UI.

## Verification And Delivery

- Build: `scripts\build_release.bat` (Release x64 plus native tests).
- Debug: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug`.
- Python core: `python -B -m unittest discover -s tests -v`.
- Native startup: `scripts\verify_runtime.ps1 -Configuration Release` and `Debug`.
- Also run each configuration with `-Font125` for enlarged-font layout regression (not a real monitor-DPI switch). Inspect the generated chart/data PNGs.
- `--demo` runs simulated data without a COM device; `--smoke-test` validates MFC startup/UI logic and closes.
- Real COM timing/throughput and visual DPI checks require explicit evidence; never imply them from core tests.
- Clean entries: `scripts\clean_mfc.bat` and compatible `scripts\clean_build.bat`; preflight Python/cache and native targets before deletion, remove only allowlisted compiler files and Python caches. Never purge obj/tests/verify folders; preserve EXE/INI/DLL/data/logs/unknown files even inside obj. Keep `clean_build.ps1 -WhatIf` non-mutating.
- Codex runs the workspace clean audit after clean/build flow changes; see `..\#READ_ME_FIRST#\AGENTS.md`.
- Keep README, SELF_CHECK and HANDOFF aligned with actual behavior and verification.
