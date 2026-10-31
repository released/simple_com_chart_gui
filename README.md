# Simple COM Chart GUI

Windows COM telemetry monitor, with a portable native MFC executable and a maintained Python implementation.

Read the [Traditional Chinese user guide](docs/USER_GUIDE.md) or the complete [English user guide](docs/USER_GUIDE_EN.md).

## Run the native tool

Copy `build\SimpleComChart.exe`, `build\lang_tw.ini` and `build\lang_en.ini` together to a writable folder on a Windows 10/11 x64 PC and open it. Python, Qt and the Visual C++ redistributable are not required. A USB-UART adapter may still need its manufacturer's driver.

The native interface defaults to Traditional Chinese. Settings > Language (next launch) selects Traditional Chinese or English, saved as `[UI] Language=tw|en`. Restart normally to apply; current receive/capture is preserved when selecting. Missing or invalid catalogs use an English fallback with a visible notice. Raw data, user keys, file formats and some technical diagnostics retain their original form.

Select COM / baud / data / parity / stop, then Connect. New keys automatically appear in Channels. Use None and select the desired signals; Y scales automatically. For offline files, disconnect first and open UART > Offline replay.

## Tabs and Save As

### UART tools and navigation

Chart and each small plot offer 250 ms, 500 ms and every second from 1 to 300.
Type seconds or ms/s directly; press Enter or leave the field to apply. Each
small plot has an independent range; invalid input preserves its previous value.

The tool provides single-instance activation, adaptable layout/font handling, wrapping main controls and scrollable UART pages. A source/REC/AUTO/provenance/loss strip stays visible. See [UART guide](docs/UART_OPTIMIZATION_GUIDE.md).

**UART > Workbench / Dual COM** opens two simultaneously visible, vertically stacked port groups with separate receive logs, controls, Save As and recording. Port A follows the main connection; Port B is independent. Tools adds packet framing, typed binary charts, grouped commands and step scripts, TX pacing, X/YMODEM, read-only Modbus RTU, a monochrome VT100-style screen and numeric/gauge/bar/LED/FFT views. See the [Workbench guide](docs/UART_WORKBENCH_GUIDE.md) for formats, limits and timing assumptions.

**UART > Laboratory / Diagnostics** provides A/B processing diagnostics, background recording-range export and valid-prefix salvage, multi-parameter packet preview/confirmed send, sequence and PC response-time statistics, offline parser cases and reported digital-state history. Binary schemas support Message ID layouts and optional COBS/SLIP while retaining original wire recordings. The optional `build\SimpleComChartCli.exe` validates or explicitly runs finite private step scripts with CSV reports; it shares GUI session exclusion and never reconnects/retries. See the [Laboratory guide](docs/UART_LAB_GUIDE.md) for formats, persistence and measurement limits.

The [recording and workflow guide](docs/UART_SESSION_GUIDE.md) covers recording byte/time/free-space limits, capture-start settings and SHA256 session manifests, read-only COM device information, configurable numeric framing, nested JSON paths, custom CRC8/16/32, retained-data derivative/integral and passive request/reply ID analysis. These tools use the existing native architecture; previews and offline analysis never transmit.

Laboratory also provides named chart views, chart/four-plot PNG export, private
SHA256 recording notes, selected-field static HTML reports and bounded FFT
waterfall history. Packet parameters have a validated table editor; selected
retained text rows can be reviewed as offline parser cases. Profile Apply shows
typed current/staged differences. See section 15 of the [Traditional Chinese](docs/USER_GUIDE.md)
or [English user guide](docs/USER_GUIDE_EN.md) for operation and limits.

