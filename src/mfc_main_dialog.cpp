#include "mfc_main_dialog.h"
#include "build_version.h"
#include "button_feedback.h"
#include "dpi_support.h"
#include "session_instance.h"
#include "uart_workbench.h"

#pragma comment(lib, "comctl32.lib")

static constexpr int HOTPLUG_SCAN_MS = 1000;
static constexpr int UI_UPDATE_MS = 50;

static constexpr int IDC_COMBO_PORT = 101;
static constexpr int IDC_BTN_SCAN = 102;
static constexpr int IDC_BTN_CONNECT = 103;
static constexpr int IDC_COMBO_BAUD = 104;
static constexpr int IDC_COMBO_DATA = 105;
static constexpr int IDC_COMBO_PARITY = 106;
static constexpr int IDC_COMBO_STOP = 107;

static constexpr int IDC_BTN_REFRESH = 201;
static constexpr int IDC_BTN_FIT = 202;
static constexpr int IDC_BTN_SNAPSHOT = 203;
static constexpr int IDC_BTN_OVERLAY = 204;
static constexpr int IDC_COMBO_AUTO = 205;
static constexpr int IDC_COMBO_TIME = 206;

static constexpr int IDC_CHANNEL_PANEL = 300;
static constexpr int IDC_PLOT_VIEW = 301;
static constexpr int IDC_STATUS = 400;


static COLORREF kColorTable[] = {
    RGB(255,  99,  71),
    RGB( 30, 144, 255),
    RGB( 50, 205,  50),
    RGB(255,  20, 147),
    RGB(138,  43, 226),
    RGB(255, 140,   0),
    RGB(  0, 206, 209),
    RGB(220,  20,  60),
};

static double live_seconds() {
    static LARGE_INTEGER freq = []{
        LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) / static_cast<double>(freq.QuadPart);
}

double CMainDialog::now_seconds() const {
    if(extracted_clock_) return std::max(extracted_now_+1,model_.newest_timestamp());
    const auto data=replay_.data();
    if (data && !data->records.empty()) return std::max(data->records[replay_.position()?replay_.position()-1:0].seconds+1,model_.newest_timestamp());
    return std::max(live_seconds(),model_.newest_timestamp());
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialog)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_DRAWITEM()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_GETMINMAXINFO()
    ON_BN_CLICKED(IDC_BTN_SNAPSHOT, &CMainDialog::OnSnapshotClicked)
    ON_BN_CLICKED(IDC_BTN_OVERLAY, &CMainDialog::OnOverlayClicked)
    ON_MESSAGE(WM_APP + 1, &CMainDialog::OnChannelChanged)
    ON_MESSAGE(WM_APP + 2, &CMainDialog::OnExportComplete)
    ON_MESSAGE(DataPage::kSelectionChanged, &CMainDialog::OnChartSelectionChanged)
    ON_MESSAGE(MessagePage::kFontChanged, &CMainDialog::OnMessageFontChanged)
    ON_MESSAGE(UartConsole::kRequest, &CMainDialog::OnUartRequest)
    ON_MESSAGE(AdvancedTools::kRequest, &CMainDialog::OnAdvancedRequest)
    ON_MESSAGE(WM_DPICHANGED, &CMainDialog::OnDpiChanged)
    ON_MESSAGE(DataPage::kNavigate, &CMainDialog::OnSourceNavigate)
    ON_MESSAGE(MessagePage::kLocateSource, &CMainDialog::OnLocateSource)
    ON_MESSAGE(PlotView::kCursorMoved, &CMainDialog::OnPlotCursor)
    ON_MESSAGE(MessagePage::kKeyboardTx, &CMainDialog::OnKeyboardTx)
    ON_MESSAGE(MessagePage::kKeyboardArm, &CMainDialog::OnKeyboardArm)
    ON_MESSAGE(UartWorkbench::kRequest, &CMainDialog::OnWorkbenchRequest)
    ON_COMMAND(ID_HELP_LOGFORMAT, &CMainDialog::OnHelpLogFormat)
