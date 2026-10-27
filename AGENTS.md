# AGENTS.md

## Workspace Layout And Safe Clean

Use the hybrid profile already described here. Screenshots live under docs/images. Both clean entries preflight the complete build tree, verify CMake producer identity and remove only recognized generated files; a file inside obj/tests is not automatically disposable.

Python cache cleanup is file-only (.pyc/.pyo), not recursive cache-directory deletion. Preserve EXE/INI/unknown cache contents too; expanded fixtures must check both native and source/cache junctions before native deletion.


## Project Scope

- Text/binary serial telemetry monitor and native UART workbench; the maintained Python frontend has its own feature set.
- Native output: `build\SimpleComChart.exe` for Windows 10/11 x64.
- Use static MFC and static CRT; execution must not require Python or the VC redistributable.
- Native delivery excludes `runtime_*.cpp` UI self-tests AND synthetic telemetry. `SCC_ENABLE_RUNTIME_CHECKS` defaults OFF; `build.ps1 -RuntimeChecks` uses `build/obj/cmake-runtime` and outputs ONLY under `build/verify/runtime-bin/<Configuration>`. `verify_runtime.ps1` builds/uses this separate artifact. `--demo` is removed/rejected (exit 2); production also rejects `--smoke-test`. Only check-enabled smoke runs may use the private test feeder in `runtime_test_telemetry.cpp`, never normal sessions. Never overwrite delivery with a check-enabled binary or re-add demo UI/help/sample values without an explicit user requirement.
- Dialogs use `CDialog` and `_AFX_NO_MFC_CONTROLS_IN_DIALOGS`: this tool has standard Win32/common/custom controls, not Feature Pack CMFC controls. If adding CMFC controls, review this restriction before changing resources. Release size optimization uses `/O1 /GL /Gw /LTCG /OPT:REF /OPT:ICF`; keep static runtime, x64 and security defaults. Link maps stay under the selected obj directory. Do not apply UPX or switch to runtime DLL dependencies implicitly.
- `VERSION` is the version source; CMake generates native title/resource version definitions.
- Keep the native layout consistent with sibling MFC tools: root `src/`, `scripts/`, `build/`, `tests/` and `CMakeLists.txt`. Do not recreate a nested native source tree.
- `src/SimpleComChart.rc` and `src/app.ico` are the native resources. The maintained Python implementation is under `src/python` (core/ui/utils/main.pyw/app.ico); root `main.pyw` is the compatible launcher. Documentation screenshots are under `docs/images`.

## Contracts

- Messages log controls: Save As is an immutable retained-display snapshot; start/
  stop are independent continuous RX recording. Buttons live beside Find, wrap
  with measured captions, and use mutually exclusive/draining gray states.
  Show only actual .log leaf and current-segment written KB (1024 bytes), not raw
  payload or aggregate rotation bytes. Localize dynamic menus before display.
  Select uart_receive/ui_language and MessageLog tw|en (TW Font125 for layout);
  this scope includes affected export/font checks, not all protocols/Python.

- Chart time inputs: shared chart_time helpers, 250 ms/500 ms and 1..300 s choices,
  editable 0.25..300 s; commit on Enter/focus loss, preserve state on invalid input.
  Model/renderer minimum is 0.25 s; retain legacy long named views/analysis.
  Select telemetry_core/uart_workflow and native ChartTime (TW Font125) / affected
  Charts. Do not rerun unchanged UART/protocol/soak/Python suites for time controls.

- Current approved workflow expansion: docs/UART_WORKFLOW_PLAN.md (eight items).
  Keep native/static architecture, view-only explicit application/default none,
  shared packet validation and confirmed TX, exact text-case provenance, SHA256
  recording-note spans, typed profile review, atomic bounded PNG/HTML output and
  bounded background waterfall. Source catalogs and both complete guides stay in
  sync. Laboratory FieldId/ActionId are separate ranges; never overlap added pages.
  Select uart_workflow core and -Scope Workflow -UiLanguage en|tw (Font125 when
  layout changes). Use -Scope Soak -WaterfallSoak for sustained receive/render
  impact. Reuse unchanged protocol evidence; do not append All/Python suites.

- Native language contract: docs/UI_LANGUAGE_PLAN.md. Default Traditional Chinese;
  UI/Language=en|tw is personal and applies next launch. Catalogs src/i18n/lang_en.ini
  and lang_tw.ini copy beside every native GUI output. Translate presentation only;
  keep canonical dropdown values, raw RX/TX, keys/aliases and file formats exact.
  Never introduce global hooks on received EDIT text. Preserve both complete user
  guides: docs/USER_GUIDE.md (Traditional Chinese), docs/USER_GUIDE_EN.md (English).
  Use -Scope Language -UiLanguage tw|en for native UI checks; unrelated fixtures
  explicitly seed en so their canonical legacy assertions keep their meaning.

