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

Current native build: **1.0.1.018**. Tabs are ordered **Messages / Chart / 4 Charts**. Choose the startup page under **Settings > Startup tab**; Chart remains the default when unconfigured. The active tab has a dark-grey face, white text and a dark underline. Messages uses green text on black in both ASCII and HEX, remembers its adjustable font size and can Save As its current text view. Its display/font toolbar shares aligned control bounds and vertically centered labels. Toolbar/filter hover help replaces permanent Include/Exclude explanations; applied/error status stays visible. Button tooltips track their actual child windows. Existing raw capture, remembered chart choices/reset, timestamp filenames, filters and export formats remain supported. The last successful serial settings are remembered, with same-bridge automatic recovery after an unexpected disconnect. Connection-state changes keep the surrounding layout fixed. A failed manual Connect shows an explicit warning with the PORT, original error and troubleshooting steps.

- **Chart** keeps the existing selectable combined waveform. Include/Exclude labels now use actual-font measurements and spacing.
- **4 Charts** lists discovered fields in a left-hand checkbox panel, with All / None. Only checked fields become table columns; initially the first four are checked. Additional fields stay unchecked once you make a selection. Row and Time (s) are always shown. The fixed-header table has one row per accepted receive line, newest at the bottom; fields absent from a line stay blank, not stale or zero. Headers and cells are left-aligned. Column widths follow measured titles and recent values instead of stretching across the viewport; a bounded sample keeps resizing responsive. Select fewer fields to avoid horizontal scrolling; selecting many may still require it.
- Table field choices are independent of the Chart page and of the four chart selectors. Hiding a field does not discard it or remove it from Save As: exports continue to contain all retained fields/accepted lines. Valid Apply/Connect resets table field choices to the new capture's first four fields.
- **Auto-scroll to newest** is initially checked. Uncheck it to inspect older rows without updates pulling the view down. If the inspected row expires from retention, the view stays at the oldest remaining row.
- The four bottom charts each have their own field selector, time window (5/10/30/60/120 seconds), history and automatic Y scale, including small decimals and negative values. Their selections/axes do not depend on the Chart tab. Initially all four are `(none)`, so select the desired fields yourself. Choices are saved beside the EXE and restored next time; a remembered field waits for matching incoming data, never another channel. **Reset to none** disables all four and immediately clears their saved choices, without erasing table/log data. Apply/Connect clear samples but retain the choices.
- Each chart shows a marker and live value at its latest visible sample, including the first/only sample. **Show name**, beside the tabs and initially checked, applies to both pages: checked = `field: value`, unchecked = `value`. It does not hide numeric labels. The Chart page's existing Overlay button still switches its labels/markers on or off. Long names are shortened when needed without shortening the numeric value. Empty/expired series have no stale label.
- **Save As...** on the two chart tabs exports their shared filtered capture. Its editable default filename uses PC local time at click, e.g. `20261022_143025.log`. Default `.log` and optional `.txt` save the original filtered lines as plain text with CRLF. `.csv` and the compatible `.cvs` spelling save the table as UTF-8 with BOM; `.xls` writes a real Excel 97-2003 BIFF8 workbook, without requiring Excel. Missing numeric cells remain blank. CSV headers are quoted and protected against formula interpretation. Existing filenames still require overwrite confirmation. Messages has its own text-export behavior below.
- Capture is separate from the rolling waveform history: Chart Refresh/Auto Refresh, Snapshot, channel checkboxes and Time Window do not erase the table/log. Capture continues while minimized. Successful manual Connect or valid Apply starts a new capture; save before resetting or closing. Automatic bridge recovery and invalid Apply preserve data.
- Retention is bounded to 10,000 raw lines / 32 MiB payload. The data page displays retained and discarded counts. Save As exports the retained snapshot taken after confirming the filename, not a continuous recording and not lines received later. Older discarded rows cannot be recovered from that export.
- XLS supports 254 telemetry fields plus Row/Time; choose CSV/log for a wider capture. Export runs in the background. An existing destination is replaced only after successful writing; a locked/changed destination or failed export is reported without deleting the original. Close is deferred while saving.

This UI/export revision applies to the portable MFC EXE. The maintained Python frontend remains unchanged; its parser/filter/model compatibility tests are retained.

### Startup tab

Use **Settings > Startup tab > Messages / Chart / 4 Charts**. A radio check marks the confirmed choice, which saves immediately beside the EXE in `SimpleComChart.ini`, section `[UI]`, key `StartupTab` (`Messages`, `Chart` or `4Charts`). It applies **next launch**; choosing it does not change the current page or clear captured data. Ordinary tab clicks do not alter this explicit default. Missing/invalid settings fall back to Chart. A failed save keeps the previous choice and shows an INI-write error; use a writable EXE folder. This preference does not automatically connect a device.