END_MESSAGE_MAP()

CMainDialog::CMainDialog(CWnd* pParent)
    : CDialog(IDD_MAIN_DIALOG, pParent),primary_traffic_(std::make_shared<TrafficStore>()) {
}
CMainDialog::~CMainDialog() = default;

BOOL CMainDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    build_ui();
    build_filter_ui();
    ui_font_ = make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    const auto font = ui_font_ ? ui_font_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child; child = ::GetWindow(child, GW_HWNDNEXT)) {
        if (child != btn_connect_ && child != btn_refresh_ && child != btn_fit_ &&
            child != btn_snapshot_ && child != btn_overlay_)
            ::SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
    data_page_.create(m_hWnd, &capture_, font);
    channel_panel_.set_ui_font(font);
    message_page_.create(m_hWnd, font);
    build_hover_help(font);
    load_settings();
    workbench_=std::make_unique<UartWorkbench>();workbench_->primary_queue(&uart_operations_);workbench_->initialize(settings_path_,primary_traffic_);workbench_feed_enabled_=workbench_->binary_enabled(0);uart_operations_.pacing(workbench_->pacing(0));
    {const auto& c=extractor_.config();if(workbench_->any_binary()&&(!c.time_field.empty()||(c.mode==ExtractionConfig::Regex&&c.time_group>0))){workbench_->suspend_binary();show_status_message(L"Saved binary parser conflicts with text Time field; review Workbench settings.",9000);}}
    workbench_numeric_owned_=workbench_->binary_enabled(0);load_startup_tab();
    ui_ready_ = true;
    save_settings();
    SetTimer(IDT_HOTPLUG, HOTPLUG_SCAN_MS, nullptr);
    SetTimer(IDT_UI, UI_UPDATE_MS, nullptr);
    scan_ports();

    CMenu menu;
    if (menu.LoadMenuW(IDR_MAINMENU)) {
        SetMenu(&menu);
        menu.Detach();
    }

    HICON icon = LoadIconW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDI_APPICON));
    if (icon) {
        SetIcon(icon, TRUE);
        SetIcon(icon, FALSE);
    }

    apply_startup_tab(); // Set visibility before the first show, not after briefly displaying Chart.
    ShowWindow(SW_MAXIMIZE);
    CRect client;
    GetClientRect(&client);
    layout_ui(client.Width(), client.Height());
#ifdef CHART_RUNTIME_CHECKS
    smoke_test_ = std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--smoke-test") != std::wstring::npos;
    demo_ = smoke_test_;
    if (demo_) set_left_status(L"Test telemetry (isolated verification only)");
#endif
    CString title;
    title.Format(L"Simple COM Chart %s | %hs %hs", CHART_VERSION_W, __DATE__, __TIME__);
    SetWindowTextW(title);
    publish_main_instance(m_hWnd);

    return TRUE;
}

void CMainDialog::OnOK() {
    // prevent default Enter-to-close
}

BOOL CMainDialog::PreTranslateMessage(MSG* message) {
    if (message && message_page_.translate_key(*message)) return TRUE;
    return CDialog::PreTranslateMessage(message);
}

void CMainDialog::OnCancel() {
    if(workbench_){workbench_->shutdown();if(!workbench_->shutdown_complete()){close_after_workbench_=true;return;}const auto error=workbench_->close_recording_error();if(!error.empty()){workbench_->cancel_shutdown();uart_notice_(m_hWnd,error.c_str(),L"Port B recording",MB_OK|MB_ICONWARNING);return;}}
    stop_automation(L"Stopped: application closing");cancel_file_tx(L"File TX stopped: application closing."); replay_.pause(GetTickCount64());
    if (replay_load_.busy() || profile_job_.busy() || replay_io_.busy() || file_tx_job_.busy()) {
        close_after_tools_ = true; replay_load_.cancel(); profile_job_.cancel(); replay_io_.cancel();file_tx_job_.cancel();
        show_status_message(L"Cancelling background file work before closing...", 5000); return;
    }
    if (exporting_) {
        if(workbench_)workbench_->cancel_shutdown();
        show_status_message(L"Export in progress. Close again after Save As completes.", 5000);
        return;
    }
    if (!stop_serial_thread()) {
        close_after_serial_ = true; disconnect_pending_ = true; disconnect_unexpected_ = false;
        update_connection_ui(); show_status_message(L"Cancelling UART I/O before closing...", 5000); return;
    }
    if (recorder_.status().active) {
        close_after_recording_ = true; recorder_.stop();
        show_status_message(L"Finishing recording before closing...", 5000); return;
    }
    EndDialog(IDCANCEL);
}


