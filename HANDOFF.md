# HANDOFF.md

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