- Current approved session/workflow expansion: docs/UART_SESSION_PLAN.md (eight
  features). Keep native/static portability, compatible existing formats and no
  automatic TX. Preserve the uncommitted .028 Laboratory work. Capture metadata
  is a sanitized start snapshot and marks later numeric-setting changes; manifests
  are relative references with file hashes, not duplicate raw archives. A limit
  stops recording, never deletes user logs or closes COM. JSON paths require the
  explicit `ptr:` prefix so old flat JSON with unused columns keeps its meaning.
  Persist pointer selections with the parser/Port B v2 marker; retain v1 for
  ordinary settings. Use `verify_runtime.ps1 -Scope Session` for these six new
  Laboratory pages and affected core targets including `uart_session`. Expand
  to receive/profile/workbench checks only where shared code changed. Automated
  device information uses an injected catalog; never open real ports in fixtures.

- Approved native laboratory scope: `docs/UART_LAB_PLAN.md`; user behavior and
  bounds: `docs/UART_LAB_GUIDE.md`. Keep diagnostics/recording/packet/quality/case/
  schema/digital UI in the owned Laboratory, not extra main-page controls. Native
  codecs/layout directives reuse existing 12-field binary preferences and legacy
  profile readers; optional Lab preferences extend v2 without rejecting older v2.
- CLI is an explicit finite helper sharing the production UARTChart.v1 guard;
  help/validate are read-only. Require run + port/settings + new report + confirm
  flag before hardware, one outstanding TX, stop on error/partial/cancel, never
  retry/reconnect/resume or load personal INI. No private test-mode CLI bypass.
- Select `verify_runtime.ps1 -Scope Lab` for laboratory changes, affected
  `uart_lab`/binary/commands/analysis/profile cores, and scoped receive checks when
  receive metadata changes. `uart_lab_performance` isolates diagnostic CPU cost;
  RX soak is separately selected for receive/render/retention changes. Core
  `build.ps1` defaults to telemetry_core; use TestRegex/ExtraTargets for affected
  targets, not an unconditional full-suite run. Core/runtime TEMP/TMP stays in a
  fresh owned project-local run and is restored/cleaned on every outcome.

- Approved UART workbench scope/acceptance is docs/UART_WORKBENCH_PLAN.md (2026-10-25).
  Dual COM must show two simultaneously visible vertically stacked groups with
  independent logs/control/recording; primary A retains the existing chart session.
  Preserve all preexisting uncommitted UART work and private INI. No Git publication
  or target firmware implementation is authorized by this feature request.
- Workbench product behavior/limits: docs/UART_WORKBENCH_GUIDE.md (1.0.1.025).
  Real child containers keep A/B logs separate. Preserve generation tags through
  raw traffic; binary/packet partials never cross reconnects. Raw recording remains
  independent of filters, inspection, protocol/numeric parsing and view clearing.
  Binary/RTU use PC receipt time and exclude conflicting text time fields/replay.
- A per-port TX lease excludes Console/keyboard/file/automation/tool interleaving.
  Worker completions for protocol traffic use shared state, never window captures
  or UI waits. Failed/partial writes stop without uncertain-byte retries; Stop and
  reconnect never arm/replay commands. Profiles lock owned preference windows too.
  Private .scccommands/.sccsteps definitions persist; execution/arming does not.
- X/YMODEM preflight source files and stage received output before atomic commit.
  Validate batch names, bounds, CRC/sequence/retries and actual disk outcomes;
  never overwrite batch names or guess XMODEM padding length. CAN is best-effort.
  This is generic PC transfer, not MCU flashing logic or hardware acceptance.
  RTU is read-only FC03/04, one outstanding request, stop on exception/timeout.
- Instruments use retained source clocks for FFT (Hann/mean removal); reject
  irregular timing unless an explicit device sample rate is selected. Workbench
  terminal is a receive-only monochrome ASCII VT100-style subset with no query TX;
  the existing Messages light-terminal remains separate. Do not claim full VT100.