void CMainDialog::OnSize(UINT nType, int cx, int cy) {
    CDialog::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED) {
        is_minimized_ = true;
        return;
    }

    if (is_minimized_) {
        is_minimized_ = false;
        flush_pending_lines();
        if (!snapshot_) {
            plot_view_.update_from_model(now_seconds());
        } else {
            ::InvalidateRect(plot_view_.hwnd(), nullptr, FALSE);
        }
    }

    if (ui_ready_) {
        layout_ui(cx, cy);
    }
}


void CMainDialog::flush_pending_lines() {
    if (serial_error_pending_) {
        serial_error_pending_ = false;
        std::wstring err;
        {
            std::lock_guard<std::mutex> lock(error_mutex_);
            err = serial_error_;
            serial_error_.clear();
        }
        disconnect(true);
        if (!err.empty()) {
            log_line(L"Serial error: " + err);
        } else {
            log_line(L"Serial disconnected");
        }
        return;
    }

    PendingData pending;
    int dropped_lines = 0;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending = std::move(pending_);
        pending_ = PendingData{};
        dropped_lines = pending_dropped_;
        pending_dropped_ = 0;
    }

    message_page_.append_history(pending.raw);
    if(!pending.raw.bytes().empty()) message_page_.set_source_end(pending.source_end);
    message_page_.refresh();
    pending_line_loss_+=static_cast<uint64_t>(std::max(0,dropped_lines));
    if(workbench_&&(workbench_->binary_enabled(0)||workbench_->numeric_suspended(0)))return;
    if (pending.lines.empty()) {
        return;
    }

    double now = 0.0;
    const auto batch_start=GetTickCount64();
    for (size_t index=0;index<pending.lines.size();++index) {
        if(index && (index>=64 || GetTickCount64()-batch_start>=8)) {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_.lines.insert(pending_.lines.begin(),std::make_move_iterator(pending.lines.begin()+index),std::make_move_iterator(pending.lines.end()));
            bound_pending_lines(pending_,pending_dropped_);break;
        }
        const auto& item=pending.lines[index];
        if(!filter_configured_) continue;
        if (!line_filter_.matches(item.line)) continue;
        const auto extracted=extractor_.extract(item.line);
        if(extracted.limited || !extracted.error.empty()) {++parser_skipped_;continue;}
        const auto& kv=extracted.values;
        double ts = item.ts > 0.0 ? item.ts : now_seconds();
        if(extracted.seconds) {
            if(!extracted_clock_) {extracted_origin_=*extracted.seconds;extracted_base_=ts;extracted_now_=ts;extracted_clock_=true;}
            ts=extracted_base_+(*extracted.seconds-extracted_origin_);
            if(!std::isfinite(ts) || ts<extracted_now_) {++parser_skipped_;continue;} extracted_now_=ts;
        }
        accept_numeric(item.line,kv,ts,item.source_offset,item.source_count);
        now = std::max(now, ts);
        if (!kv.empty()) {
            if (ts > now) {
                now = ts;
            }
        }
    }

    if (now <= 0.0) {
        now = now_seconds();
    }

    model_.prune(now);
    sync_channels();
    if(::IsWindowVisible(data_page_.hwnd())) data_page_.refresh(now);

    if (!snapshot_ && ::IsWindowVisible(plot_view_.hwnd())) {
        plot_view_.update_from_model(now);
    }

    std::unordered_map<std::string, double> latest;
    for (const auto& key : model_.get_keys()) {
        double value=0;if(model_.latest_value(key,value)) latest[key]=value;
    }
    channel_panel_.update_values(latest);

    set_right_status(L"Samples: " + std::to_wstring(model_.get_total_samples()) + L" | CH: " + std::to_wstring(model_.get_enabled_count()));

    int dropped_keys = model_.consume_dropped_keys();
    if (dropped_keys > 0) {
        show_status_message(L"Channel limit reached (" + std::to_wstring(model_.get_max_channels()) + L"), increase Max CH and Apply", 5000);
        log_line(L"Channel limit reached, ignored new keys");
    }

    if (dropped_lines > 0) {
        show_status_message(L"Input overrun: dropped lines", 3000);
        log_line(L"Input overrun: dropped lines");
    }

    int rx_overflow = serial_mgr_->consume_rx_overflow();
    if (rx_overflow > 0) {
        show_status_message(L"Input overflow: dropped bytes", 3000);
        log_line(L"Input overflow: dropped bytes");
    }
}

