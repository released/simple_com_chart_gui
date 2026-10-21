# Native MFC implementation

Native implementation notes for [Simple COM Chart](../README.md). Sources are in root `src/`, scripts in root `scripts/`, and CMake configuration in root `CMakeLists.txt`. Static MFC and static CRT are enabled; the shipped Release EXE does not require Python, Qt or VC runtime installation.

## Build

From the project root, run `scripts\build_release.bat`. Output: `build\SimpleComChart.exe`.

The CMake/VS/all build entries in root `scripts\build` delegate to that canonical entry. Intermediate files are under `build\obj\cmake`. Debug output is `build\Debug\SimpleComChart.exe`. The prior cache was preserved in `build\obj\cmake_pre_layout` during the source-directory migration; normal clean can remove it as a compiler intermediate.

The native RC/icon files are `src\SimpleComChart.rc` and `src\app.ico`. Historical EXEs formerly inside the old container are preserved under `build\legacy_native_mfc`; use `build\SimpleComChart.exe` for the current release.

## Modules

- `log_parser` and `line_filter`: UI-independent numeric parsing and raw-line filters.
- `channel_model`: bounded signed-double histories and channel selection.
- `capture_store`: sparse chronological receive rows and immutable export snapshots; separate from rolling charts.
- `capture_export`: atomic text/CSV/BIFF8 XLS export using built-in Windows structured storage (ole32), no Excel/library runtime.
- `data_page`: selectable virtual table columns with frozen header/optional follow, explicit capture-column mapping, bounded latest-value cache and four independent field/time/chart instances. Table selection never restricts capture/export.
- `plot_view` / `plot_view_labels` / `channel_panel`: waveform drawing, latest-point markers and fitted name:value/numeric-only labels, float values and checkboxes. Label drawing is a separate source unit shared by both tabs.
- `mfc_main_dialog_ui`: controls and layout.
- `mfc_main_dialog_filter`: Apply, portable INI and demo input.
- `mfc_main_dialog_export`: Save As dialog, reusable worker and completion/close guard.
- `runtime_smoke`: isolated startup/UI logic checks.
- `serial_manager`: synchronized Windows serial I/O.

The root README describes filter semantics and supported telemetry. Settings stay in `SimpleComChart.ini` beside the EXE; USB-UART adapter drivers are hardware-dependent.