### Messages

**Messages**, the leftmost tab, is an unfiltered raw receive monitor, separate from numeric capture. Text is green on black. ASCII is the default; switch to HEX to view the same bytes as hexadecimal, 16 per row with byte offsets. Updates do not require a newline. CR, LF, NUL and non-ASCII bytes remain available for HEX; ASCII displays printable 7-bit characters/TAB and normalized line breaks, escaping other bytes as `\xNN` (not Unicode decoding). Opening the app uses the saved startup-tab choice, or Chart if none is configured.

Use **Font: - / +** beside Clear messages to change message text by 1 pt per click. The current size is shown; range is 8–32 pt, default 10 pt. Both ASCII/HEX use the same size. Changes save immediately to `[Messages] FontSizePt` in `SimpleComChart.ini` beside the EXE and restore next launch. The buttons stop at the limits; edited invalid/out-of-range INI values are constrained to that range. Only message text changes, not toolbar/chart text, captured bytes or saved log content. Auto-scroll off keeps the inspected line where possible; auto-scroll on still follows the newest data. Use a writable EXE folder so settings can be saved.

**Auto-scroll** follows newest messages by default; uncheck it to inspect older text. **Clear messages** affects only this page. Switching ASCII/HEX or applying chart filters does not clear messages; a successful new connection starts fresh. Disconnect retains the final queued raw bytes. Pending and displayed raw buffers each retain at most 256 KiB, evicting oldest bytes in batches. Dropped counts expired history; Input lost counts pending bytes evicted before UI delivery, not hardware-level loss. After a pending gap, old displayed history is cleared rather than joining across missing input. If an inspected position expires, the view clamps to the retained start.

**Save As...** on Messages saves the current **ASCII or HEX text view**, independently of Include/Exclude and even with no numeric/chart rows. Choose `.log` (default) or `.txt`; the editable filename uses PC local date/time to seconds, e.g. `20261022_143025.log`. ASCII keeps its displayed byte escapes and line breaks; HEX saves offsets and hexadecimal text, not binary bytes. No BOM, header or extra newline is added. It exports all retained message history, not only the visible screen/selection; older dropped/lost bytes cannot be recovered. The snapshot is taken after confirming the filename and remains unchanged by later receive/mode/Clear/tab changes. Saving runs in the background with the same safe-replacement and close/busy guards as chart exports. Empty history reports no messages to save. This is not continuous recording or offline import.

### Chart toolbar and hover explanations

All four buttons have real functions: **Refresh** clears rolling Chart samples (not captured logs/table), **Fit** recalculates the visible/frozen axes, **Snapshot / Live** freezes/resumes the Chart, and **Overlay** toggles latest-point markers/labels. They are retained and disabled on the other pages, whose plots/messages are independent. The toolbar uses normal text, straight borders and system UI-grey faces. Pressing/executing a command briefly turns its button red; completion restores grey, even when Snapshot/Overlay is toggled. Snapshot's caption changes to Live while frozen. Connect is grey when disconnected and red while executing/connected, returning to grey after disconnect/failure. Hover stays neutral and disabled controls stay grey; waveform colors remain distinct.

Pause over a button to see its explanation. **Auto Refresh (s)** has help on both its label and selector: it automatically clears Chart waveform samples every selected N seconds; **Off** stops timed clearing. It is **not** the serial receive or UI-update interval. Capture/table/log and Messages remain intact. Include/Exclude explanations also appear only on hover, over either label or input. Tooltips wait about 0.6 s, appear without fade/animation, wrap long text and close normally after moving away/clicking. Applied-filter status and validation errors remain on screen. No action or data reset is triggered by reading help.

## Filters and waveforms

- Include: a raw line must contain/match this expression; empty accepts all.
- Exclude: matching raw lines are discarded; empty excludes nothing.
- Text: case-sensitive substring search; Regex: common ECMAScript-style search.
- For multiple alternatives, select **Regex**, enter `A|B`, then Apply. Example Include `fan,|ac_adc_mv:` displays lines containing `fan,` OR `ac_adc_mv:`; Exclude `error|timeout` discards lines containing either word, even when Include matches. Matching is case-sensitive.
- Commas and semicolons are literal characters, not automatic multi-string separators. In Text mode even `|` is literal: the entire input must occur as one substring. Empty Include accepts all lines; empty Exclude rejects none. Regex metacharacters must be escaped when matching them literally, e.g. `cap\[0\]`.
- The UI keeps applied/error/new-capture status visible. Hover over Include/Exclude to see the short Regex `|` examples and priority reminders. Filter selection applies to whole raw lines; use channel/table checkboxes to choose which fields are visible.
- Max CH: 1..1024, default 256.
- Apply commits the filters/limit, clears existing samples/channels, and starts a new capture.
- Invalid regex or channel limit keeps the previously applied configuration and data.
- Channel checkboxes choose which numeric signals to plot.
- Click **Channel** above either checkbox list to toggle ascending/descending names; the header shows A-Z or Z-A. Sorting ignores case and treats digit groups numerically (P2 before P10); equal names under that comparison use an exact-name tie-break. Each list retains its own direction for new channels and capture resets during the session. Sorting preserves checkbox/value/color associations and never sorts the chronological data table or changes the four chart selectors. Value-header sorting is not included.
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

