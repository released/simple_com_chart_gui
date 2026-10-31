# Simple COM Chart User Guide

[繁體中文使用指南](USER_GUIDE.md)

This guide covers the native Windows MFC application: monitoring UART/COM data,
extracting and plotting numeric fields, saving recordings, and explicitly sending
commands or files. The Python frontend has a separate interface; the language
settings described here apply to the native EXE.

## 1. Install and start

Keep these files together in a writable folder:

```text
SimpleComChart.exe
lang_tw.ini
lang_en.ini
```

Windows 10/11 x64 is supported. Python, Qt and the Visual C++ redistributable are
not required. A USB-UART bridge still needs a suitable Windows device driver;
the application does not replace that driver.

Personal settings are stored in `SimpleComChart.ini` beside the EXE. Use a writable
folder. Only one normal application instance runs per Windows session; launching
another copy activates the existing window.

## 2. Language and startup tab

The default language is **Traditional Chinese**. Choose **Settings > Language
(next launch) > English / Traditional Chinese**, close the tool normally and start
it again. Saving this choice does not rebuild windows, reconnect, transmit or
clear the current capture.

The preference is stored in the personal INI:

```ini
[UI]
Language=en
```

`en` selects English and `tw` selects Traditional Chinese. A missing or invalid
preference defaults to Traditional Chinese. Catalog files use UTF-8; preserve the
text IDs and escape sequences when editing translations. A missing or invalid
selected catalog displays a notice and uses English fallback. Missing individual
entries retain their English source text.

Raw RX/TX, field names, aliases, file formats, protocol tokens and some lower-level
diagnostics retain their original form. Windows file-picker controls and OS error
messages follow the operating system's language.

**Settings > Startup tab** selects Messages, Chart or 4 Charts for the next launch.
Chart is the fallback. Ordinary tab clicks do not change this explicit preference.

## 3. Connect and recover a connection

1. Attach the bridge and click **Scan**.
2. Select COM, baud, data bits, parity and stop bits.
3. Click **Connect**. Successful device/settings are remembered. Startup restores
   the selection without opening the port automatically.
4. On failure, follow the prompt: check whether another application owns the
   port, the USB connection, Device Manager, driver and serial settings.

Unexpected bridge removal or read failure starts recovery for the same identifiable
device, including a changed COM number. Retry delay grows to at most 10 seconds.
Cancel or manual Disconnect stops recovery. Unknown or ambiguous devices require
manual selection; a different device reusing a COM number is not selected as a
substitute. Recovery preserves captured data/selections and never replays commands.

## 4. Main tabs

| Tab | Purpose | Important behavior |
|---|---|---|
| Messages | Raw receive monitor | Independent of numeric Include/Exclude, green text on black |
| Chart | Selected signals on shared axes | New fields start unchecked; Y follows visible selected data |
| 4 Charts | Selected-field table and four independent plots | Table choices and four plot selectors are independent |

The active tab has a dark grey background and white text. Engineering-style grey
action buttons change color briefly while executing; Connect is red when connected.

## 5. Messages, search and font

Select ASCII or HEX, or UTF-8/ANSI in Options. HEX shows bytes and offsets; ASCII
escapes control and non-ASCII bytes that cannot be shown directly. Display modes
do not modify the original received bytes.

Use **Font − / +** for 8..32 pt message text. Changes save immediately to INI.
Options include newline handling, wrapping, receipt timestamps, ANSI SGR colors,
and a light CR/backspace/erase-line mode. This is not full terminal emulation.

**Auto-scroll** follows new data. Disable it to inspect older messages. **Pause
display** pauses rendering while receive/recording continue. **Clear messages**
does not stop the connection or recording.

`Ctrl+F` opens search; `F3` / `Shift+F3` finds next/previous. Literal, case-sensitive
and bounded Regex modes are available. `Ctrl+F2` adds a bookmark; `F2` finds the
next bookmark. Expired history or reformatting can invalidate search/bookmark
positions. Message history is limited to 256 KiB; history expiry and input loss
are reported separately.