- Approved native UART expansion is tracked in `docs/UART_DEBUG_PLAN.md`; exclude firmware/event-specific trigger recording. Deliver and verify incremental batches; never claim unchecked items are implemented. Python frontend is unchanged unless requested.
- Current native optimization contract is in `docs/UART_OPTIMIZATION_PLAN.md` and `docs/UART_OPTIMIZATION_GUIDE.md` (1.0.1.022); do not restart completed A/B1/B2/C. Seven advanced pages share a clipped scroll viewport below fixed tabs; tab-key focus must scroll into view. Use measured button widths, wrapping main toolbars and lifetime-owned replacement fonts for PMv2. `SimpleComChart.manifest` contributes dpiAwareness only; VS_DPI_AWARE supplies the one legacy dpiAware node. Never embed duplicate legacy DPI nodes. Enlarged fonts are not actual monitor-DPI verification.
- `session_instance` acquires stable session-local UARTChart.v1 ownership before UI/settings/workers across paths, copies and Debug/Release. Secondary launches wait bounded readiness, notify/restore existing main/modal UI and exit; errors fail closed. Only isolated noninteractive smoke copies under build/verify use path-private identities. Keep ownership through worker/window/GDI teardown; do not kill or reset the existing owner.
- Offline replay is background and paged: at most 4096 in-memory index records, then a delete-on-close temporary disk index with a 256 KiB mapped view; source reads stay on cancellable background jobs. Disk/input caps: 64 GiB / 50 million records / 4096 segments. Completed prefix_1..N.bin segments sort numerically; reject missing/duplicate numbers, mixed indexed/unindexed sources, malformed/incomplete adjacent .idx files. Sources are held read-only against writes/deletes until unloaded. Cross-segment byte search/time seek are background, pause on result and never transmit. Plain sources remain explicitly synthetic; receipt clocks are not wire clocks. Epoch-check reads across seek/unload; cancel/drain all file jobs before owner teardown. Keep virtual clock behavior, live isolation and safe no-source Unload.
- Generic `field_extractor` is layered after line filtering: compatible Key/value, exact CSV/Tab configured columns, flat numeric JSON and bounded Regex groups. Preview never applies; a validated saved Apply starts a new numeric capture but preserves Messages/recording. Extracted seconds use a capture-local origin; reject invalid/backwards time without altering raw receipt clocks. Parser/display preferences are optional schema-v1 profile extensions (old profiles default to Key/value/raw display); encoded profiles still cap at 64 KiB.
- DisplayPreferences project only rendered values; source/capture/export values and stable keys remain raw. Alias/unit/precision/scale/offset persist with bounded schema/UTF-8 validation; non-finite projection shows n/a. Both channel panels search keys/aliases and All/None affect visible results only, preserving hidden checkbox IDs. Analysis/Channels selectors filter editable search choices. Plot navigation must preserve actual drag capture and analysis snapshot statistics; clip fixed-Y waveforms to plot bounds and label the latest visible sample, not a future point.
- Exact source offset/count and sequence identify table/RX navigation; retain them across split reads, repeated contents, shared receipt timestamps and CR/LF boundaries. Never guess by substring. Expired/cleared/paused/terminal mappings report unavailable. UI parsing is capped at 64 lines / approximately 8 ms per dispatch; pending numeric input caps at 2000 lines / 4 MiB. Raw recording precedes UI/filtering. Avoid std::regex for numeric scalar parsing, full-series copies for latest values, or hidden-page rebuilds at RX rate; reuse change-scoped sustained receive evidence after unrelated edits.
- Light terminal is optional CR/BS/CSI K rendering only, no full VT claim, timestamps or local-TX fabrication. Keyboard TX is explicitly confirmed, generation-bound and never armed from INI/reconnect; every paste is bounded/preview-confirmed. Ctrl+C is byte 03, not Break. File TX validates complete UTF-8 text before first send, preserves binary/BOM/control bytes except explicit newline conversion, and uses background bounded reads, one queued chunk, pacing/progress/pause/cancel. Block overlapping Console/keyboard/automation, stop on partial/error/disconnect/close with no retry/resume; accepted bytes cannot be revoked. Never join unfinished file work on normal UI paths.
- Profiles are versioned fixed-schema .sccprofile files, max 64 KiB, in portable profiles/ for named entries. Exclude private profiles/raw recordings from Git. Import/load stages only; explicit confirmed Apply requires idle UART/no replay/recording/export, merges unrelated INI keys in a background staging file (1 MiB limit), disables main settings input through atomic commit, then restores actual saved preferences without opening hardware. A confirmed profile may explicitly replace saved serial preferences; this exception does not claim a successful connection or permit an INI hardware path. No commands/history/recording paths/automation auto-start in profiles. Cancellation does not undo a completed save.
- Chart analysis uses retained capture snapshots, sparse cells, inclusive intervals, nearest-sample A/B values, explicit capture-relative axes, reference lines and independent view-end/window navigation. Left/right mouse messages set A/B; Clear removes overlays only. Empty intervals are n/a. Preserve default live chart behavior; analysis-only axes/overlays must be opt-in. Dynamic page changes keep tab selection/focus consistent and use child/sibling paint clipping plus complete redraw; verify each expected control is actually visible, not merely that existing visible HWNDs fit bounds.
- Automation is explicit, finite, generation-bound and one-at-a-time: Periodic/Batch/Auto reply, literal raw RX patterns (same Text/HEX format), checksum-before-ending, 4096-byte total limit, 64 commands, 100..60000 ms interval/timeout, 1..10000 cycles, 0..3 explicitly selected timeout retries and one-hour cap. A raw-byte match/driver acceptance is not protocol ACK. Partial/write/queue failure stops with no retry; disconnect/close/Stop cancels queued automation and never resumes after reconnect. In-flight effects cannot be revoked. Feed raw RX on worker before filters; stop/manual serial-control/TX gating must stay responsive. Commands/arming are not persisted. Use fake transports only in automated tests.
- B1 UART operations use a separate owned/modeless Console (UART menu), not extra controls on the main tabs. Text is UTF-8; HEX accepts hex digits/whitespace without 0x. Validate the entire immutable payload, including explicit None/CR/LF/CRLF suffix, before queueing (1..4096 bytes). Only Send/Ctrl+Enter transmits; history selection/preset save/load never does. Keep 32 history entries and 8 presets in portable INI, encoded for quote/Unicode safety but not encrypted; never publish these private values. Record TX results with target/operation ID and distinguish driver acceptance from target ACK. Keep Messages/raw .bin strictly RX-only; readable recording separates TX/RX and byte totals.
- SerialControls has one persisted `Configuration=v1:flow:dtr:rts` value (flow 0=None, 1=RTS/CTS, 2=XON/XOFF, 3=DSR/DTR). Missing options use legacy None/DTR-on/RTS-on; malformed/truncated/unreadable settings block manual Connect until reviewed/applied. Idle Apply saves choices only; live Apply persists only successful current-generation completion. Same-bridge recovery retains the confirmed live settings, never command replay. Disable manual RTS/DTR when owned by handshake; report unsupported pin sampling as n/a. Live control changes/Break require an owned cancelable warning and post-modal connection-generation validation, since hardware may reset/change boot mode. Break is an explicit approximately 250 ms signal, not Ctrl+C.
- RX and operation workers share a stable owned transport with overlapped native Read/Write and distinct completion storage/events. Never hold the RX mutex during TX wait or Break sleep; GUI status/overflow reads must not acquire an I/O-held mutex. Queue up to 16 / 64 KiB, bind commands to connection generations, cancel queued work on disconnect and report late/partial outcomes without retry. Native cancellation requests do not prove rollback. Keep the transport/OVERLAPPED/event alive until completion; defer normal disconnect/close while either worker is stopping, join only OS-signaled finished workers on the UI path, and retain teardown-only owner-lifetime joins. Preserve final reported RX bytes even when completion also reports an error. See Microsoft API references in docs/MFC_BUILD.md; physical driver behavior is separate acceptance.
- Statistics are payload bytes / approximate trailing-one-second rates, accepted TX bytes/failures and driver error-bit observations (not exact physical error/event counts). Keep raw-history expiry and software pending loss separate. Manual new connection/reset clears statistics; reconnect preserves them. Reset never clears capture/recording. Show pin n/a when disconnected, not stale logical levels as measured voltages.
- Continuous RX recording is independent of Messages/capture filters, display pause/clear, retention and tab visibility. Feed it on the serial worker before UI queuing. One background writer, bounded 8 MiB/4096-item input, CREATE_NEW files, observable write/flush/overflow errors. On a recorder input gap stop visibly; never quietly resume or overwrite existing files. Save raw RX only in `.bin`, receipt offsets/times/events in `.idx`, escaped readable RX in `.log`. Rotation is between read chunks by size/date; do not imply a chunk is a UART protocol packet or timestamps are wire/device times. Stop drains queued input; normal close waits, and failed close-time recording leaves the window open with a warning. No real device access in automated fixtures.
- GUI continuous recording writes user-selected readable `.log` segments at the chosen destination. Keep `.bin`, `.idx` and `.sccmeta` together under the auto-created `log` folder beside the EXE/INI; recorder-core callers may retain the compatible single-base default. Never overwrite either destination.
- Messages adds UTF-8 and explicit ANSI code pages (system/950/936/1252), receipt clock/elapsed/delta, configurable CR/LF/CRLF, wrap, optional ANSI foreground SGR, and display pause under Options. Keep ASCII/HEX defaults and raw bytes unchanged; incomplete multibyte suffixes wait for more input, invalid bytes stay inspectable in HEX. ANSI is not full VT emulation. Use the plain EDIT by default and the system RichEdit only for wrap/SGR/highlighting; no packaged runtime dependency.
- Find is a separate collapsible toolbar (Ctrl+F, Enter/F3/Shift+F3), literal/case or bounded line-slice Regex, previous/next, optional match highlights and session bookmarks (Ctrl+F2/F2). Invalid/unsafe regex is visible. Search has a deliberately limited subset (no backreferences or quantifiers in/on groups, at most one variable quantifier per alternative, 2048-character slices, at most 4096 hits), not the Chart filter engine. Follow-off is set on explicit navigation. Bookmarks expire on history eviction/display reformat/reset; they never alter raw recording. Route shortcuts before MFC dialog-key processing and do not toggle an already-open Find bar closed with Ctrl+F.
- Advanced Messages display/encoding options persist in `[Messages]`; pause/search/bookmarks do not. Save As retains its snapshot semantics, formats current retained bytes using the selected options, and encodes Unicode views as UTF-8 without BOM; raw binary recording remains separate. A paused display does not freeze the export source.
- Discover channels from comma-delimited `key:value` or `key=value`; no firmware-specific key catalogue.
- Preserve finite signed double values, decimal/scientific numbers, hex, ASCII units and percent magnitude.
- Slash-separated values become `key[0]`, `key[1]`, etc. Skip malformed fields without fabricating zeros.
- Raw-line Include/Exclude filters run before parsing. Text is case-sensitive; Regex is the documented byte-oriented leftmost-longest bounded NFA subset, not full Python/ECMAScript. No backreferences/lookaround/lazy quantifiers; max 1024 states, repeat count 64, nesting 16, 8 capture groups, 65536 work units per online match and 64 KiB line. Limit hits are observable, skip whole lines and never truncate then match. Invalid Apply retains a valid active filter; invalid first-start settings wait for valid Apply rather than silently widening numeric capture.
- Preserve filter syntax: multiple alternatives require explicit Regex mode with `|`; comma/semicolon do not split patterns, and Text treats the whole input literally. Keep only the applied/error status as a measured/wrapping paragraph. Include/Exclude explanations are hover tooltips on both labels and fields, not permanent paragraphs. Exclusion wins even when Include matches; do not imply line filters select individual chart fields.
- Keep Include/Exclude tooltip hints brief: Set Regex, use `|`, and one example each. Put detailed syntax/priority explanations in Help/README. Invalid Apply keeps capture and a visible error status; do not hide validation errors in tooltips.
- Applying valid filters/limits starts a new capture; invalid input retains the active filter/data.
- Channel limit defaults to 256, configurable 1..1024. Keep buffers bounded.
- Clicking a Channel header toggles case-insensitive natural name order (P2 before P10), with visible A-Z/Z-A text in the header. Bind checkboxes, values and colors to stable channel IDs, never sorted row positions. Each panel keeps its own sort direction for the session/new channels/capture resets; sorting must not change waveform selection or data-table column mapping.
- MFC capture settings stay in `SimpleComChart.ini` beside the EXE.
- MFC tab order is Messages, Chart, 4 Charts. Settings > Startup tab explicitly selects/persists `[UI] StartupTab=Messages|Chart|4Charts`; missing/invalid values default to Chart. Apply the saved page before first show, with matching radio check/focus/visibility and page-specific controls. Choosing the startup option takes effect next launch and writes only that INI key; ordinary tab switching must not overwrite it or clear capture. Failed writes retain the previous confirmed option and show an error. Use named tab IDs in state/export/test logic, never assumed numeric positions. Selected tabs use dark-grey fill/white text with a dark underline; inactive tabs remain neutral grey. Preserve tab notifications, keyboard operation and page-specific control states.
- Keep data capture independent of the Chart page's selection, Refresh, Snapshot and time window.
- Capture retains up to 10000 accepted raw lines / 32 MiB payload; expose eviction count. Apply/connect reset capture; invalid Apply must not.
- Data rows are sparse (missing is blank, never zero); keep headers fixed and preserve the visible row when Auto-scroll is off, including retention eviction.
- Four small plots own their channel model, time window and automatic Y scale. Do not share selection/axes with the Chart page.
- All four plots default to `(none)` when no saved choices exist. Persist each exact selected field in `[FourCharts]` of the portable INI, including disabled slots. Restore names before discovery; wait for matching fields, never substitute arbitrary channels. Apply/connect clear samples but retain choices. Reset to none clears all four choices and saves immediately, without clearing the table/capture. Time windows remain per-session.
- Left-align table headers/cells; content-size columns from measured headers and a bounded recent-row sample (at most 64 rows / approximately 2048 cells). Do not stretch columns to fill unused width or scan the entire capture each update. Sparse blanks and capture-column mapping must remain correct.
- Messages displays exact ReadFile bytes independently of Include/Exclude and numeric parsing, without waiting for LF. Default ASCII shows printable 7-bit text/TAB/newlines, escaping other bytes as `\\xNN`; HEX shows 16-byte rows and offsets including CR/LF/NUL. Mode switches retain raw history. Retain at most 256 KiB per pending/UI buffer; report eviction and pending-input loss. Clear messages only resets raw history; valid Apply does not, successful connect does. Disconnect drains queued raw bytes. Auto-scroll defaults on; off preserves the inspected vertical position, clamping after eviction. Messages Save As writes the current ASCII/HEX formatted retained history verbatim to .log/.txt only, unfiltered and independent of numeric capture. Snapshot after filename confirmation; no BOM, extra header/trailing newline, binary or continuous recording. Use the shared background worker, atomic replacement and close/busy guards; save may work with zero chart rows. Later receive/mode/Clear/tab changes must not change an in-flight snapshot.
- Render both ASCII and HEX message content as green text on black. Handle the readonly EDIT's WM_CTLCOLORSTATIC as well as WM_CTLCOLOREDIT, returning a borrowed stock black brush. Keep toolbar/settings controls neutral.
- Messages Font -/+ changes only the message body in 1 pt steps (8..32 pt, default 10 pt), with a visible size readout and disabled buttons at limits. Share the size across ASCII/HEX, persist immediately to `[Messages] FontSizePt`, and restore during normal INI load. Clamp invalid/out-of-range settings. Create a replacement font before selecting it, retire only the page-owned previous font, preserve data/export text and the inspected row when possible; follow-on still tracks newest. Do not resize shared toolbar/chart fonts or clear history when changing size.
- Keep the Messages toolbar aligned to the combo's actual closed-field height: labels, checkbox, Clear and font controls share top/bottom/center, with single-line static text vertically centered. Retain dropdown-list space separately. Verify alignment after body-font changes and resize with normal/enlarged UI fonts, not just non-overlapping widths.
- Chart and data-page field checkboxes default to none for initial and later discoveries, including a new capture. Preserve explicit user choices for already-known fields. Data-page All / None control visible table columns only, independently of both chart selections. Keep explicit visible-to-capture column mapping, sparse blanks, bounded latest-value caches and scroll anchors. Hidden fields remain in capture/export. Saved four-small-chart choices still restore exactly; do not clear them to implement checkbox defaults.
- Both chart tabs show latest-point markers and live labels, including singleton series; omit stale labels outside the time window or for `(none)`. The shared default-on Show name checkbox switches name:value versus numeric-only labels, not label visibility. Preserve numeric text when shortening long names; Show name is per-session UI state.
- The Chart page automatically expands AND shrinks Y using enabled, filtered samples in its current time window. Fit uses the same calculation; Snapshot/Fit must use frozen samples, not incoming live values. Apply/Refresh/no visible data reset to the empty 0..50 fallback. Never retain a previous capture's axis bounds or accumulate overlay padding.
- Chart/4 Charts Save As defaults to plain .log; .txt is raw text, .csv/.cvs is escaped UTF-8 table, .xls is binary BIFF8 (254 numeric fields maximum). Both chart and Messages exports use immutable snapshots on a shared worker, preserve the destination on failure, and guard close while exporting. Save As is disabled while any export is busy, including after tab switches, and re-enabled on every tab after success/failure.
- Save As suggests the PC's local date/time to seconds, `yyyyMMdd_HHmmss.log`, at click time. Keep the editable filename, format choices and overwrite confirmation. Refresh/Fit/Snapshot/Overlay are functional Chart-page commands, not unused controls.
- UI/layout must guard uncreated HWNDs. Preserve decimal values in model, hover, labels and Y-axis.
- Keep control fonts alive until dialog teardown; never delete a shared font inside paint handlers. Runtime checks must verify live fonts and full caption bounds after repeated resize/repaint/tab switches, not only startup.
- Keep the toolbar neutral: system UI-grey faces, normal-weight text and straight borders. Refresh/Fit/Snapshot/Overlay use one temporary red accent only while pressed/executing, then return to grey regardless of toggle state. Connect is grey disconnected, red while executing/connected, grey again after disconnect/failure. Disabled controls override active/hover colors. Preserve command behavior, keyboard focus and lifetime-safe feedback cleanup; never retain owner pointers across destructive dispatch.
- Reserve Connect/Cancel/Disconnect's widest measured caption during normal layout and budget the serial row's COM width from actual fixed controls/gaps. Connection-state updates change caption/enabled/accent state only, never call whole-dialog layout or move unrelated children. Verify unchanged child geometry and zero unrelated window-position events across repeated opening/connected/waiting/idle states; settled PrintWindow images alone cannot prove absence of transient flicker.
- Provide native hover help for Refresh/Fit/Snapshot/Overlay, Auto Refresh label/combo, and Include/Exclude labels/fields. Explain that Auto Refresh periodically clears Chart samples, not the receive/update interval, and preserves capture/Messages. Register actual child HWND tools with TTF_IDISHWND / TTF_SUBCLASS; separately subclass parent-client fallback rectangles for transparent labels/disabled children and update those rectangles after layout. Do not rewrite child mouse messages as parent messages. Use a compatible TOOLINFO prefix, stable text storage, bounded wrapping/delays, no fade/animation and owner-time cleanup; no runtime dependency or manifest/theme overhaul. Registration/hit-test/forced-popup checks do not replace actual cursor-dwell acceptance.
- Serial I/O runs in a worker; synchronize handle/overflow state with the UI.
- Persist only the last successful native port/settings in `[Serial]` (Port, DeviceInstanceId, Baud, DataBits, Parity, StopBits). Restore selection at startup without opening hardware. Never accept an INI-supplied interface/file path; derive open paths only from current SetupAPI enumeration.
- Unexpected read error/PnP removal arms cancellable same-device recovery, preserving all capture/chart/raw history and selections. Match unique stable instance ID and enumerated COM interface, including COM renumbering; never fall back to a reused COM number or a different first port. Unknown/ambiguous identity/interface requires manual connection. Quiet RX and failed scans are not device loss. Manual Disconnect/Cancel stops retries; a new successful manual Connect retains its existing fresh-capture behavior.
- Keep driver open on one independent worker with no dialog pointer. Cancellation/teardown must discard late successes and close their transport; do not accumulate workers while an open is pending. Retry backoff is 2/4/8/10 seconds maximum, with 1-second hotplug scans, non-modal status and rate-limited repeated-error logging. Waiting continues until original device returns or the user cancels; this opt-in recovery is not protocol-command replay. Safe automated tests use injected fake transport/enumeration and fixture INIs, never a real COM open.
- A failed user-triggered Connect must show one owned warning MessageBox, not just bottom status: include the requested PORT, original error/code and actionable checks (disconnect Tera Term/PuTTY/duplicate tool, verify USB/Device Manager/serial settings, retry). Say the port may be busy/unavailable, never claim a specific process owns it or close other apps. Restore idle controls/release rejected handles before the prompt; retain the last successful INI/capture on failure. Automatic retries/cancelled late results never show this modal warning. Warning dispatch may run a nested modal loop; guard owner teardown on return. The serial fixture intercepts dispatch to assert content/count/owner/flags without real hardware or unattended modal dialogs.

