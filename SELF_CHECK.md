# SELF_CHECK.md

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