## Remembered PORT and bridge recovery (native 1.0.1.015)

The last **successful** COM connection saves its PORT, baud, data bits, parity, stop bits and Windows device instance ID immediately in `[Serial]` of the portable INI. Next launch restores those choices, but does not open hardware until you press **Connect**. Failed/unconfirmed choices do not overwrite the saved working connection. If the saved bridge is absent, the tool does not silently choose a different port.

If a manual **Connect** fails, **Connection failed** appears once with the PORT and original error (including the Windows error number/system description when the OS can provide one). The warning suggests disconnecting the same PORT in Tera Term/PuTTY/another tool instance, checking the bridge/USB cable and Device Manager COM number, verifying serial settings, then pressing Connect again. Being occupied is a possibility, not a proven identification of the other process; the tool does not close other applications. No selected PORT, invalid settings or a previous pending open also produce an explanatory warning. Failed attempts keep the saved working connection and existing capture. Automatic reconnection failures remain non-modal status/log entries, so they do not repeatedly interrupt you.

After a connected bridge disappears or reports a read error, the status shows waiting/reconnecting. The tool waits for that same identified bridge and retries with the original serial parameters, even if Windows assigns it a different COM number. A different bridge reusing the old number is not opened. Retry delay grows from 2 seconds to at most 10 seconds; absent devices are only scanned once per second. The waiting state continues until the bridge returns or you press **Cancel**. Driver open happens in the background; one pending open at a time, with late results discarded after cancellation. Repeated identical failures are rate-limited in the log, without modal dialogs.

Recovery keeps existing chart samples, table rows, raw Messages and selections; new bytes append after reconnection. An incomplete numeric line from the old connection is not joined to the new stream; Messages retains the exact received bytes. **Disconnect/Cancel** deliberately stops automatic recovery. No received data is not itself a disconnect. A device without a unique stable identity/enumerated serial interface needs manual reconnection rather than unsafe guessing. Recovery reopens the serial device only, without firmware commands, command replay or target reset requests; the existing DTR/RTS enable behavior remains, so adapters/targets that reset on those signals may still reset when reopened.

Automated verification uses simulated devices only. Physical USB unplug/reinsert, adapter-specific identity behavior and driver/target side effects remain hardware acceptance checks. This feature applies to the native MFC EXE; the Python frontend is unchanged.

Connect/Cancel/Disconnect share a fixed width measured from the longest caption; serial controls/pages no longer move as the state changes. The COM field uses the row's remaining width so the last Stop field still fits at narrow/enlarged-font layouts. Only connection-related captions, enabled controls, button accent and status are updated.

## Portable configuration

The MFC tool creates `SimpleComChart.ini` beside its EXE. It stores applied Include, Exclude, Regex and MaxChannels, four selected field names in `[FourCharts]`, Messages font size, the startup-page choice in `[UI] StartupTab`, and the last successful serial connection in `[Serial]` (`Port`, `DeviceInstanceId`, `Baud`, `DataBits`, `Parity`, `StopBits`). Serial interface paths are never loaded from INI. Nonempty chart names use a `key:` prefix to preserve quoted field names through Windows profile APIs; empty means `(none)`. User selector changes and Reset save immediately. Unapplied filter edits are not saved. Run from a writable directory.

Table field choices, chart time windows, ASCII/HEX mode, Auto-scroll and Show name remain per-session UI state.

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

The startup verifier also writes `layout-result.txt`: checks font-handle validity and full text bounds at 1120/1180/1500-pixel window widths, on all three tabs, after repeated synchronous repaint. Before-fix failed reports/screenshots are retained in `build/verify/font-lifecycle-before-1.0.1.002`.

Y-axis regressions write `y-axis-result.txt` and `y-axis-201.png` in the same runtime fixture directories. Cases include 1000 -> filtered 200/201, channel unchecking, time-window shrink, negative decimals, constant zero, no selected data, invalid Apply, stable overlay margins and Snapshot/Fit/Live behavior.

`data-display-result.txt` verifies selectable column indices, sparse blanks, full capture preservation, bounded field caches, latest-point labels, both name modes and long-name numeric preservation. `data-selected-names.png`, `data-selected-values.png` and `data-long-name.png` are actual native UI captures.