## Verification And Delivery

- Analysis expansion: preserve first/min/max/last per pixel without altering raw
  samples; keep B text filters/extraction/generation independent and qualify B
  numeric/derived outputs. Shared charts use compatible PC receipt clocks. No
  stale values, silent source collisions or automatic channel selection.
- Offline binary parsing resets partials on seek/recorded generation changes;
  workspace v2 accepts only validated allowlisted settings, keeps v1 readable and
  never imports private paths/commands/arming. Derived expressions are bounded,
  finite and same-frame; per-source EMA resets with generation/capture.
- Binary inspection and bounded background run comparison are read-only. XY uses
  same-row pairs; comparison uses explicit alignment and one-to-one nearest pairs,
  never interpolation. Numeric script assertions retain TX/generation ownership.
  Forwarding requires explicit confirmation, both TX leases, finite limits and
  stop-on-error/partial/gap/possible-echo; no auto start/retry/reconnect resume.
- Analysis integration: `scripts\verify_runtime.ps1 -Scope AnalysisExpansion`
  covers actual native controls, A/B fake RX/TX, binary replay, workspace apply,
  formulas, inspector, XY/comparison and worker shutdown. Add affected Charts /
  Workbench / Advanced and selected core targets for shared paths; no All/Python.
- Apply workspace `Test Artifact Ownership And Retention` and SELF_CHECK TST-01.
  Generated bulk fixtures belong to a fresh isolated run and must use checked
  exact-file RAII/finally cleanup, including failure/cancel. Keep compact results
  and recipes; never delete user UART logs, delivery EXE/INI or unknown files. Normal clean
  remains compiler-file-only. `uart_optimization_tests` retains only its small
  recipe after success; the 74,240,000-byte replay input must not accumulate.
