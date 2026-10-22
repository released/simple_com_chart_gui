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
- `data_page` / `data_page_table`: selectable virtual table columns with frozen header/optional follow, left-aligned bounded content sizing, explicit capture-column mapping, and four independent field/time/chart instances with remembered none/field choices and Reset. Table selection never restricts capture/export.
- `message_buffer` / `message_page`: bounded raw receive history, ASCII escapes/exact HEX formatting and independent monitor controls; raw bytes are captured before text line splitting/filtering.
- `plot_view` / `plot_view_labels` / `channel_panel`: waveform drawing, latest-point markers and fitted name:value/numeric-only labels, float values and checkboxes. Label drawing is a separate source unit shared by both tabs.
- `mfc_main_dialog_ui`: controls and layout.
- `mfc_main_dialog_help` / `hover_help`: measured owner-draw selected tabs and native tooltip definitions. Actual child-HWND tools use automatic TTF_SUBCLASS routing; separate parent-area tools cover transparent labels/disabled children with resize-aware rectangles. No rewritten PreTranslateMessage relay. TOOLINFO uses the compatible V1 prefix (no themed-control manifest requirement); text pointers refer to process-lifetime literals and the dialog explicitly destroys its tooltip.
- `mfc_main_dialog_filter`: Apply, portable INI and demo input.
- `mfc_main_dialog_preferences`: explicit Settings > Startup tab radio menu, stable `[UI] StartupTab` names, guarded startup application and key-only persistence. Missing/invalid defaults to Chart; failed writes retain the previous confirmed choice. A new INI is created with UTF-16 BOM using CREATE_NEW, never truncating an existing file. Startup preference loads separately from capture/filter reloads and applies before first show; ordinary tab changes do not save it.
- `mfc_main_dialog_export`: tab-aware Save As dialog (Messages ASCII/HEX .log/.txt versus filtered chart formats), reusable worker and completion/close guard. `capture_export` shares one atomic writer for both paths; Messages text does not fabricate capture rows.
- `button_feedback`: neutral grey/red toolbar palette and lifetime-safe synchronous command feedback.
- `runtime_smoke` / `runtime_messages` / `runtime_preferences` / `runtime_buttons`: isolated startup/UI, raw-byte display, portable selections/reset, table sizing and actual button-pixel checks.
- `runtime_message_export`: current ASCII/HEX snapshot serialization with no filtered rows, mode/receive/Clear isolation, worker/busy/close guards, safe failures/replacement and retention offsets. Does not automate the native file dialog.
- `runtime_message_font`: actual Messages font -/+ commands, point-size bounds, immediate portable INI persistence/restore, content/scroll isolation, GDI replacement lifetime and large-font layout evidence. Also verifies all eight toolbar controls share the native closed-combo field's top/bottom/center and single-line labels are vertically centered at multiple widths/body sizes. `message_page` owns only its replaceable body font; toolbar fonts remain borrowed.
- `runtime_ui_help`: named tab routing/default selection, native tab/message color contracts, tooltip target/text/geometry checks and deterministic popup screenshots without cursor movement.
- `serial_manager`: synchronized Windows serial I/O and SetupAPI stable instance/interface enumeration.
- `serial_connection`: pure port/settings validation, unique identity lookup and explicit idle/open/connected/wait/reopen state with bounded retry backoff.
- `serial_open_job`: one background open result with cancellation, no retained dialog pointer and late-success disposal. A still-blocked cancelled driver call prevents another open worker, rather than accumulating threads.
- `mfc_main_dialog_serial` / `mfc_main_dialog_serial_settings`: receive-worker lifecycle, measured connection/cancel UI, same-bridge hotplug recovery, last-successful `[Serial]` persistence and startup selection restoration (no startup auto-open). Manual connects clear history as before; automatic recovery appends. INI cannot supply interface paths.
- Manual Connect failures route through one owned native warning with PORT/error/checklist after restoring retry-ready controls and closing rejected transports. OS open failures preserve the numeric code and append FormatMessage text when available. The UI timer guards teardown after modal dispatch; background retries remain non-modal. Runtime fixtures inject a warning dispatcher only for assertions, restoring the real MessageBox entry afterwards.
- `runtime_serial` / `runtime_serial_fixture`: injected fake enumeration/transport tests for INI, removal/read failure, occupied/reused/renumbered ports, history preservation and cancellation. No actual COM open; physical adapter behavior remains manual acceptance.
- `runtime_connection_redraw`: connection-state-only geometry/caption/probe checks across all pages and narrow/wide layouts. `--smoke-test=serial` / `verify_runtime.ps1 -Scope Serial` selects this and fake serial regressions only, writing a scope-specific summary in a separate fixture directory. Normal layout reserves the widest connection caption; `update_connection_ui` must not invoke whole-dialog layout.
- `runtime_startup_tab`: `--smoke-test=startup` / `verify_runtime.ps1 -Scope Startup -StartupTab Messages|Chart|4Charts` seeds an isolated main INI and verifies actual process startup plus menu/INI/fallback/read-only/data-preservation cases. Full All scope also includes the regression and explicitly establishes Chart only for its later existing chart tests; it must not assume the user's startup page is always Chart. No COM open or unrelated suites in Startup scope.

The root README describes filter semantics and supported telemetry. Settings stay in `SimpleComChart.ini` beside the EXE; USB-UART adapter drivers are hardware-dependent.