- UART > Field extraction: compatible key/value, CSV/Tab, flat numeric JSON or explicit nested JSON Pointer paths, and bounded Regex name/value/time groups; Preview is separate from Apply. Parser preferences are portable.
- Search channels by key/alias; All/None affect visible results only. UART > Channel display adds alias/unit/decimals/scale/offset without changing raw logs or exported numeric values.
- Wheel zoom, middle/Ctrl-left/Space-left pan, draggable A/B, double-click return to latest and Ctrl-right fixed/automatic Y. Table/Messages/chart navigation uses exact RX positions, with honest expired-history notices.
- Replay streams source data in background with a disk-paged index, numerically ordered completed rotation segments, cross-file Text/HEX search and time seek. Limits: 64 GiB / 50 million records / 4096 segments, not an equivalent RAM allocation.
- Messages Options adds optional CR/BS/erase-line rendering. Keyboard TX and readable ASCII local-status echo default on for each new valid connection; manual disable lasts for that connection. Paste still requires preview/confirmation. Raw RX/log remain unchanged; Ctrl+C sends 03, not Break.
- UART > Send file adds explicit binary/validated UTF-8 text transfer, bounded chunks/pacing/progress/pause/cancel. No partial-write retry or reconnect replay; this is not a flashing protocol or target ACK.

### Offline replay, profiles and analysis

Open **UART > Offline replay / Profiles / Chart analysis / Automation** for replay, portable profiles, analysis and generic send/reply automation. See the [advanced UART guide](docs/ADVANCED_UART_TOOLS.md).

- Background offline import, paused load, Play/Pause, 0.1..100x speed and 0-based record seek. Completed recorder `.bin` + matching `.idx` uses PC receipt intervals; plain text/unindexed binary uses explicitly synthetic intervals. No UART TX, live recording or hardware-byte statistics from replay. Display retention still applies.
- Named portable `.sccprofile` save/load and validated import/export. Load stages only; explicit idle-only Apply atomically merges saved preferences without opening hardware. Private history/commands/recording paths are excluded. Profiles are not encrypted and are ignored by Git.
- Independent retained-capture analysis chart: left/right click A/B or type cursor times, delta, inclusive interval min/max/mean/RMS/population SD, optional Y reference and view-end/window navigation. Empty/sparse intervals never fabricate zero samples. Capture-relative axes do not change the live charts.
- Explicit finite Periodic/Batch/Auto reply with Text/HEX, literal reply/trigger bytes, bounded wait/timeout retries and Sum8/XOR8/CRC16-Modbus. Stop/close/disconnect stops the run; no startup/reconnect resume. Failed/partial TX is not retried. Commands are not persisted. This is generic automation, not firmware-specific trigger recording.

### UART Console

Open **UART > Console / Serial options...**. This is a separate modeless window; closing it hides the Console without stopping RX/recording. Its title identifies the connection/COM and tool version.