## 6. Numeric parsing and filters

The default framing ends a sample in LF/CRLF. For example:

```text
fan,duty:920,target:2340,fg_hz:69.1,offset:-2.5,err:11%,cmd:0x3202,cap:105/76/29
```

Use `:` or `=` between a name and value. Signed integers, decimals, scientific
notation, hexadecimal and ASCII unit suffixes are supported. `11%` is numeric 11;
`cap:105/76/29` creates `cap[0]`, `cap[1]`, `cap[2]`. Missing or invalid values
remain blank rather than becoming zero.

`[MCFG:0x00,exp:0xD4]` creates `MCFG=0` and `exp=212` separately;
`[formula],(S7:,-592)` creates `[formula].S7=-592`. Final names, including group
and vector suffixes, must fit 128 UTF-8 bytes. Overlong fields are skipped without
truncation and appear in Laboratory diagnostics.

Include/Exclude operates on each complete raw line before numeric extraction:

| Setting | Example | Effect |
|---|---|---|
| Text Include | `fan,` | Keeps a line containing the exact case-sensitive substring |
| Regex Include | `fan,\|ac_adc_mv:` | Keeps either matching line type |
| Regex Exclude | `error\|timeout` | Discards either match, even when Include matches |

Comma and semicolon do not split conditions. Empty Include accepts all; empty
Exclude rejects nothing. **Apply** starts a new numeric capture. Invalid settings
keep the previous valid settings/data. Max CH defaults to 256, range 1..1024.

**UART > Field extraction** also supports CSV/Tab, flat JSON, bounded Regex groups
and explicit nested JSON Pointer selections:

```text
ptr:rpm=/fan/rpm,temp=/temps/1
```

Input `{"fan":{"rpm":201},"temps":[25,30]}` produces `rpm=201`, `temp=30`.
Preview does not apply settings; use **Apply parser** after reviewing the result.
Parser/framing Apply reuse the last confirmed line filter; unapplied main-window
filter edits remain unapplied. Failed saves keep active settings and capture.
A new manual connection resets the numeric clock origin and leaves Snapshot;
automatic same-device recovery continues the existing capture.
Laboratory **Numeric input framing** independently configures A/B/offline CR,
CRLF, custom HEX delimiters or host-receipt idle gaps. Raw recording is unchanged.

## 7. Chart and four independent plots

New fields start unchecked. Select individual fields or **All**. Click the Channel
header to change name sorting; search filters the visible choices. **Channel
display** adds aliases, units, scale and offset without changing raw/export values.

**Refresh** clears rolling chart samples; **Fit** recalculates axes; **Snapshot /
Live** freezes/resumes plotting; **Overlay** controls latest-point/value markers.
These actions do not erase the captured log or Messages. **Auto Refresh** periodically
clears Chart samples; it is not the receive or UI-update frequency. Hover for help.

Use wheel zoom, middle/Ctrl-left/Space-left pan, A/B cursors, double-click to return
to latest, and fixed Y ranges. Automatic Y follows the selected data; the empty
0..50 fallback does not limit actual measurements.

Chart **Time Window** and each small plot's time field offer `250 ms`, `500 ms`
and every second from `1 s` to `300 s`. You can type seconds such as `201` or
`0.25`, or include `ms` / `s`. Press Enter or leave the field to apply. Valid
input is 0.25..300 seconds; invalid input keeps the previous setting. Small
plots remain independent, with fractional-second X ticks for short windows.
This does not change receive/update frequency or increase the sample rate.
Named views can save these ranges; enlarging a window cannot recover expired
rolling samples.

The 4 Charts table shows only checked fields, plus Row/Time. Newest rows are at
the bottom; missing cells remain blank. Disable **Auto-scroll to newest** to stay
on older rows. Each small plot has its own field/time window/Y axis and starts at
`(none)`. Plot choices save to INI. **Reset to none** also clears the saved choices.

## 8. Save As and continuous recording

Default filenames use local PC date/time to seconds, such as
`20261026_143025.log`; you can edit them.