- `verify_runtime.ps1` uses a fresh owned `build/verify/runs/<case>-<GUID>` and
  `scripts/test_artifacts.ps1`. After the owned process exits, preserve compact
  results/configuration/screenshots and hashes under
  `build/verify/evidence/<case>/last-pass` or `last-failure`, then delete exact
  registered scratch files, including copied EXEs and synthetic recordings.
  Reuse canonical runtime-bin EXEs during active verification, not one per scope.
  After verification and its recovery/publication checkpoint, retire proven-owned
  generated binaries/caches; retaining one needs a size, reason and cleanup point.
  Unknown/modified evidence and reparse escapes must stop overwrite/deletion.
- `-SkipBuild` is only for an unchanged, already verified native application
  with its verification binary still present (for example, testing runner cleanup);
  it does not establish a fresh build. Without that artifact, omit `-SkipBuild`.
  Select `tests/test_artifacts_tests.ps1` for retention changes, and the focused
  session_instance / telemetry_core targets when their helper/export lifecycle
  changes. Their fresh fixtures remove generated EXE/XLS/log outputs via RAII.
- Final cleanup also reviews CMake-owned test/benchmark EXEs and obsolete linker
  maps under build/obj against actual target/cache metadata. Remove these exact
  reproducible outputs after tests; do not keep every configuration/cache copy.
  This is separate from ordinary clean's delivery-EXE protection. Keep the formal
  EXE/INI and compact evidence. Maps, caches and runtime-bin copies are rebuildable,
  not permanent requirements. Once source/ownership and recovery are verified,
  generated obj/verify/Debug/legacy_native_mfc contents may be retired through the
  scoped lifecycle procedure; never infer permission from those names alone.
  Retire a temporary recovery archive when its recorded source-publication /
  recovery checkpoint has been independently confirmed.