void CMainDialog::sync_channels() {
    auto keys = model_.get_keys();
    for (size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        COLORREF color = kColorTable[i % (sizeof(kColorTable) / sizeof(kColorTable[0]))];
        channel_panel_.ensure_channel(key, false, color);
    }
    channel_panel_.update_count(static_cast<int>(keys.size()));

    auto state_map = channel_panel_.get_checkbox_state_map();
    for (const auto& kv : state_map) {
        model_.set_enabled(kv.first, kv.second);
    }
}

void CMainDialog::set_left_status(const std::wstring& text) {
    left_status_ = text;
    if (!flash_active_) {
        ::SendMessageW(status_, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
}

void CMainDialog::set_right_status(const std::wstring& text) {
    const auto recording = recorder_.status();
    right_status_ = recording.active ? L"REC " + std::to_wstring(recording.bytes) + L" bytes | " + text : text;
    ::SendMessageW(status_, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(right_status_.c_str()));
}

void CMainDialog::show_status_message(const std::wstring& text, int ms) {
    left_flash_ = text;
    flash_active_ = true;
    ::SendMessageW(status_, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(left_flash_.c_str()));
    KillTimer(IDT_STATUS);
    SetTimer(IDT_STATUS, ms, nullptr);
}

void CMainDialog::draw_owner_button(const DRAWITEMSTRUCT* dis) {
    if (!dis) {
        return;
    }

    const bool pressed = (dis->itemState & ODS_SELECTED) || ::GetPropW(dis->hwndItem, kButtonBusyProperty);
    const auto colors = button_colors(pressed, dis->CtlID == IDC_BTN_CONNECT && (serial_mgr_->is_connected() || connection_.opening()),
        (dis->itemState & ODS_DISABLED) != 0, (dis->itemState & ODS_HOTLIGHT) || dis->hwndItem == hover_btn_);
    HBRUSH brush = CreateSolidBrush(colors.background);
    FillRect(dis->hDC, &dis->rcItem, brush); DeleteObject(brush);
    FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_BTNSHADOW));

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, colors.text);

    HFONT old_font = nullptr;
    if (btn_font_) {
        old_font = static_cast<HFONT>(SelectObject(dis->hDC, btn_font_));
    }
    wchar_t text[128] = {};
    ::GetWindowTextW(dis->hwndItem, text, 127);
    DrawTextW(dis->hDC, text, -1, const_cast<LPRECT>(&dis->rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (old_font) {
        SelectObject(dis->hDC, old_font);
    }
    if (dis->itemState & ODS_FOCUS) {
        RECT focus = dis->rcItem; InflateRect(&focus, -3, -3); DrawFocusRect(dis->hDC, &focus);
    }
}

void CMainDialog::update_hover_button(POINT pt, bool leave) {
    HWND new_hover = nullptr;
    if (!leave) {
        HWND child = ChildWindowFromPointEx(m_hWnd, pt, CWP_SKIPINVISIBLE);
        if (child == btn_connect_ || child == btn_refresh_ || child == btn_fit_ ||
            child == btn_snapshot_ || child == btn_overlay_) {
            new_hover = child;
        }
    }

    if (new_hover != hover_btn_) {
        if (hover_btn_) {
            ::InvalidateRect(hover_btn_, nullptr, TRUE);
        }
        hover_btn_ = new_hover;
        if (hover_btn_) {
            ::InvalidateRect(hover_btn_, nullptr, TRUE);
        }
    }

    if (!tracking_mouse_ && !leave) {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        TrackMouseEvent(&tme);
        tracking_mouse_ = true;
    }
    if (leave) {
        tracking_mouse_ = false;
    }
}

void CMainDialog::OnTimer(UINT_PTR nIDEvent) {
    // Capture/logging must continue when the window is minimized.
    if (nIDEvent == IDT_HOTPLUG) {
        poll_serial_ports();
    } else if (nIDEvent == IDT_UI) {
        poll_serial_stop(); poll_uart_operations();
        poll_workbench();
        if (!::IsWindow(m_hWnd)) return;
        poll_advanced_tools();
        if (!::IsWindow(m_hWnd)) return;
        poll_recording();
        if (!::IsWindow(m_hWnd)) return;
        poll_serial_connection();
        if (closing_serial_ || !::IsWindow(m_hWnd)) return; // A modal connection warning may close its owner.
#ifdef CHART_RUNTIME_CHECKS
        if (demo_ && !serial_mgr_->is_connected() && !replay_.data() && !replay_load_.busy()) feed_demo();
#endif
        flush_pending_lines();
        update_source_status();
#ifdef CHART_RUNTIME_CHECKS
        if (smoke_test_ && demo_tick_ >= 24) run_smoke_test();
#endif
    } else if (nIDEvent == IDT_AUTO) {
        ::SendMessageW(m_hWnd, WM_COMMAND, IDC_BTN_REFRESH, 0);
    } else if (nIDEvent == IDT_STATUS) {
        flash_active_ = false;
        KillTimer(IDT_STATUS);
        ::SendMessageW(status_, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(left_status_.c_str()));
    }
    CDialog::OnTimer(nIDEvent);
}

BOOL CMainDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
    if (profile_job_.busy() && profile_job_action_==4) return TRUE;
    const auto command = LOWORD(wParam);
    const bool feedback = HIWORD(wParam) == BN_CLICKED && (command == IDC_BTN_CONNECT ||
        (command >= IDC_BTN_REFRESH && command <= IDC_BTN_OVERLAY));
    ButtonFeedback executing(feedback ? ::GetDlgItem(m_hWnd, command) : nullptr);
    if (LOWORD(wParam) == IDC_CHECK_SHOW_NAME && HIWORD(wParam) == BN_CLICKED) {
        const bool enabled = ::SendMessageW(check_show_name_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        plot_view_.set_show_names(enabled); data_page_.set_show_names(enabled); return TRUE;
    }
    switch (LOWORD(wParam)) {
    case ID_UART_REPLAY: show_advanced_tools(AdvancedTools::Replay); return TRUE;
    case ID_UART_BINARY_REPLAY: binary_replay_options(); return TRUE;
    case ID_UART_DERIVED: derived_options(); return TRUE;
    case ID_UART_PROFILES: show_advanced_tools(AdvancedTools::Profiles); return TRUE;
    case ID_UART_ANALYSIS: show_advanced_tools(AdvancedTools::Analysis); return TRUE;
    case ID_UART_AUTOMATION: show_advanced_tools(AdvancedTools::Automation); return TRUE;
    case ID_UART_STOP_AUTOMATION: stop_automation(L"Stopped by user"); return TRUE;
    case ID_UART_EXTRACTION: show_advanced_tools(AdvancedTools::Extraction); return TRUE;
    case ID_UART_CHANNELS: show_advanced_tools(AdvancedTools::Channels); return TRUE;
    case ID_UART_FILE_TX: show_advanced_tools(AdvancedTools::FileTx); return TRUE;
    case ID_UART_CONSOLE: show_uart_console(); return TRUE;
    case ID_UART_WORKBENCH: show_workbench(); return TRUE;
    case ID_UART_RECORD_START: start_recording(); return TRUE;
    case ID_UART_RECORD_STOP: recorder_.stop(); return TRUE;
    case ID_UART_RECORD_OPTIONS: recording_options(); return TRUE;
    case ID_STARTUP_MESSAGES: choose_startup_tab(kMessagesTab); return TRUE;
    case ID_STARTUP_CHART: choose_startup_tab(kChartTab); return TRUE;
    case ID_STARTUP_FOUR_CHARTS: choose_startup_tab(kDataTab); return TRUE;
    case 207:
        if (HIWORD(wParam) == BN_CLICKED) save_capture_as();
        return TRUE;
    case 505:
        if (HIWORD(wParam) == BN_CLICKED) apply_filter();
        return TRUE;
    case ID_HELP_LOGFORMAT:
        OnHelpLogFormat();
        return TRUE;
    case IDC_BTN_SCAN:
        if (HIWORD(wParam) != BN_CLICKED) {
            return TRUE;
        }
        scan_ports();
        return TRUE;
    case IDC_BTN_CONNECT:
        if (HIWORD(wParam) != BN_CLICKED) {
            return TRUE;
        }
        on_connect_toggle();
        return TRUE;
    case IDC_BTN_REFRESH: {
        if (HIWORD(wParam) != BN_CLICKED) {
            return TRUE;
        }
        model_.reset_samples();
        plot_view_.reset_visual();
        if (!snapshot_) {
            plot_view_.update_from_model(now_seconds());
        }
        channel_panel_.update_values({});
        set_right_status(L"Samples: 0 | CH: " + std::to_wstring(model_.get_enabled_count()));
        return TRUE;
    }
    case IDC_BTN_FIT:
        if (HIWORD(wParam) != BN_CLICKED) {
            return TRUE;
        }
        plot_view_.fit_enabled_channels();
        return TRUE;
    case IDC_COMBO_AUTO:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = static_cast<int>(::SendMessageW(combo_auto_, CB_GETCURSEL, 0, 0));
            if (sel == 0) {
                KillTimer(IDT_AUTO);
            } else {
                wchar_t buf[8] = {};
                ::SendMessageW(combo_auto_, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buf));
                int sec = _wtoi(buf);
                if (sec > 0) {
                    SetTimer(IDT_AUTO, sec * 1000, nullptr);
                }
            }
        }
        return TRUE;
    case IDC_COMBO_TIME:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            wchar_t buf[8] = {};
            int sel = static_cast<int>(::SendMessageW(combo_time_, CB_GETCURSEL, 0, 0));
            ::SendMessageW(combo_time_, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buf));
            double sec = _wtof(buf);
            model_.set_time_window(sec);
            if (!snapshot_) {
                plot_view_.set_time_window(sec);
                model_.prune(now_seconds());
                plot_view_.update_from_model(now_seconds());
            }
        }
        return TRUE;
    default:
        break;
    }
    return CDialog::OnCommand(wParam, lParam);
}

