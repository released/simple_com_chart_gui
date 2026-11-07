# Native build and architecture

Simple COM Chart uses C++17, static MFC and static CRT on Windows x64.
The native GUI requires no Python, Qt or separately installed VC runtime.

## Build requirements

- Visual Studio 2022 with Desktop development with C++ and MFC x64.
- Windows SDK and CMake, using the Visual Studio installation when available.
- Run the following commands from the repository root.

~~~bat
scripts\build_release.bat
~~~

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration Debug
~~~

| Output | Location |
| --- | --- |
| Formal Release GUI | build/SimpleComChart.exe |
| Optional finite-script CLI | build/SimpleComChartCli.exe |
| Release language catalogs | build/lang_tw.ini and build/lang_en.ini |
| Debug GUI | build/Debug/SimpleComChart.exe |
| Compiler/CMake intermediates | build/obj/cmake/ |

The paired UTF-8 language catalogs are copied from src/i18n to the selected
GUI output directory. The personal SimpleComChart.ini remains separate.

## Build profiles

- The normal build produces the GUI, CLI and baseline telemetry test target.
  It runs the telemetry_core test by default.
- ExtraTargets and TestRegex select additional native targets and CTest cases
  through scripts/build.ps1. Test source remains under tests/.
- RuntimeChecks enables a separate UI verification build under
  build/obj/cmake-runtime, with EXEs only under build/verify/runtime-bin/.
  Production EXEs exclude runtime UI test code and synthetic telemetry.
- The Release profile uses static linkage and linker dead-code removal.
  Runtime DLL requirements and supported architecture are unchanged by that profile.

## Source responsibilities

| Module group | Responsibility |
| --- | --- |
| serial_manager, serial_connection, serial_open_job | Driver I/O, device identity and bounded connection/recovery state |
| message_buffer, message_page, message_format | Raw receive history and display-only text/HEX presentation |
| line_filter, field_extractor, channel_model, capture_store | Numeric extraction and bounded capture |
| plot_view, data_page, channel_panel | Shared chart, selectable table and independent small plots |
| uart_recorder, uart_files, replay_storage, uart_offline | Recording, guarded file output and background replay |
| uart_operations, uart_console, uart_automation | Explicit transmit operations and finite automation |
| advanced_tools, workbench, laboratory | Separate advanced views and their owned jobs |
| mfc_main_dialog_* | Composition, settings, menus, layout and worker completion |
| src/i18n | English and Traditional Chinese presentation catalogs |

Raw RX positions remain the common reference for display, table and chart
navigation. Display options do not rewrite recorded bytes. Driver-accepted TX
and software simulation are distinct from target acknowledgement or electrical timing.

## Clean and local data

~~~bat
scripts\clean_mfc.bat
~~~

- Clean removes recognized compiler/CMake intermediates after producer and path checks.
- Formal EXEs, language catalogs, personal INI and unknown files are preserved.
- Disposable verification files use owned project-local runs and are retired
  by their runners; folder names alone do not authorize recursive deletion.
- Build and verification directories can be absent between runs. The portable
  GUI does not depend on obj/, Debug/ or verify/.

## Operation references

- [Traditional Chinese guide](USER_GUIDE.md)
- [English guide](USER_GUIDE_EN.md)
- [Advanced UART tools](ADVANCED_UART_TOOLS.md)
- [Workbench](UART_WORKBENCH_GUIDE.md)
- [Laboratory](UART_LAB_GUIDE.md)