Messages places **Save As / Start recording / Stop recording** beside **Find**.
Save As saves the currently retained display snapshot; empty history gives a
visible explanation without creating an empty file. Start recording saves future
RX continuously. Start is disabled while recording; Stop is enabled. Both are
disabled while queued data finishes writing, then Start becomes available again.
The area below shows saved/current recorded log filenames and sizes without paths.
KB means 1024 bytes; recording size belongs to the named current `.log` segment,
not raw payload or all rotation segments combined. Recording adds `_1.log`,
`_2.log`, etc., and retains the companion files described below.

| Source | Formats | Saved contents |
|---|---|---|
| Messages Save As | `.log` / `.txt` | All retained messages in the current display format, not raw binary |
| Chart Save As | `.log` / `.txt` | Original accepted filtered lines |
| Chart Save As | `.csv` / `.cvs` / `.xls` | All retained fields/rows, independent of visible checkboxes |
| UART Start recording | `.bin` + `.idx` + `.log` + `.sccmeta` | Continuous RX, receipt times/events, readable TX/RX and safe start settings |

Save As exports the retained snapshot after filename confirmation, excluding later
receive data. Numeric capture retains at most 10000 lines / 32 MiB. XLS supports
254 numeric fields plus Row/Time; use CSV/log for wider data. Background export
preserves an existing destination on failure.

Continuous recording is independent of message clearing, display pause and chart
filters. Raw `.bin` contains RX only. A line containing only one complete finite
number uses the stable `value` channel. Generic bracketed fields such as
`[P3:76.1803]` create `P3`; the tool's exact `[HH:mm:ss.mmm] [RX] ` readable-log
prefix is PC metadata, not a channel. In addition to `key:value` / `key=value`, the
default parser accepts `[S07],raw:,-19056` as `[S07].raw`; the separated token must
be a complete finite number. The user chooses the readable numbered
`.log` prefix and folder. The application automatically creates a `log` folder
beside the EXE/INI and keeps `.bin`, `.idx` and adjacent `.sccmeta` files there.
Folder-creation or existing-name conflicts fail visibly and never overwrite old
files. **Recording options** configures size/daily
rotation, total output bytes, duration and minimum free space; zero disables each
guard. Reaching a guard stops recording without disconnecting or deleting old
logs. Options apply to the next recording on A or B.

## 9. Console and serial controls

**UART > Console / Serial options** sends UTF-8 text or HEX with None/CR/LF/CRLF
ending. HEX accepts `41 42` or `4142`, without `0x`. A complete send including
ending is limited to 4096 bytes. Only Send / `Ctrl+Enter` transmits. Selecting
history, loading a preset or previewing never sends automatically.

The INI can retain 32 history entries and 8 presets. These may include private
commands/passwords; encoding is not encryption. Flow control supports None,
RTS/CTS, XON/XOFF and DSR/DTR; handshake ownership limits manual DTR/RTS.
Break is an explicit approximately 250 ms line signal, not Ctrl+C. Pin changes or
Break may reset a device, so review wiring and target state before confirming.

Keyboard TX and readable ASCII local-status echo default on for each new valid
connection; a manual disable lasts for that connection. Controls and non-ASCII
bytes are escaped and never fabricated as RX or raw `.bin` data. Every paste still
requires preview/confirmation; cancel/disconnect never replay keys. Driver acceptance is distinct from
the device completing or acknowledging an operation.

## 10. Dual COM Workbench

**UART > Workbench / Dual COM** shows two simultaneously visible stacked groups.
A follows the main connection; B is independently selected/connected. Receive
windows, control, sending, Save As and recording remain separate.

Tools includes packet framing, typed binary parsing, B text charts, read-only
field/bit inspection, named commands, finite step scripts, TX pacing, file transfer,
read-only Modbus RTU, a monochrome terminal subset, instruments/FFT, XY and run
comparison. A/B forwarding requires explicit confirmation and finite limits.
Stop, error, history gap or disconnect stops it without automatic resume.

## 11. Offline replay, profiles and analysis