- Select **Text UTF-8** or **HEX**, and **Append: None / CR / LF / CRLF**. Text is sent literally (no automatic `\\r` escape interpretation); ASCII is the UTF-8 subset. HEX accepts `41 42` or `4142`, without `0x`; invalid/odd input is rejected as a whole. The total packet including suffix is 1..4096 bytes. Empty text plus an explicit suffix can send just Enter/CR/LF. Click **Send** or Ctrl+Enter; ordinary Enter does not send. Queue/completion lines identify the target and operation ID; accepted-by-driver is not an acknowledgement from the device.
- The input dropdown remembers up to 32 submitted entries, including mode/suffix. Eight **Preset** slots have explicit Save/Load/Clear; loading/selecting never transmits. History/presets are portable INI data (hex-encoded UTF-8 for safe storage, NOT encryption) and may contain secrets; keep them private. **Clear history** deletes the saved history; **Clear TX view** clears only the displayed transcript.
- Choose **Flow control: None / RTS-CTS / XON-XOFF / DSR-DTR**, and manual DTR/RTS states. Handshake disables its owned manual checkbox. Default/missing configuration remains None with DTR/RTS on, matching the earlier tool's pin defaults. **Apply serial** while disconnected only saves options for next Connect. While connected it warns before submitting the change and saves after successful completion. Invalid/unreadable stored control options block Connect until reviewed/applied. Other serial settings/last-successful PORT keep their existing behavior; startup still does not open hardware.
- **Send Break...** is a separate explicit approximately 250 ms UART break condition, not Ctrl+C. DTR/RTS, handshake and Break can reset/interrupt devices or change boot mode: confirm wiring/target state before use. These controls are logical driver settings, not voltage measurements. Recovery reopens with confirmed settings and may therefore reproduce pin-related hardware side effects; it never replays TX or Break commands.
- Statistics show RX payload bytes, trailing-one-second payload rates, TX driver-reported accepted bytes/failures, and Frame/Parity/Overrun/RX-buffer/Break error-bit observations. Drivers may coalesce errors or lack pin reporting; these are not exact electrical error counts. CTS/DSR/DCD/RI show n/a when unavailable/disconnected. **Reset statistics** does not clear Messages, captured rows or recording.
- TX and RX use separate workers and overlapped native I/O so a flow-controlled write need not freeze receive processing. Queue limits are 16 requests / 64 KiB. Disconnect/close enters **Stopping** while I/O cancellation finishes; the GUI stays alive and the port is not reused until both workers finish. A late/partial request may already have affected hardware, so it is reported and never automatically resent. Confirmation is tied to the original connection generation; a changed connection requires a new submission.
- Messages and raw `.bin` remain RX-only: TX is not fabricated as received data. Continuous readable `.log` records `[TX]` separately and resumes `[RX]` direction tags; raw RX offsets/index remain unchanged. TX statistics/log data are driver-reported bytes, not proof of physical delivery.

### Recording and message display

Recording captures received bytes independently of the visible message history. Display options change presentation without changing raw RX data.

- **Messages > Save As / Start recording / Stop recording** sit beside Find. Save As saves the retained display; empty history gives an explanation. Start/Stop control continuous recording and disable according to state. Default names use local PC `YYYYMMDD_HHMMSS.log`; the page shows saved/current recorded filenames and actual `.log` segment size in KB (1024 bytes), without paths.
- **UART > Start recording RX...** lets the user choose the readable `.log` prefix and folder. Numbered readable files (`_1.log`, `_2.log`, etc.) are written there. Exact RX `.bin`, receipt/event `.idx` and the private capture-start `.sccmeta` are kept together under the automatically created `log` folder beside the EXE/INI. Existing files are never overwritten; choose another prefix after a name collision. The suggested name includes PC date/time to seconds. Recording can be armed before Connect and continues across tab changes, filters, display pause/Clear and reconnect waits. The raw file never contains synthetic timestamps, TX or connection markers. The readable file intentionally escapes non-ASCII bytes; inspect/decode the raw file for original Unicode bytes.
- **UART > Recording options...** configures size rotation (1..1024 MiB, default 32) and local-date rotation (default on); saved beside the EXE and applied next recording. Rotation is checked between read chunks, so a single unusually large chunk may exceed the target size. Each `.idx` remains beside its `.bin`, offsets restart at zero per segment, and concatenating numbered raw segments restores RX order. Completed raw/index segment pairs can be loaded through Offline replay.
- Recording runs before UI queuing on a separate disk worker; UI-history expiry cannot erase bytes already recorded. The writer flushes at approximately 250 ms and at Stop; this is not a power-loss durability guarantee. Its bounded queue is 8 MiB / 4096 items. Overflow stops recording with an explicit gap/error rather than silently continuing an incomplete capture. Disk/path/flush failures remain visible; partial evidence is preserved. Stop drains prior queued data; closing waits for completion and reports failed completion before allowing a later close. The status bar shows REC and byte count.
- **Messages > Display** adds UTF-8 and ANSI to ASCII/HEX. **Options...** selects system ANSI/Big5/GBK/Western code pages, PC local clock / elapsed from first received byte / delta between displayed lines, Auto/LF/CR/CRLF line endings, word wrap, ANSI foreground SGR colors, and Pause display. All timestamps are PC receipt times (multiple lines in one read can share a time), not MCU or UART bit timing. Partial UTF-8/DBCS suffixes wait for more bytes; HEX always preserves their original bytes. ANSI SGR is optional and not a complete terminal emulator.
- Encoding/clock/newline/code page/wrap/SGR save in portable `[Messages]` INI keys. Pause is session-only and does not stop receiving/recording. Save As still exports the latest retained snapshot (including bytes received during display pause), not a continuous file; Unicode display exports use UTF-8 without BOM. Default ASCII/HEX output remains compatible.
- **Find... / Ctrl+F** opens a separate search bar: Text or Regex, Case, Prev/Next (F3/Shift+F3), Highlight, Mark (Ctrl+F2), Next mark (F2) and Clear marks. Search is independent of Include/Exclude. Navigation stops Auto-scroll. Regex is deliberately bounded: ordinary literals/classes/alternation/anchors with at most one variable quantifier per alternative, no quantifiers in/on groups, backreferences or bounded-repeat expressions; searches run in 2048-character line slices, with at most 4096 hits. Use Text for long or complex input. Invalid/unsafe regex reports an error without deleting data. Bookmarks last only for the current retained display and clear on reformat/history eviction/reset.