void CMainDialog::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    if (lpDrawItemStruct && lpDrawItemStruct->CtlType == ODT_TAB && lpDrawItemStruct->hwndItem == tabs_) {
        draw_tab(lpDrawItemStruct); return;
    }
    draw_owner_button(lpDrawItemStruct);
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMainDialog::OnMouseMove(UINT nFlags, CPoint point) {
    POINT pt = { point.x, point.y };
    update_hover_button(pt, false);
    CDialog::OnMouseMove(nFlags, point);
}

void CMainDialog::OnMouseLeave() {
    update_hover_button(POINT{}, true);
    CDialog::OnMouseLeave();
}

LRESULT CMainDialog::OnChannelChanged(WPARAM, LPARAM) {
    sync_channels();
    if (!snapshot_) {
        plot_view_.update_from_model(now_seconds());
    }
    return 0;
}

void CMainDialog::OnHelpLogFormat() {
    help_dialog_.show(m_hWnd);
}

void CMainDialog::OnSnapshotClicked() {
    bool checked = ::SendMessageW(btn_snapshot_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (checked == snapshot_) {
        snapshot_ = !snapshot_;
        ::SendMessageW(btn_snapshot_, BM_SETCHECK, snapshot_ ? BST_CHECKED : BST_UNCHECKED, 0);
    } else {
        snapshot_ = checked;
    }
    plot_view_.set_frozen(snapshot_);
    if (snapshot_) {
        ::SetWindowTextW(btn_snapshot_, L"Live");
    } else {
        ::SetWindowTextW(btn_snapshot_, L"Snapshot");
        plot_view_.update_from_model(now_seconds());
    }
    ::InvalidateRect(btn_snapshot_, nullptr, TRUE);
}

void CMainDialog::OnOverlayClicked() {
    bool checked = ::SendMessageW(btn_overlay_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (checked == overlay_enabled_) {
        overlay_enabled_ = !overlay_enabled_;
        ::SendMessageW(btn_overlay_, BM_SETCHECK, overlay_enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
    } else {
        overlay_enabled_ = checked;
    }

    plot_view_.set_overlay_enabled(overlay_enabled_);
    if (overlay_enabled_ && !snapshot_) {
        plot_view_.update_from_model(now_seconds());
    } else {
        ::InvalidateRect(plot_view_.hwnd(), nullptr, FALSE);
    }
    ::InvalidateRect(btn_overlay_, nullptr, TRUE);
}

void CMainDialog::OnDestroy() {
    closing_serial_ = true; serial_open_.cancel();
    stop_automation(L"Stopped: owner teardown");cancel_file_tx(L"File TX stopped: teardown."); replay_load_.cancel(); profile_job_.cancel(); replay_io_.cancel();file_tx_job_.cancel();
    if (advanced_tools_.GetSafeHwnd()) advanced_tools_.DestroyWindow();
    if (uart_console_.GetSafeHwnd()) uart_console_.DestroyWindow();
    if(workbench_&&workbench_->GetSafeHwnd())workbench_->DestroyWindow();
    stop_serial_thread();
    // Normal user close is deferred above. Teardown-only joins protect owner lifetime.
    if (serial_thread_.joinable()) serial_thread_.join();
    if (serial_operation_thread_.joinable()) serial_operation_thread_.join();
    KillTimer(IDT_HOTPLUG); KillTimer(IDT_UI);
    hover_help_.destroy();
    { std::lock_guard<std::mutex> lock(export_mutex_); stop_export_ = true; }
    export_wake_.notify_one();
    if (export_thread_.joinable()) export_thread_.join();
    save_settings();
    disconnect();
    stop_serial_thread();
    if (btn_font_) {
        DeleteObject(btn_font_);
        btn_font_ = nullptr;
    }
    // Child controls borrow this font for their entire window lifetime.
    if (ui_font_) {
        DeleteObject(ui_font_);
        ui_font_ = nullptr;
    }
    CDialog::OnDestroy();
}

void CMainDialog::OnGetMinMaxInfo(MINMAXINFO* info) {
    CDialog::OnGetMinMaxInfo(info);
    info->ptMinTrackSize.x = 800;
    info->ptMinTrackSize.y = 540;
}

void CMainDialog::log_line(const std::wstring& msg) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::ofstream f("app.log", std::ios::app);
    if (!f) {
        return;
    }
    SYSTEMTIME st;
    GetLocalTime(&st);
    char stamp[64] = {};
    sprintf_s(stamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    f << stamp;
    for (wchar_t ch : msg) {
        if (ch >= 0 && ch <= 127) {
            f << static_cast<char>(ch);
        } else {
            f << '?';
        }
    }
    f << "\n";
}
