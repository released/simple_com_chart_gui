# Simple COM Chart GUI

## Publication and local data

The current source/layout and native delivery are published without personal INIs,
logs or debugger/user options. Defaults are generated locally where supported;
existing local settings/data are not deleted by clean or publication. Current-tree
index removals do not erase earlier Git history. See SELF_CHECK.md for actual
build/test results and hardware limitations.


Windows COM telemetry monitor, with a portable native MFC executable and a maintained Python implementation.

## Run the native tool

Copy `build\SimpleComChart.exe` to a writable folder on a Windows 10/11 x64 PC and open it. Python, Qt and the Visual C++ redistributable are not required. A USB-UART adapter may still need its manufacturer's driver.

Select COM / baud / data / parity / stop, then Connect. New keys automatically appear in Channels. Use None, select the desired signals, then Fit. Data comes from the live COM stream; offline log import is not implemented.

## Filters and waveforms

- Include: a raw line must contain/match this expression; empty accepts all.
- Exclude: matching raw lines are discarded; empty excludes nothing.
- Text: case-sensitive substring search; Regex: common ECMAScript-style search.
- Max CH: 1..1024, default 256.
- Apply commits the filters/limit, clears existing samples/channels, and starts a new capture.
- Invalid regex or channel limit keeps the previously applied configuration and data.
- Channel checkboxes choose which numeric signals to plot.
- Fit rescales Y for selected signals, including small decimals and negative values.
- Snapshot freezes the plot; Live resumes. Overlay toggles latest-value labels.
- Refresh clears samples while retaining discovered channels.

For the attached fan/AC-style logs, use `fan,` in Text mode to select fan lines, `ac_adc_mv:` for AC lines, or `^(fan,|ac_adc_mv:)` in Regex mode for both. Leave Include/Exclude empty to capture every valid field.

Filters apply to the whole raw line before numeric parsing. Arbitrary prose without a numeric `key:value` or `key=value` field can be filtered, but does not itself become a waveform. Regex behavior shared between Python and C++ covers ordinary groups, alternation, anchors and character classes; Python-specific regex extensions are not portable.

## Data format

Each sample ends in LF or CRLF. Fields are comma-separated, with `:` or `=` between key and value. Keys are dynamically discovered, with a 128-byte UTF-8 limit.

```text
fan,duty:920,target:2340,fg_hz:69.1,offset:-2.5,err:11%,cmd:0x3202,cap:105/76/29
ac_adc_mv:1169,ac_v:116,ocp_mv:377,ntc_c:76,cnt:554
```

| Input | Channel/value |
|---|---|
| `fg_hz:69.1` | `fg_hz = 69.1` |
| `offset:-2.5` | `offset = -2.5` |
| `err:11%` | `err = 11` (percentage magnitude) |
| `cmd:0x3202` | `cmd = 12802` |
| `CHG:4179mv` | `CHG = 4179` (units are not rescaled) |
| `A=2e-3` | `A = 0.002` |
| `cap:105/76/29` | `cap[0] = 105`, `cap[1] = 76`, `cap[2] = 29` |

Standalone tokens such as `fan`, malformed numbers, empty list elements and non-finite values are skipped. Receipt timestamps are generated on the PC; device timestamps are not inferred. History is limited by the selected time window and 20,000 samples per channel.

## Portable configuration

The MFC tool creates `SimpleComChart.ini` beside its EXE. It stores applied Include, Exclude, Regex and MaxChannels. Unapplied edits are not saved. Run from a writable directory.

## Build and verify

Developer machine: Visual Studio 2022 C++ desktop workload, MFC x64, Windows SDK and CMake.

```bat
scripts\build_release.bat
```

This builds Release x64 and runs the native core tests. Output: `build\SimpleComChart.exe`. Build Debug with `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug`.

Additional checks:

```powershell
python -B -m unittest discover -s tests -v
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1 -Configuration Release
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1 -Configuration Debug
```

Run `build\SimpleComChart.exe --demo` to see simulated telemetry without hardware. The runtime verifier launches isolated copies with `--smoke-test`; it does not connect to or transmit data to a COM device.

Clean: `scripts\clean_mfc.bat` (compatible `scripts\clean_build.bat`) removes allowlisted compiler intermediates and Python caches only, after path/reparse preflight. EXE, INI, DLL, logs, data and unknown delivery files remain, even inside obj/tests. `clean_build.ps1 -WhatIf` is non-mutating. Alternate entries under `scripts\build` delegate to the main scripts.

## Project structure

```text
simple_com_chart_gui/
  src/                 MFC C++ sources, SimpleComChart.rc and native app.ico
  scripts/             build, clean and verification entry points
    build/             alternate build/clean wrappers
    setup/             existing Visual Studio setup helpers
  build/               SimpleComChart.exe and portable configuration
    obj/               compiler and CMake intermediates
    legacy_native_mfc/ preserved historical executables
  tests/               native/Python tests and telemetry fixtures
  docs/MFC_BUILD.md    native module and build notes
  CMakeLists.txt       root native build definition
  src/python/          maintained Python main/core/ui/utils and icon
  docs/images/         historical documentation screenshots
  main.pyw             compatible Python launcher
```

The former `native_mfc` source container has been flattened into this layout. Output remains `build\SimpleComChart.exe`; the original Python double-click entry remains compatible while its modules live under `src/python`.

## Python source

`main.pyw` uses PyQt5, pyqtgraph and pyserial. Its parser, channel model and line-filter behavior are kept aligned with the native tool. The portable distribution is the MFC EXE.

See `AGENTS.md`, `SELF_CHECK.md` and `HANDOFF.md` for development rules and actual verification.