Tabs are ordered **Messages / Chart / 4 Charts**. Choose the startup page under **Settings > Startup tab**; Chart remains the default when unconfigured. The active tab has a dark-grey face, white text and a dark underline. Messages uses green text on black in both ASCII and HEX, remembers its adjustable font size and can Save As its current text view. Its display/font toolbar shares aligned control bounds and vertically centered labels. Toolbar/filter hover help replaces permanent Include/Exclude explanations; applied/error status stays visible. Button tooltips track their actual child windows. Chart choices/reset, timestamp filenames, filters and exports are described below. The last successful serial settings are remembered, with same-bridge automatic recovery after an unexpected disconnect. Connection-state changes keep the surrounding layout fixed. A failed manual Connect shows an explicit warning with the PORT, original error and troubleshooting steps.

- **Chart** keeps the existing selectable combined waveform. Include/Exclude fields filter raw receive lines.
- **4 Charts** lists discovered fields in a left-hand checkbox panel, with All / None. Only checked fields become table columns; all fields start unchecked, including later discoveries. Row and Time (s) are always shown. The fixed-header table has one row per accepted receive line, newest at the bottom; fields absent from a line stay blank, not stale or zero. Headers and cells are left-aligned. Column widths follow measured titles and recent values instead of stretching across the viewport; a bounded sample keeps resizing responsive. Select fewer fields to avoid horizontal scrolling; selecting many may still require it.
- Table field choices are independent of the Chart page and of the four chart selectors. Hiding a field does not discard it or remove it from Save As: exports continue to contain all retained fields/accepted lines. Valid Apply/Connect resets table field choices to none; saved small-chart choices are retained.
- **Auto-scroll to newest** is initially checked. Uncheck it to inspect older rows without updates pulling the view down. If the inspected row expires from retention, the view stays at the oldest remaining row.
- The four bottom charts each have their own field selector, time window (250 ms, 500 ms and every second from 1 to 300), history and automatic Y scale, including small decimals and negative values. Their selections/axes do not depend on the Chart tab. Initially all four are `(none)`, so select the desired fields yourself. Choices are saved beside the EXE and restored next time; a remembered field waits for matching incoming data, never another channel. **Reset to none** disables all four and immediately clears their saved choices, without erasing table/log data. Apply/Connect clear samples but retain the choices.
- Each chart shows a marker and live value at its latest visible sample, including the first/only sample. **Show name**, beside the tabs and initially checked, applies to both pages: checked = `field: value`, unchecked = `value`. It does not hide numeric labels. The Chart page's existing Overlay button still switches its labels/markers on or off. Long names are shortened when needed without shortening the numeric value. Empty/expired series have no stale label.
- **Save As...** on the two chart tabs exports their shared filtered capture. Its editable default filename uses PC local time at click, e.g. `20261022_143025.log`. Default `.log` and optional `.txt` save the original filtered lines as plain text with CRLF. `.csv` and the compatible `.cvs` spelling save the table as UTF-8 with BOM; `.xls` writes a real Excel 97-2003 BIFF8 workbook, without requiring Excel. Missing numeric cells remain blank. CSV headers are quoted and protected against formula interpretation. Existing filenames still require overwrite confirmation. Messages has its own text-export behavior below.
- Capture is separate from the rolling waveform history: Chart Refresh/Auto Refresh, Snapshot, channel checkboxes and Time Window do not erase the table/log. Capture continues while minimized. Successful manual Connect or valid Apply starts a new capture; save before resetting or closing. Automatic bridge recovery and invalid Apply preserve data.
- Retention is bounded to 10,000 raw lines / 32 MiB payload. The data page displays retained and discarded counts. Save As exports the retained snapshot taken after confirming the filename, not a continuous recording and not lines received later. Older discarded rows cannot be recovered from that export.
- XLS supports 254 telemetry fields plus Row/Time; choose CSV/log for a wider capture. Export runs in the background. An existing destination is replaced only after successful writing; a locked/changed destination or failed export is reported without deleting the original. Close is deferred while saving.

