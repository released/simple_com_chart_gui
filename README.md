# Simple COM Chart GUI

Windows COM telemetry monitor with a portable native MFC application and a
maintained Python frontend. The native tool combines raw serial messages,
selectable numeric charts, recording, replay and UART diagnostics.

## Run

Copy these files together to a writable folder on Windows 10/11 x64:

- [SimpleComChart.exe](build/SimpleComChart.exe)
- [lang_tw.ini](build/lang_tw.ini)
- [lang_en.ini](build/lang_en.ini)

The native application uses static MFC/CRT and runs without Python, Qt or the
Visual C++ redistributable. A USB-UART adapter may require its manufacturer's driver.
The interface defaults to Traditional Chinese; Settings > Language selects the
language for the next launch.

## Basic operation

1. Select COM, baud, data bits, parity and stop bits, then press Connect.
2. Use Messages for raw received text or HEX. New channels start unchecked;
   saved four-plot selections are restored from the personal INI. Select the
   required signals for display.
3. Enter an optional Include or Exclude filter and apply it. Text mode matches
   one literal string; Regex supports alternatives such as fan|ac_adc_mv.
4. Use Save As for a retained-display snapshot, or Start recording / Stop
   recording for continuous received data. Default filenames use PC date and time.
5. Disconnect before opening an offline replay file. Replay never transmits to a port.

| View | Purpose |
| --- | --- |
| Messages | Raw ASCII/HEX/ANSI display, search, font size, auto-scroll and Save As |
| Chart | Selected numeric channels on a shared plot with automatic or fixed Y scaling |
| 4 Charts | Selectable table columns and four independent plots, each starting at (none) |
| Workbench / Dual COM | Two simultaneously visible, vertically stacked port groups with separate logs and controls |
| Console, Laboratory and advanced windows | Explicit sending, recording analysis, profiles and protocol tools |

Chart and each small plot have independent editable time ranges: 250 ms,
500 ms, and every whole second from 1 to 300. Invalid input keeps the previous range.

## Data and filters

The native default parser recognizes comma-separated key:value or key=value
fields, units, hexadecimal values and slash vectors. For example:

~~~text
fan,duty:920,target:2340,fg_hz:69.1,err:11%,cmd:0x3202,cap:105/76/29
ac_adc_mv:1169,ac_v:116,ntc_c:76,cnt:554
~~~

| Input | Numeric result |
| --- | --- |
| fg_hz:69.1 | fg_hz = 69.1 |
| err:11% | err = 11; percentage magnitude |
| cmd:0x3202 | cmd = 12802 |
| cap:105/76/29 | cap[0] = 105, cap[1] = 76, cap[2] = 29 |

- Include must match; Exclude takes priority. Both operate on the raw line before numeric parsing.
- Empty filters accept every valid numeric field. Arbitrary nonnumeric text remains a message, without a fabricated waveform.
- Field extraction also offers CSV/Tab, numeric JSON paths and bounded Regex rules with a separate preview/apply step.
- Raw RX and recorded bytes remain separate from display aliases, scaling and terminal presentation.

## Recording and UART tools

- Save As exports the retained display. Continuous recording writes raw RX,
  receipt indexes and optional readable logs independently of display history.
- Dual COM keeps A and B in separate visible groups; Port A follows the main
  connection and Port B is independent.
- Console provides explicit Text/HEX sending, history, presets, serial controls,
  Break and driver-reported statistics.
- Workbench provides packet framing, binary charts, command groups, bounded
  scripts, TX pacing, X/YMODEM, read-only Modbus RTU and VT100-style display.
- Laboratory provides parser/packet previews, recording-range export, statistics,
  digital-state history, named views, PNG/HTML reports and bounded FFT/waterfall views.
- Profiles stage preferences before explicit idle-only Apply. Importing a profile
  does not connect, transmit or replay previous commands.

Physical pin effects, device acknowledgements and electrical timing depend on
the selected adapter and target. Driver-accepted TX is distinct from device acceptance.

## Connection and private settings

- The portable SimpleComChart.ini stores applied settings and the last successful
  port configuration. Startup restores preferences without opening hardware.
- Automatic recovery follows the same identified bridge, using bounded retries.
  It preserves the capture and never replays TX; Disconnect/Cancel stops recovery.
- Failed manual connections show the port and available Windows error details.
  An occupied port is a possible cause; other applications are not closed automatically.
- Reopening can reproduce DTR/RTS-related target effects. Devices without a stable
  unique identity require manual reconnection.
- Personal INIs, command histories, captures, logs and profiles remain private.

## Architecture

~~~mermaid
flowchart LR
    COM[COM port RX] --> Raw[Raw byte buffer and receipt positions]
    Raw --> Messages[Message display]
    Raw --> Recorder[Continuous recording]
    Raw --> Filter[Line filter and field extraction]
    Filter --> Capture[Numeric capture]
    Capture --> Charts[Chart and four independent plots]
    Replay[Offline replay] --> Raw
~~~

| Path | Purpose |
| --- | --- |
| src/ | Native UI, serial transport, parsing, recording and analysis |
| src/python/ | Python frontend and its own supported feature set |
| scripts/ | Build, clean and verification entries |
| tests/ | Native/Python tests and synthetic fixtures |
| docs/ | User guides and detailed feature references |
| build/ | Formal EXEs and language catalogs; obj/ holds compiler intermediates |
| CMakeLists.txt | Native build definition |

## Build and Python source

Native development requires Visual Studio 2022 C++ desktop tools, MFC x64,
Windows SDK and CMake. From the project root:

~~~bat
scripts\build_release.bat
~~~

Outputs are build/SimpleComChart.exe and the optional finite-script helper
build/SimpleComChartCli.exe. The GUI runs independently of the CLI helper.
The [native build guide](docs/MFC_BUILD.md) describes configurations and safe clean.
The root github_push.bat previews and publishes reviewed repository updates.

The compatible main.pyw launcher uses PyQt5, pyqtgraph and pyserial. Python keeps
its own feature set and original key/value parser; native-only parsing, UART and
workbench features are identified in the guides. The portable delivery is native MFC.

## Guides

- [繁體中文操作指南](docs/USER_GUIDE.md)
- [Complete English user guide](docs/USER_GUIDE_EN.md)
- [Workbench and Dual COM](docs/UART_WORKBENCH_GUIDE.md)
- [Laboratory and CLI](docs/UART_LAB_GUIDE.md)
- [Replay, profiles and analysis](docs/ADVANCED_UART_TOOLS.md)
- [Recording and session formats](docs/UART_SESSION_GUIDE.md)
- [Field extraction and navigation](docs/UART_OPTIMIZATION_GUIDE.md)
- [UART analysis](docs/UART_ANALYSIS_GUIDE.md)