Completed raw/index rotation segments replay using recorded receipt timing;
unindexed files use visibly synthetic timing. Load starts paused. Speed control,
record/time seek and cross-segment Text/HEX search never transmit to UART.
Background reading and a paged index support up to 64 GiB / 50 million records /
4096 segments; these bounds do not imply equivalent RAM allocation.

Named `.sccprofile` files store safe A/B workspace settings. Loading stages only;
explicit confirmed Apply requires idle disconnected ports and unloaded replay.
Private commands/history, recording paths and auto-start state are excluded.
Language is an independent personal preference and is not overwritten by profiles.

Session manifests reference same-folder files only. When readable logs are
requested, every `.bin` segment needs its adjacent `.log`; missing names are
listed and creation stops. Select No explicitly for a raw/index-only session
when readable logs were saved elsewhere; those logs are not in the manifest.

Chart analysis offers cursor differences, interval min/max/mean/RMS/standard
deviation and a reference line. Bounded derived expressions/EMA do not reuse stale
missing inputs. FFT requires suitable sample timing; it does not measure UART bits.

## 12. Laboratory tools

| Page | Purpose and operation |
|---|---|
| Diagnostics | Observe filtering, parsing and software gaps; not proof of wire loss |
| Recording / range export | Check segments, export ranges or save a valid prefix without in-place repair |
| Multi-parameter packet | Named parameters, length/manual sequence, checksum and COBS/SLIP; preview then confirm one send |
| Sequence / response statistics | Observe discontinuities and host timing from the latest finite script |
| Offline parser cases | Import/run/save bounded CSV cases without COM operations |
| Binary schemas / codecs | Typed fields and layouts selected by Message ID |
| Digital state history | Plot reported bit/enum states without extending across gaps |
| Recording sessions | Relative filenames + SHA256, verified safe setting staging, no raw duplication |
| COM device information | Read-only driver-published name, VID/PID, location and more; no guessed missing serial |
| Numeric input framing | Independent text completion rules for A/B/offline |
| Custom CRC | CRC8/16/32 parameters/ranges shared by TX/RX cores |
| Time derivative / integral | Retained raw-data calculation/export with gaps and generation boundaries |
| Passive transactions | Observe ID/prefix matches, timeouts and ambiguous overlap without TX |

`.sccmeta` describes recording-start settings, not a complete change history;
later numeric-setting changes are marked. SHA256 identifies file content, not
authorship or correct target behavior.

## 13. Troubleshooting

| Symptom | Check |
|---|---|
| Connect fails | Release the COM port in Tera Term/PuTTY/etc.; check USB, driver and serial settings |
| Text arrives but no plot | Review format/framing, Include/Exclude and parser preview, then enable desired numeric fields |
| Small signals are compressed | Use independent Y axes in 4 Charts or select fewer shared-axis fields |
| Updates move the inspected text | Disable Auto-scroll; display pause still allows recording |
| Older data disappears | Inspect retention/expiry/loss counters; use continuous recording for longer sessions |
| Unexpected English/fallback | Check saved language, restart, catalog location and UTF-8 format; technical diagnostics may remain English |
| INI cannot save | Use a writable folder and remove the INI read-only attribute |

## 14. Detailed formats and limits

- [UART display, operations and large replay](UART_OPTIMIZATION_GUIDE.md)
- [Dual COM, binary, scripts, transfer and FFT](UART_WORKBENCH_GUIDE.md)
- [Numeric analysis, profiles, XY and run comparison](UART_ANALYSIS_GUIDE.md)
- [Laboratory, parser cases and finite CLI](UART_LAB_GUIDE.md)
- [Sessions, framing, CRC, JSON paths and calculus](UART_SESSION_GUIDE.md)

Some specialist guides and protocol/low-level diagnostics use English. Follow the
actual target protocol for automation/transfers. Host receipt clocks and fake
transport checks do not replace physical UART/hardware acceptance.

## 15. Views, cases, notes and analysis delivery

Open **UART > Laboratory / Diagnostics** and choose a page from the top selector.