These tab and export functions apply to the portable MFC EXE. The Python frontend has a separate UI.

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

Native [signal and recording analysis](docs/UART_ANALYSIS_GUIDE.md) adds independent Port B text charts, offline binary decoding, complete workspace profiles, derived values/EMA, a read-only byte/bit inspector, numeric script assertions/CSV results, recorded-run comparison and live XY. Dense Chart rendering preserves narrow peaks. Explicit finite A/B forwarding is available in Workbench Tools; it reserves both TX queues and never starts or resumes automatically.

## Filters and waveforms

- Include: a raw line must contain/match this expression; empty accepts all.
- Exclude: matching raw lines are discarded; empty excludes nothing.
- Text: case-sensitive substring search; Regex: bounded byte-oriented NFA search. See the extraction guide for supported syntax and limits.
- For multiple alternatives, select **Regex**, enter `A|B`, then Apply. Example Include `fan,|ac_adc_mv:` displays lines containing `fan,` OR `ac_adc_mv:`; Exclude `error|timeout` discards lines containing either word, even when Include matches. Matching is case-sensitive.
- Commas and semicolons are literal characters, not automatic multi-string separators. In Text mode even `|` is literal: the entire input must occur as one substring. Empty Include accepts all lines; empty Exclude rejects none. Regex metacharacters must be escaped when matching them literally, e.g. `cap\[0\]`.
- The UI keeps applied/error/new-capture status visible. Hover over Include/Exclude to see the short Regex `|` examples and priority reminders. Filter selection applies to whole raw lines; use channel/table checkboxes to choose which fields are visible.
- Max CH: 1..1024, default 256.
- Apply commits the filters/limit, clears existing samples/channels, and starts a new capture.
- Invalid regex or channel limit keeps the previously applied configuration and data.
- Channel checkboxes choose which numeric signals to plot. Discovered fields start unchecked; choose individual fields or All to display them. New fields stay unchecked without changing existing choices. A new capture starts with none selected.
- Click **Channel** above either checkbox list to toggle ascending/descending names; the header shows A-Z or Z-A. Sorting ignores case and treats digit groups numerically (P2 before P10); equal names under that comparison use an exact-name tie-break. Each list retains its own direction for new channels and capture resets during the session. Sorting preserves checkbox/value/color associations and never sorts the chronological data table or changes the four chart selectors. Value-header sorting is not included.
- Y automatically follows the minimum/maximum of the enabled, filtered samples in the displayed time window, with margins for decimals/negative values and optional end labels. A previous 1000-scale capture does not pin a later 201-scale capture. Channel selection and time-window changes rescale without requiring Fit.
- Fit explicitly recalculates the same range; there is no separate manual-only scaling formula. 0..50 is only the empty-data fallback, including after Apply/Refresh or when no channels have visible data.
- Snapshot freezes the plot and axis. Fit during Snapshot uses frozen samples, not incoming live values; Live resumes automatic scaling. Overlay toggles latest-value labels without accumulated axis expansion.
- Refresh clears samples while retaining discovered channels.