`filter-guidance-result.txt` and `filter-guidance.png` verify the multi-string examples through the receive/capture path, Exclude priority, case sensitivity and preservation after invalid Apply. Layout checks measure the remaining applied/error status paragraph on all three tabs at 1120/1180/1500-pixel widths; Include/Exclude help now lives in tooltips.

`channel-sort-result.txt` verifies real header mouse-message dispatch, natural ascending/descending order, stable checkbox/value/color identities, live updates/new fields, All/None, independent 4 Charts field selection and retained session direction. `channel-sorted.png` and `data-fields-sorted.png` show the native result.

`messages-result.txt`, `messages-ascii.png` and `messages-hex.png` cover raw binary escapes/CRLF, no-newline updates, filtering independence, mode switches/partial HEX rows, follow/clear/disconnect and fonts. `chart-preferences-result.txt` and `data-auto-columns.png` cover none defaults, exact INI restoration, delayed discovery, persisted Reset, left-aligned content widths and timestamp filenames. These fixtures do not exercise a physical COM device.

`message-export-result.txt` covers Messages-only ASCII/HEX text snapshots with an empty filtered capture, mode/receive/Clear isolation, shared busy/close guards, locked-file preservation, replacement, extension rejection and retained HEX offsets. Native core tests also compare exact saved text and safe failures; dialog interaction through a physical mouse is a separate UI acceptance check.

`message-font-result.txt` checks actual font-button notifications, immediate INI writes/restoration, missing/invalid/oversized settings, bounds, font height/lifetime, ASCII/HEX/data/export isolation and scroll behavior. It also checks all eight toolbar controls' top/bottom/center alignment and static-label centering at body sizes 8/10/32 pt and widths 1120/1500. `messages-font-large-ascii.png` / `messages-font-large-hex.png` show 24 pt content; `messages-toolbar-aligned.png` shows the aligned toolbar at 10 pt.

`serial-connection-result.txt`, `serial-reconnect-waiting.png` and `serial-reconnected.png` exercise fixture-only INI restoration, failed/manual opens, read/PnP loss, quiet RX/failed scans, absent/reused/renumbered COM ports, busy retry, capture preservation, manual cancel and late-open cancellation. Injected transports/scanners call no real COM open. Core tests also cover retry backoff, ambiguous identity, invalid configuration, single-worker and owner-close behavior. These are not physical bridge acceptance tests.

For connection-only changes, use `scripts/verify_runtime.ps1 -Configuration Debug -Scope Serial` (or Release, optionally `-Font125`). It runs connection geometry/state plus fake serial recovery, not the unrelated chart/export/font/filter suites. Isolated evidence is in `build/verify/<configuration>[-font125]-serial`. `connection-redraw-result.txt` counts unrelated window-position events and checks fixed child rectangles/caption fit through 72 state updates across all tabs and 1120/1500 widths; `connection-opening-page-*.png` are settled layout captures, not a recording of transient flicker. Before-fix evidence is retained under `build/verify/connection-redraw-before-1.0.1.016`.

The serial fixture also checks manual warning dispatch/content/owner/OK-warning flags, exactly-once failure reporting, no alerts on automatic retries or cancelled late results, invalid selection/settings and retry after failure. It intercepts MessageBox dispatch to avoid unattended dialogs; `manual-connection-warning.txt` records the exact simulated access-denied warning text. This is not a real Tera Term occupancy test or a physical-mouse dialog acceptance check.

For startup-page changes, use `scripts/verify_runtime.ps1 -Configuration Debug -Scope Startup -StartupTab Messages` (also `Chart` / `4Charts`, and Release). It writes only test-owned fixture INIs and launches a separate process for the chosen page; `startup-tab-result.txt`, `startup-page.png` and the scoped summary are under `build/verify/<configuration>-startup-<choice>`. It verifies real startup selection/focus/visibility, menu commands/radio checks, immediate INI persistence, no data/current-page reset, normal tab switching, fallback and read-only-file failure. It does not run unrelated serial/chart/export suites or open a COM device.

`button-theme-result.txt` and `button-theme.png` cover actual owner-draw pixel colors, pressed/executing/completed/disabled states, preserved toggle actions, connection palette and feedback cleanup after control destruction. The connected palette is checked without opening hardware; physical COM connection transitions remain a separate hardware check.

`ui-help-result.txt`, `messages-green.png`, `tabs-selected.png` and `auto-refresh-tooltip.png` cover tab order/default Chart, notification/page routing, selected-tab colors, black-green message colors, ten logical tooltip targets (each with a native child-HWND tool plus a parent-area fallback), resized rectangles, tooltip text/delay, native popup rendering and cleanup. Popup rendering is activated deterministically inside the fixture without moving the user's cursor; this is not a physical mouse-hover or monitor-DPI acceptance test.

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

