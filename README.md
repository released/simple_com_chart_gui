# Simple COM Chart GUI

## Publication and local data

Keep personal INIs, logs and debugger/user options out of publication.
Defaults are generated locally where supported;
existing local settings/data are not deleted by clean or publication. Current-tree
index removals do not erase earlier Git history. See SELF_CHECK.md for actual
build/test results and hardware limitations.


Windows COM telemetry monitor, with a portable native MFC executable and a maintained Python implementation.

## Run the native tool

Copy `build\SimpleComChart.exe` to a writable folder on a Windows 10/11 x64 PC and open it. Python, Qt and the Visual C++ redistributable are not required. A USB-UART adapter may still need its manufacturer's driver.

Select COM / baud / data / parity / stop, then Connect. New keys automatically appear in Channels. Use None and select the desired signals; Y scales automatically. Data comes from the live COM stream; offline log import is not implemented.

## Native tabs and Save As (1.0.1.002)

Current native build: **1.0.1.004**. The Chart page now automatically expands and shrinks its Y axis to enabled, filtered data in the current time window. Retains the 1.0.1.003 toolbar/font-lifetime fix. Capture/export formats and serial protocol behavior are unchanged.

- **Chart** keeps the existing selectable combined waveform. Include/Exclude labels now use actual-font measurements and spacing.
- **Data / 4 Charts** discovers all numeric field names as columns, with a fixed header and one row per accepted receive line. Newest rows append at the bottom; fields absent from that line stay blank, not copied from another line or replaced with zero. Row and Time (s) identify receive order and elapsed PC receipt time.
- **Auto-scroll to newest** is initially checked. Uncheck it to inspect older rows without updates pulling the view down. If the inspected row expires from retention, the view stays at the oldest remaining row.
- The four bottom charts each have their own field selector, time window (5/10/30/60/120 seconds), history and automatic Y scale, including small decimals and negative values. Their selections/axes do not depend on the Chart tab. Default choices are the first four discovered fields; `(none)` disables a chart.
- **Save As...** is shared by both tabs. Default `.log` and optional `.txt` save the original filtered lines as plain text with CRLF. `.csv` and the compatible `.cvs` spelling save the table as UTF-8 with BOM; `.xls` writes a real Excel 97-2003 BIFF8 workbook, without requiring Excel. Missing numeric cells remain blank. CSV headers are quoted and protected against formula interpretation.
- Capture is separate from the rolling waveform history: Chart Refresh/Auto Refresh, Snapshot, channel checkboxes and Time Window do not erase the table/log. Capture continues while minimized. Successful Connect or valid Apply starts a new capture; save before resetting or closing. Invalid Apply preserves data.
- Retention is bounded to 10,000 raw lines / 32 MiB payload. The data page displays retained and discarded counts. Save As exports the retained snapshot taken after confirming the filename, not a continuous recording and not lines received later. Older discarded rows cannot be recovered from that export.
- XLS supports 254 telemetry fields plus Row/Time; choose CSV/log for a wider capture. Export runs in the background. An existing destination is replaced only after successful writing; a locked/changed destination or failed export is reported without deleting the original. Close is deferred while saving.

This UI/export revision applies to the portable MFC EXE. The maintained Python frontend remains unchanged; its parser/filter/model compatibility tests are retained.

## Filters and waveforms

- Include: a raw line must contain/match this expression; empty accepts all.
- Exclude: matching raw lines are discarded; empty excludes nothing.
- Text: case-sensitive substring search; Regex: common ECMAScript-style search.
- Max CH: 1..1024, default 256.
- Apply commits the filters/limit, clears existing samples/channels, and starts a new capture.
- Invalid regex or channel limit keeps the previously applied configuration and data.
- Channel checkboxes choose which numeric signals to plot.
- Y automatically follows the minimum/maximum of the enabled, filtered samples in the displayed time window, with margins for decimals/negative values and optional end labels. A previous 1000-scale capture does not pin a later 201-scale capture. Channel selection and time-window changes rescale without requiring Fit.
- Fit explicitly recalculates the same range; there is no separate manual-only scaling formula. 0..50 is only the empty-data fallback, including after Apply/Refresh or when no channels have visible data.
- Snapshot freezes the plot and axis. Fit during Snapshot uses frozen samples, not incoming live values; Live resumes automatic scaling. Overlay toggles latest-value labels without accumulated axis expansion.
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
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1 -Configuration Release -Font125
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_runtime.ps1 -Configuration Debug -Font125
```

Run `build\SimpleComChart.exe --demo` to see simulated telemetry without hardware. The runtime verifier launches isolated copies with `--smoke-test`; it does not connect to or transmit data to a COM device.

Runtime checks generate `chart-page.png`, `data-page.png`, `runtime-capture.log` and `smoke-result.txt` under `build/verify/<configuration>[-font125]`. The font-scale fixture is not a substitute for a real 100%/125% monitor-DPI acceptance test. Core tests create five-format export fixtures under `build/verify/export-tests`. Optional independent XLS/CSV readback uses `scripts/verify_exports.py --reader-path <isolated-xlrd-folder>`; xlrd is a test-only reader, never a runtime dependency.

The startup verifier also writes `layout-result.txt`: checks font-handle validity and full text bounds at 1120/1180/1500-pixel window widths, on both tabs, after repeated synchronous repaint. Before-fix failed reports/screenshots are retained in `build/verify/font-lifecycle-before-1.0.1.002`.

Y-axis regressions write `y-axis-result.txt` and `y-axis-201.png` in the same runtime fixture directories. Cases include 1000 -> filtered 200/201, channel unchecking, time-window shrink, negative decimals, constant zero, no selected data, invalid Apply, stable overlay margins and Snapshot/Fit/Live behavior.

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