For the attached fan/AC-style logs, use `fan,` in Text mode to select fan lines, `ac_adc_mv:` for AC lines, or `^(fan,|ac_adc_mv:)` in Regex mode for both. Leave Include/Exclude empty to capture every valid field.

Filters apply to the whole raw line before numeric parsing. A line containing only one complete finite number becomes the stable `value` channel. Generic bracketed numeric fields such as `[P3:76.1803]` become `P3`; the tool's exact readable-log `[HH:mm:ss.mmm] [RX] ` prefix is metadata, not a channel. Arbitrary prose without a numeric field can be filtered, but does not itself become a waveform. Regex behavior shared between Python and C++ covers ordinary groups, alternation, anchors and character classes; Python-specific regex extensions are not portable.

## Data format

The default numeric framing ends each sample in LF or CRLF. Native Laboratory settings also support CR, a custom delimiter or a PC receipt idle gap, independently for A/B/offline replay. Fields in the default key/value parser are comma-separated, with `:` or `=` between key and value. Strict device output such as `[S07],raw:,-19056` is accepted as `[S07].raw`; the token after the empty value must be a complete finite number. `[formula],(S7:,-592)` becomes `[formula].S7`; `[MCFG:0x00,exp:0xD4]` creates separate `MCFG` and `exp` channels. Every final key, including its group and vector suffix, must fit 128 UTF-8 bytes. Overlong fields are skipped without truncation, with bounded examples in Laboratory diagnostics.

Parser and numeric-framing Apply reuse the last confirmed line filter. Unapplied Include/Exclude/Max CH edits stay unapplied; a failed settings save keeps the active parser and capture. A new manual connection resets the numeric time origin and leaves Snapshot; automatic same-device recovery preserves the existing capture. Session manifests reference same-folder files: when readable logs are requested, every matching `.log` must be present beside its `.bin`, otherwise creation stops and lists the missing logs. Select No explicitly for a raw/index-only session when readable logs were saved elsewhere.

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

## Remembered PORT and bridge recovery

The last **successful** COM connection saves its PORT, baud, data bits, parity, stop bits and Windows device instance ID immediately in `[Serial]` of the portable INI. Next launch restores those choices, but does not open hardware until you press **Connect**. Failed/unconfirmed choices do not overwrite the saved working connection. If the saved bridge is absent, the tool does not silently choose a different port.

If a manual **Connect** fails, **Connection failed** appears once with the PORT and original error (including the Windows error number/system description when the OS can provide one). The warning suggests disconnecting the same PORT in Tera Term/PuTTY/another tool instance, checking the bridge/USB cable and Device Manager COM number, verifying serial settings, then pressing Connect again. Being occupied is a possibility, not a proven identification of the other process; the tool does not close other applications. No selected PORT, invalid settings or a previous pending open also produce an explanatory warning. Failed attempts keep the saved working connection and existing capture. Automatic reconnection failures remain non-modal status/log entries, so they do not repeatedly interrupt you.

After a connected bridge disappears or reports a read error, the status shows waiting/reconnecting. The tool waits for that same identified bridge and retries with the original serial parameters, even if Windows assigns it a different COM number. A different bridge reusing the old number is not opened. Retry delay grows from 2 seconds to at most 10 seconds; absent devices are only scanned once per second. The waiting state continues until the bridge returns or you press **Cancel**. Driver open happens in the background; one pending open at a time, with late results discarded after cancellation. Repeated identical failures are rate-limited in the log, without modal dialogs.