- Apply workspace `Change-Impact Verification`: select only changed/affected checks from the entries below, not every command on each edit. Reuse unchanged evidence with attribution; document reasons for broader coverage. Documentation-only edits do not rebuild or rerun the application. For MFC-only changes, do not run unrelated Python suites; use enlarged-font checks only when font/layout geometry is affected.

- Build: `scripts\build_release.bat` (Release x64 plus native tests).
- Debug: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug`.
- Python core: `python -B -m unittest discover -s tests -v`.
- Native startup: `scripts\verify_runtime.ps1 -Configuration Release` and `Debug`.
- Connection-only regression: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Serial` (Release also supported, optionally `-Font125`). Runs only connection geometry/state and fake serial persistence/recovery/cancel, under isolated `build/verify/<configuration>[-font125]-serial`; no real COM open. Use it for connection UI changes instead of the full runtime suite.
- Startup-tab regression: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Startup -StartupTab Messages` (also Chart/4Charts, Release). Seeds only isolated fixture INIs and verifies actual process startup plus menu/INI/fallback/failed-write/page-routing checks; no hardware or unrelated suites. Do not run Serial/All/Python checks for a startup-option-only edit.
- UART receive batch: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Uart` (Release / Font125 supported). Selects new display/recording integration plus affected existing Messages/export/font/fake-serial regressions only. Native `uart_receive_tests` covers formatter/receipt/search/recorder files/rotation/errors. Build entry bundles existing telemetry_core; do not append All/Python or unrelated chart suites. All fixture writes are under build/verify; personal INIs must remain untouched.
- UART operations: `scripts\verify_runtime.ps1 -Configuration Debug -Scope UartOperations` (Release / Font125 supported), plus native `uart_operations` core target. Covers actual Console controls and fake transport TX/control/partial/cancel/concurrent RX/late data; local named-pipe tests exercise the native overlapped helper without opening COM hardware. Shared I/O/stop changes also select affected Serial/Uart receive regressions; small later Console-only changes do not require rerunning them. No real TX/side-band action in safe automated tests.
- Advanced tools: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Advanced` (Release / Font125 supported) and native `uart_advanced` target. Exercise replay files/timing/seek/clock source, profile schema/atomic preservation/cancel/read-only, snapshot statistics/checksums/finite scheduler, actual controls/tab visibility/geometry/cursor messages and fake-worker automation. Select Serial/Uart/UartOperations only when shared receive/config/queue paths change; do not append All/Python. Font125 is enlarged fonts, not physical DPI or physical-mouse acceptance.
- Optimization: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Optimization` (Release / Font125 / Font150 supported), native `uart_optimization` and `session_instance` targets. Covers generic extraction, display-only preferences, source correlation, actual chart/file-TX/keyboard controls, partial/cancel/generation isolation and seven small-screen pages. `-Scope Soak -SoakSeconds 60..86400` is separately selected independent synthetic RX/recording/UI measurement (default 60 seconds; duration + 120-second process deadline). Run it when receive/render/retention performance is affected, not after every later UI/document edit. Private helper crash/timeout tests must never target production processes/data. Physical UART/DPI-switch/foreground/elevation remain separate acceptance.
- Affected chart source/axis/selection regression only: `scripts\verify_runtime.ps1 -Configuration Debug -Scope Charts`, covering Y autoscale/frozen values, independent data-page plots/retention, default-none checkboxes and saved four-chart choices; use for affected chart changes, not every isolated advanced-tools edit.
- When font/layout geometry is affected, use the relevant configuration(s) with `-Font125` for enlarged-font layout regression (not a real monitor-DPI switch) and inspect the affected page PNGs; this is not an unconditional four-variant test requirement.
- `--demo` is not supported. `--smoke-test` is available only in the separate verification build. `scripts/verify_delivery.ps1 -Configuration Release|Debug` checks the actual shipped binary (hash-matched copies with fresh private INIs), all three startup pages remaining empty without input, normal close and rejection of both removed flags; it refuses to interfere with an existing normal owner. Verify delivery independently of the check-enabled fixture after build-mode changes. Real-data functionality remains covered by selected check-enabled/core fixtures, not by injecting fake data into production.
- Real COM timing/throughput and visual DPI checks require explicit evidence; never imply them from core tests.
- Clean entries: `scripts\clean_mfc.bat` and compatible `scripts\clean_build.bat`; preflight Python/cache and native targets before deletion. A matching project-owned CMake cache authorizes only recognized generated compiler/project/cache/map/core-test files; fixed-name non-Release and check-only runtime-bin copies are also disposable after process checks. Never recursively purge obj/tests/verify folders. Preserve formal root EXE/CLI/catalogs/INI, data/logs, compact verify/evidence and unknown files even inside generated directories. Keep `clean_build.ps1 -WhatIf` non-mutating.
- Codex runs the workspace clean audit after clean/build flow changes; see `..\#READ_ME_FIRST#\AGENTS.md`.
- Keep README, SELF_CHECK and HANDOFF aligned with actual behavior and verification.