| Page / operation | Workflow |
|---|---|
| Named chart views | Select main channels, four plots, windows/fixed Y; save a named view, then stage and explicitly Apply it later |
| Multi-parameter packet > Edit parameters | Add, update or remove name/value/minimum/maximum rows; OK updates parameters without sending |
| Chart image export | Choose Chart or 4 Charts, set dimensions and save PNG with current axes, curves and latest labels |
| Offline parser cases > Use selected capture row | Select a retained 4 Charts table row; review copied text/expected values, append, then Run cases explicitly |
| Recording notes | Choose a completed file, byte offset/count and note; save a private `.sccnotes` sidecar; loading verifies its source |
| Profile Apply | Review Current/Staged differences; select a row for full values; Cancel leaves settings unchanged |
| HTML analysis report | Enter one exact key per line or use checked main fields, choose an interval and save self-contained HTML |
| FFT waterfall | Select a key, FFT samples, rate, history rows and dB limits, then Start; newest rows are at the bottom |

Named views live in `views/name.sccview` beside the EXE and support import/export.
Names accept 1..48 ASCII letters, digits, underscores or hyphens, excluding
reserved Windows device names.
They restore display choices and time/Y ranges only, without changing COM,
parsing or captured data. Return Snapshot to Live before Apply. Missing keys are
listed and stay unselected even if they arrive later. Newly discovered fields still start unchecked; previously user-saved four-plot
choices retain their existing INI behavior. Expired rolling samples are not recovered.

The parameter grid allows 32 unique names; `length` and `sequence` are reserved.
Values must be finite and within their bounds. Preview the resulting packet and
use the existing confirmed Send once afterward; grid editing never sends.

PNG requires at least 400x300 for Chart or 800x600 for 4 Charts, at most 4096 per
dimension and 16 million pixels total. Filenames default to local date/time to
seconds. After filename confirmation the current view is rasterized, then
encoded/saved in background. Snapshot remains a freeze control; none plots stay empty.

Captured cases accept retained text rows; binary numeric display descriptions
are not parser input. B text removes only its producer-recorded display prefix.
The case editor's parser is used without applying a live parser. Expected values
start as observations from that parser: review/edit them before accepting, rather
than treating them as independent proof. Existing bounds remain 64 cases/64 KiB,
4096 input bytes each. Expected key names cannot contain comma, colon or equals.

Notes use zero-based byte offsets and optional zero count for a point marker.
Spans must fit the source. Limits are 256 notes, 4096 UTF-8 bytes per note and
1 MiB per sidecar. Save beside the recording; relative filename, length and
SHA256 verify identity without modifying raw contents. To locate, first load
the identical single-file recording with the same filename in offline replay,
then select the note index. Locate pauses at the containing record; Play is a
separate action. Rotated multi-file sources are unsupported for note navigation;
EOF markers do not guess a last record. Clear staged notes never deletes files.

Profile review compares supported workspace settings, excluding private history
and unsupported INI sections. Apply still requires idle ports/background jobs
and starts a new capture. Changed preferences during review require a fresh review.
Long list values are shortened; the selected row's detail pane shows complete values.

HTML supports up to eight fields over an inclusive capture-relative interval;
empty end means the latest retained data. It includes raw numeric summaries and
static SVG with missing/generation boundaries, excluding later RX, complete raw
recordings and private commands. Open in a browser without network, JavaScript
or installed components. Failed PNG/HTML writes preserve an existing destination.
Each file-work page has Cancel file work. It requests cancellation of the current
background job; a save that already completed is not undone.

Waterfall uses Hann/mean-removed FFT: 32..4096 power-of-two samples, 1..256 rows,
and increasing dB limits within -200..200. Rate 0 validates approximately uniform
source clocks. Batched UART receipt often fails this condition; specify a device
rate only when known. There is no resampling or UART-bit timing measurement.
At most two background frames are scheduled per second; unchanged samples are
not recomputed. Stop or hiding the page prevents further scheduling. Gaps,
generation/frequency-axis/configuration changes clear history; enough contiguous
samples are needed before it resumes drawing.