Recovery keeps existing chart samples, table rows, raw Messages and selections; new bytes append after reconnection. An incomplete numeric line from the old connection is not joined to the new stream; Messages retains the exact received bytes. **Disconnect/Cancel** deliberately stops automatic recovery. No received data is not itself a disconnect. A device without a unique stable identity/enumerated serial interface needs manual reconnection rather than unsafe guessing. Recovery reopens the serial device only, without firmware commands, command replay or target reset requests; the existing DTR/RTS enable behavior remains, so adapters/targets that reset on those signals may still reset when reopened.

Recovery depends on the bridge's stable identity and driver behavior. Confirm compatibility with the actual adapter and target. This feature applies to the native MFC EXE.

Connect/Cancel/Disconnect share a fixed width measured from the longest caption; serial controls/pages no longer move as the state changes. The COM field uses the row's remaining width so the last Stop field still fits at narrow/enlarged-font layouts. Only connection-related captions, enabled controls, button accent and status are updated.

## Portable configuration

The MFC tool creates `SimpleComChart.ini` beside its EXE. It stores applied Include, Exclude, Regex and MaxChannels, four selected field names in `[FourCharts]`, Messages font size, the startup-page choice in `[UI] StartupTab`, and the last successful serial connection in `[Serial]` (`Port`, `DeviceInstanceId`, `Baud`, `DataBits`, `Parity`, `StopBits`). Serial interface paths are never loaded from INI. Nonempty chart names use a `key:` prefix to preserve quoted field names through Windows profile APIs; empty means `(none)`. User selector changes and Reset save immediately. Unapplied filter edits are not saved. Run from a writable directory.

Table field choices, chart time windows, ASCII/HEX mode, Auto-scroll and Show name remain per-session UI state.

## Build from source

Requires Visual Studio 2022 C++ desktop workload, MFC x64, Windows SDK and CMake.

```bat
scripts\build_release.bat
```

Outputs: `build\SimpleComChart.exe` and optional-use `build\SimpleComChartCli.exe`. The GUI runs independently of the CLI helper. Build Debug with `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug`.
See [native build guide](docs/MFC_BUILD.md) for build options.

Use `scripts\clean_mfc.bat` (or compatible `scripts\clean_build.bat`) to remove verified compiler/CMake intermediates, link maps, core-test executables, non-Release output folders, check-only `verify\runtime-bin` copies and Python bytecode. Formal root EXE/CLI/catalogs/INI, `build\log` recordings, compact `verify\evidence` and unknown files are preserved.

## Project structure

```text
simple_com_chart_gui/
  src/                 MFC C++ sources, SimpleComChart.rc and native app.ico
  scripts/             build, clean and verification entry points
    build/             alternate build/clean wrappers
    setup/             existing Visual Studio setup helpers
  build/               SimpleComChart.exe and portable configuration
    obj/               generated compiler and CMake intermediates
    Debug/             generated Debug executable
    verify/            generated isolated verification output
  tests/               native/Python tests and telemetry fixtures
  docs/MFC_BUILD.md    native module and build notes
  CMakeLists.txt       root native build definition
  src/python/          maintained Python main/core/ui/utils and icon
  docs/images/         historical documentation screenshots
  main.pyw             compatible Python launcher
```

Generated build and verification directories appear when their respective
commands run and may be absent from a cleaned project. The portable Release
uses `build/SimpleComChart.exe` and its configuration beside the EXE.

## Python source

`main.pyw` uses PyQt5, pyqtgraph and pyserial. It retains its own feature set and the original comma-separated `key:value` / `key=value` parser, including slash vectors and units. Bare scalars, grouped/separated fields, bracket fields and readable-RX prefix handling described above are native MFC features; Python does not currently implement them. The portable distribution is the MFC EXE.

Keep portable INIs and logs private when sharing the executable; they may contain device settings or captured data.
