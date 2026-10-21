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
- Applying valid filters/limits starts a new capture; invalid input retains the active filter/data.
- Channel limit defaults to 256, configurable 1..1024. Keep buffers bounded.
- MFC capture settings stay in `SimpleComChart.ini` beside the EXE.
- UI/layout must guard uncreated HWNDs. Preserve decimal values in model, hover, labels and Y-axis.
- Serial I/O runs in a worker; synchronize handle/overflow state with the UI.

## Verification And Delivery

- Build: `scripts\build_release.bat` (Release x64 plus native tests).
- Debug: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug`.
- Python core: `python -B -m unittest discover -s tests -v`.
- Native startup: `scripts\verify_runtime.ps1 -Configuration Release` and `Debug`.
- `--demo` runs simulated data without a COM device; `--smoke-test` validates MFC startup/UI logic and closes.
- Real COM timing/throughput and visual DPI checks require explicit evidence; never imply them from core tests.
- Clean entries: `scripts\clean_mfc.bat` and compatible `scripts\clean_build.bat`; preflight Python/cache and native targets before deletion, remove only allowlisted compiler files and Python caches. Never purge obj/tests/verify folders; preserve EXE/INI/DLL/data/logs/unknown files even inside obj. Keep `clean_build.ps1 -WhatIf` non-mutating.
- Codex runs the workspace clean audit after clean/build flow changes; see `..\#READ_ME_FIRST#\AGENTS.md`.
- Keep README, SELF_CHECK and HANDOFF aligned with actual behavior and verification.
