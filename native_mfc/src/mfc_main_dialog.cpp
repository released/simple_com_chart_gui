#include "mfc_main_dialog.h"

#pragma comment(lib, "comctl32.lib")

static constexpr int READ_INTERVAL_MS = 20;
static constexpr int HOTPLUG_SCAN_MS = 1000;
static constexpr int UI_UPDATE_MS = 50;
static constexpr int MAX_PENDING_LINES = 2000;

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

static constexpr int IDT_HOTPLUG = 1;
static constexpr int IDT_UI = 2;
static constexpr int IDT_AUTO = 3;
static constexpr int IDT_STATUS = 4;

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

static double now_seconds() {
    static LARGE_INTEGER freq = []{
        LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) / static_cast<double>(freq.QuadPart);
}

static COLORREF darken(COLORREF c, int delta) {
    int r = std::max(0, GetRValue(c) - delta);
    int g = std::max(0, GetGValue(c) - delta);
    int b = std::max(0, GetBValue(c) - delta);
    return RGB(r, g, b);
}

static COLORREF lighten(COLORREF c, int delta) {
    int r = std::min(255, GetRValue(c) + delta);
    int g = std::min(255, GetGValue(c) + delta);
    int b = std::min(255, GetBValue(c) + delta);
    return RGB(r, g, b);
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_DRAWITEM()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_BN_CLICKED(IDC_BTN_SNAPSHOT, &CMainDialog::OnSnapshotClicked)
    ON_BN_CLICKED(IDC_BTN_OVERLAY, &CMainDialog::OnOverlayClicked)
    ON_MESSAGE(WM_APP + 1, &CMainDialog::OnChannelChanged)
    ON_COMMAND(ID_HELP_LOGFORMAT, &CMainDialog::OnHelpLogFormat)
END_MESSAGE_MAP()

CMainDialog::CMainDialog(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent) {
}

BOOL CMainDialog::OnInitDialog() {
    CDialogEx::OnInitDialog();
    build_ui();
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

    return TRUE;
}

void CMainDialog::OnOK() {
    // prevent default Enter-to-close
}

void CMainDialog::OnCancel() {
    EndDialog(IDCANCEL);
}

void CMainDialog::build_ui() {
    label_com_ = CreateWindowW(L"STATIC", L"COM:", WS_CHILD | WS_VISIBLE, 0, 0, 40, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_port_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 300, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PORT)), nullptr, nullptr);
    btn_scan_ = CreateWindowW(L"BUTTON", L"Scan", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 80, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SCAN)), nullptr, nullptr);
    btn_connect_ = CreateWindowW(L"BUTTON", L"Connect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW | BS_NOTIFY,
        0, 0, 100, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CONNECT)), nullptr, nullptr);

    label_baud_ = CreateWindowW(L"STATIC", L"Baud", WS_CHILD | WS_VISIBLE, 0, 0, 40, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_baud_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
        0, 0, 100, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_BAUD)), nullptr, nullptr);

    label_data_ = CreateWindowW(L"STATIC", L"Data", WS_CHILD | WS_VISIBLE, 0, 0, 40, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_data_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_DATA)), nullptr, nullptr);

    label_parity_ = CreateWindowW(L"STATIC", L"Parity", WS_CHILD | WS_VISIBLE, 0, 0, 50, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_parity_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_PARITY)), nullptr, nullptr);

    label_stop_ = CreateWindowW(L"STATIC", L"Stop", WS_CHILD | WS_VISIBLE, 0, 0, 40, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_stop_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_STOP)), nullptr, nullptr);

    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"9600"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"19200"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"38400"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"57600"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"115200"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"230400"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"460800"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"921600"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1000000"));
    ::SendMessageW(combo_baud_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2000000"));
    ::SendMessageW(combo_baud_, CB_SETCURSEL, 4, 0);

    ::SendMessageW(combo_data_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    ::SendMessageW(combo_data_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"6"));
    ::SendMessageW(combo_data_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"7"));
    ::SendMessageW(combo_data_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"8"));
    ::SendMessageW(combo_data_, CB_SETCURSEL, 3, 0);

    ::SendMessageW(combo_parity_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"NONE"));
    ::SendMessageW(combo_parity_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"EVEN"));
    ::SendMessageW(combo_parity_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"ODD"));
    ::SendMessageW(combo_parity_, CB_SETCURSEL, 0, 0);

    ::SendMessageW(combo_stop_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1"));
    ::SendMessageW(combo_stop_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1.5"));
    ::SendMessageW(combo_stop_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2"));
    ::SendMessageW(combo_stop_, CB_SETCURSEL, 0, 0);

    btn_refresh_ = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW | BS_NOTIFY,
        0, 0, 80, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REFRESH)), nullptr, nullptr);
    btn_fit_ = CreateWindowW(L"BUTTON", L"Fit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW | BS_NOTIFY,
        0, 0, 80, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_FIT)), nullptr, nullptr);
    btn_snapshot_ = CreateWindowW(L"BUTTON", L"Snapshot", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_OWNERDRAW | BS_NOTIFY,
        0, 0, 90, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_SNAPSHOT)), nullptr, nullptr);
    btn_overlay_ = CreateWindowW(L"BUTTON", L"Overlay", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_OWNERDRAW | BS_NOTIFY,
        0, 0, 90, 24, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OVERLAY)), nullptr, nullptr);
    ::SendMessageW(btn_overlay_, BM_SETCHECK, BST_CHECKED, 0);

    label_auto_ = CreateWindowW(L"STATIC", L"Auto Refresh (s):", WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_auto_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_AUTO)), nullptr, nullptr);
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Off"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"10"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"30"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"60"));
    ::SendMessageW(combo_auto_, CB_SETCURSEL, 0, 0);

    label_time_ = CreateWindowW(L"STATIC", L"Time Window (s):", WS_CHILD | WS_VISIBLE, 0, 0, 120, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_time_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_TIME)), nullptr, nullptr);
    ::SendMessageW(combo_time_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    ::SendMessageW(combo_time_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"10"));
    ::SendMessageW(combo_time_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"30"));
    ::SendMessageW(combo_time_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"60"));
    ::SendMessageW(combo_time_, CB_SETCURSEL, 2, 0);
    model_.set_time_window(30.0);
    plot_view_.set_time_window(30.0);

    channel_panel_.create(m_hWnd, 0, 0, 300, 300, IDC_CHANNEL_PANEL);
    plot_view_.create(m_hWnd, 0, 0, 300, 300, IDC_PLOT_VIEW);
    plot_view_.set_model(&model_);

    LOGFONTW lf = {};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
    lf.lfWeight = FW_BOLD;
    btn_font_ = CreateFontIndirectW(&lf);
    if (btn_font_) {
        ::SendMessageW(btn_connect_, WM_SETFONT, reinterpret_cast<WPARAM>(btn_font_), TRUE);
        ::SendMessageW(btn_refresh_, WM_SETFONT, reinterpret_cast<WPARAM>(btn_font_), TRUE);
        ::SendMessageW(btn_fit_, WM_SETFONT, reinterpret_cast<WPARAM>(btn_font_), TRUE);
        ::SendMessageW(btn_snapshot_, WM_SETFONT, reinterpret_cast<WPARAM>(btn_font_), TRUE);
        ::SendMessageW(btn_overlay_, WM_SETFONT, reinterpret_cast<WPARAM>(btn_font_), TRUE);
    }

    status_ = CreateStatusWindowW(WS_CHILD | WS_VISIBLE, L"", m_hWnd, IDC_STATUS);
    int parts[2] = { 600, -1 };
    ::SendMessageW(status_, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
    set_left_status(L"COM: Disconnected");
    set_right_status(L"Samples: 0 | CH: 0");
}

void CMainDialog::layout_ui(int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    int top_h = 36;
    int bottom_h = 36;
    int status_h = 24;
    int padding = 8;

    ::MoveWindow(label_com_, padding, padding + 4, 40, 20, TRUE);
    ::MoveWindow(combo_port_, padding + 50, padding, 300, 200, TRUE);
    ::MoveWindow(btn_scan_, padding + 360, padding, 60, 24, TRUE);
    ::MoveWindow(btn_connect_, padding + 430, padding, 90, 24, TRUE);
    ::MoveWindow(label_baud_, padding + 535, padding + 4, 40, 20, TRUE);
    ::MoveWindow(combo_baud_, padding + 580, padding, 90, 200, TRUE);
    ::MoveWindow(label_data_, padding + 680, padding + 4, 40, 20, TRUE);
    ::MoveWindow(combo_data_, padding + 720, padding, 60, 200, TRUE);
    ::MoveWindow(label_parity_, padding + 790, padding + 4, 50, 20, TRUE);
    ::MoveWindow(combo_parity_, padding + 840, padding, 80, 200, TRUE);
    ::MoveWindow(label_stop_, padding + 930, padding + 4, 40, 20, TRUE);
    ::MoveWindow(combo_stop_, padding + 970, padding, 80, 200, TRUE);

    int bottom_y = top_h + padding;
    ::MoveWindow(btn_refresh_, padding, bottom_y, 80, 24, TRUE);
    ::MoveWindow(btn_fit_, padding + 90, bottom_y, 80, 24, TRUE);
    ::MoveWindow(btn_snapshot_, padding + 180, bottom_y, 90, 24, TRUE);
    ::MoveWindow(btn_overlay_, padding + 280, bottom_y, 90, 24, TRUE);
    ::MoveWindow(label_auto_, padding + 380, bottom_y + 4, 120, 20, TRUE);
    ::MoveWindow(combo_auto_, padding + 510, bottom_y, 80, 200, TRUE);
    ::MoveWindow(label_time_, padding + 610, bottom_y + 4, 120, 20, TRUE);
    ::MoveWindow(combo_time_, padding + 750, bottom_y, 80, 200, TRUE);

    int main_top = bottom_y + bottom_h + padding;
    int main_h = h - main_top - status_h - padding;
    int left_w = 360;

    ::MoveWindow(channel_panel_.hwnd(), padding, main_top, left_w, main_h, TRUE);
    ::MoveWindow(plot_view_.hwnd(), padding + left_w + padding, main_top,
        w - left_w - (3 * padding), main_h, TRUE);

    ::SendMessageW(status_, WM_SIZE, 0, 0);
}

void CMainDialog::OnSize(UINT nType, int cx, int cy) {
    CDialogEx::OnSize(nType, cx, cy);
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

    if (combo_port_) {
        layout_ui(cx, cy);
    }
}

void CMainDialog::start_serial_thread() {
    if (serial_running_) {
        return;
    }
    serial_running_ = true;
    serial_thread_ = std::thread([this]() {
        while (serial_running_) {
            std::wstring error;
            auto lines = serial_mgr_.read_lines(&error);
            if (!error.empty()) {
                {
                    std::lock_guard<std::mutex> lock(error_mutex_);
                    serial_error_ = error;
                }
                serial_error_pending_ = true;
                break;
            }
            if (!lines.empty()) {
                double ts = now_seconds();
                std::lock_guard<std::mutex> lock(pending_mutex_);
                for (const auto& line : lines) {
                    pending_.lines.push_back(PendingData::Item{line, ts});
                }
                if (pending_.lines.size() > MAX_PENDING_LINES) {
                    size_t overflow = pending_.lines.size() - MAX_PENDING_LINES;
                    pending_.lines.erase(pending_.lines.begin(), pending_.lines.begin() + overflow);
                    pending_dropped_ += static_cast<int>(overflow);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(READ_INTERVAL_MS));
        }
    });
}

void CMainDialog::stop_serial_thread() {
    if (!serial_running_) {
        return;
    }
    serial_running_ = false;
    if (serial_thread_.joinable()) {
        serial_thread_.join();
    }
}

void CMainDialog::scan_ports() {
    auto ports = serial_mgr_.scan_ports();
    update_port_combo(ports);
}

void CMainDialog::update_port_combo(const std::vector<SerialPortInfo>& ports) {
    std::wstring current;
    int cur_index = static_cast<int>(::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0));
    if (cur_index != CB_ERR && cur_index < static_cast<int>(known_ports_.size())) {
        current = known_ports_[cur_index].device;
    }

    known_ports_ = ports;

    ::SendMessageW(combo_port_, CB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < ports.size(); ++i) {
        const auto& p = ports[i];
        std::wstring label = p.device + L" - " + p.description;
        int idx = static_cast<int>(::SendMessageW(combo_port_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
        ::SendMessageW(combo_port_, CB_SETITEMDATA, idx, static_cast<LPARAM>(i));
    }

    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].device == current) {
            ::SendMessageW(combo_port_, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
            return;
        }
    }

    if (!ports.empty()) {
        ::SendMessageW(combo_port_, CB_SETCURSEL, 0, 0);
    }
}

void CMainDialog::on_connect_toggle() {
    if (!serial_mgr_.is_connected()) {
        if (!connect_with_validation()) {
            return;
        }
    } else {
        disconnect();
    }
}

bool CMainDialog::connect_with_validation() {
    if (::SendMessageW(combo_port_, CB_GETCOUNT, 0, 0) == 0) {
        scan_ports();
    }

    if (known_ports_.empty()) {
        set_left_status(L"COM: No port detected");
        log_line(L"Connect failed: no port detected");
        return false;
    }

    int sel = static_cast<int>(::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0));
    if (sel == CB_ERR || sel >= static_cast<int>(known_ports_.size())) {
        set_left_status(L"COM: Selected port disappeared");
        log_line(L"Connect failed: selected port disappeared");
        return false;
    }

    auto port = known_ports_[sel].device;
    wchar_t baud_buf[32] = {};
    ::GetWindowTextW(combo_baud_, baud_buf, 31);
    int baud = _wtoi(baud_buf);
    if (baud <= 0) {
        set_left_status(L"COM: Invalid baud rate");
        log_line(L"Connect failed: invalid baud rate");
        return false;
    }

    wchar_t data_buf[8] = {};
    ::GetWindowTextW(combo_data_, data_buf, 7);
    int data_bits = _wtoi(data_buf);
    if (data_bits < 5 || data_bits > 8) {
        set_left_status(L"COM: Invalid data bits");
        log_line(L"Connect failed: invalid data bits");
        return false;
    }

    wchar_t parity_buf[8] = {};
    ::GetWindowTextW(combo_parity_, parity_buf, 7);
    std::wstring parity_text = parity_buf;
    int parity = NOPARITY;
    if (parity_text == L"EVEN") {
        parity = EVENPARITY;
    } else if (parity_text == L"ODD") {
        parity = ODDPARITY;
    }

    wchar_t stop_buf[8] = {};
    ::GetWindowTextW(combo_stop_, stop_buf, 7);
    std::wstring stop_text = stop_buf;
    int stop = ONESTOPBIT;
    if (stop_text == L"1.5") {
        stop = ONE5STOPBITS;
    } else if (stop_text == L"2") {
        stop = TWOSTOPBITS;
    }

    std::wstring error;
    if (!serial_mgr_.connect(port, baud, data_bits, parity, stop, &error)) {
        if (!error.empty()) {
            set_left_status(L"COM: Open failed: " + error);
            log_line(L"Connect failed: " + error);
        } else {
            set_left_status(L"COM: Open failed");
            log_line(L"Connect failed: open failed");
        }
        return false;
    }

    model_.reset();
    wchar_t time_buf[8] = {};
    ::GetWindowTextW(combo_time_, time_buf, 7);
    double time_window = _wtof(time_buf);
    model_.set_time_window(time_window);
    plot_view_.reset_visual();
    channel_panel_.reset();

    start_serial_thread();

    std::wstring parity_short = L"N";
    if (parity == EVENPARITY) {
        parity_short = L"E";
    } else if (parity == ODDPARITY) {
        parity_short = L"O";
    }
    std::wstring status = L"COM: Connected " + port + L" (" + std::to_wstring(baud) + L"," +
        std::to_wstring(data_bits) + parity_short + stop_text + L")";
    set_left_status(status);
    log_line(L"Connected: " + status);
    ::SetWindowTextW(btn_connect_, L"Disconnect");
    ::InvalidateRect(btn_connect_, nullptr, TRUE);

    ::EnableWindow(combo_port_, FALSE);
    ::EnableWindow(combo_baud_, FALSE);
    ::EnableWindow(combo_data_, FALSE);
    ::EnableWindow(combo_parity_, FALSE);
    ::EnableWindow(combo_stop_, FALSE);
    ::EnableWindow(btn_scan_, FALSE);

    return true;
}

void CMainDialog::disconnect() {
    stop_serial_thread();
    serial_mgr_.disconnect();
    ::SetWindowTextW(btn_connect_, L"Connect");
    ::InvalidateRect(btn_connect_, nullptr, TRUE);
    set_left_status(L"COM: Disconnected");

    ::EnableWindow(combo_port_, TRUE);
    ::EnableWindow(combo_baud_, TRUE);
    ::EnableWindow(combo_data_, TRUE);
    ::EnableWindow(combo_parity_, TRUE);
    ::EnableWindow(combo_stop_, TRUE);
    ::EnableWindow(btn_scan_, TRUE);

    ::SendMessageW(combo_auto_, CB_SETCURSEL, 0, 0);
    KillTimer(IDT_AUTO);

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_.lines.clear();
        pending_dropped_ = 0;
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
        disconnect();
        if (!err.empty()) {
            set_left_status(L"COM: " + err);
            log_line(L"Serial error: " + err);
        } else {
            set_left_status(L"COM: Disconnected");
            log_line(L"Serial disconnected");
        }
        return;
    }

    PendingData pending;
    int dropped_lines = 0;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending = pending_;
        pending_.lines.clear();
        dropped_lines = pending_dropped_;
        pending_dropped_ = 0;
    }

    if (pending.lines.empty()) {
        return;
    }

    double now = 0.0;
    for (const auto& item : pending.lines) {
        auto kv = log_parser::parse_kv_log(item.line);
        if (!kv.empty()) {
            double ts = item.ts > 0.0 ? item.ts : now_seconds();
            model_.update_from_kv(kv, ts);
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

    if (!snapshot_) {
        plot_view_.update_from_model(now);
    }

    std::unordered_map<std::string, int> latest;
    for (const auto& key : model_.get_keys()) {
        auto series = model_.get_series(key);
        if (!series.empty()) {
            latest[key] = series.back().v;
        }
    }
    channel_panel_.update_values(latest);

    set_right_status(L"Samples: " + std::to_wstring(model_.get_total_samples()) + L" | CH: " + std::to_wstring(model_.get_enabled_count()));

    int dropped_keys = model_.consume_dropped_keys();
    if (dropped_keys > 0) {
        show_status_message(L"Channel limit reached (max 16), ignored new keys", 5000);
        log_line(L"Channel limit reached, ignored new keys");
    }

    if (dropped_lines > 0) {
        show_status_message(L"Input overrun: dropped lines", 3000);
        log_line(L"Input overrun: dropped lines");
    }

    int rx_overflow = serial_mgr_.consume_rx_overflow();
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
        channel_panel_.ensure_channel(key, model_.is_enabled(key), color);
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
    right_status_ = text;
    ::SendMessageW(status_, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(text.c_str()));
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

    COLORREF bg = RGB(120, 120, 120);
    COLORREF fg = RGB(255, 255, 255);
    bool checked = (::SendMessageW(dis->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED);

    switch (dis->CtlID) {
    case IDC_BTN_REFRESH:
        bg = RGB(0x19, 0x76, 0xD2);
        break;
    case IDC_BTN_FIT:
        bg = RGB(0x7B, 0x1F, 0xA2);
        break;
    case IDC_BTN_SNAPSHOT:
        bg = checked ? RGB(0xEF, 0x6C, 0x00) : RGB(0x45, 0x5A, 0x64);
        break;
    case IDC_BTN_OVERLAY:
        bg = checked ? RGB(0x2E, 0x7D, 0x32) : RGB(0x61, 0x61, 0x61);
        break;
    case IDC_BTN_CONNECT:
        bg = serial_mgr_.is_connected() ? RGB(0xE5, 0x39, 0x35) : RGB(0x4C, 0xAF, 0x50);
        break;
    default:
        break;
    }

    if (dis->itemState & ODS_SELECTED) {
        bg = RGB(0x9E, 0x9E, 0x9E);
    } else if ((dis->itemState & ODS_HOTLIGHT) || dis->hwndItem == hover_btn_) {
        bg = lighten(bg, 18);
    } else if (dis->itemState & ODS_DISABLED) {
        bg = darken(bg, 80);
        fg = RGB(220, 220, 220);
    }

    HBRUSH brush = CreateSolidBrush(bg);
    HPEN pen = CreatePen(PS_SOLID, 1, bg);
    HGDIOBJ old_pen = SelectObject(dis->hDC, pen);
    HGDIOBJ old_brush = SelectObject(dis->hDC, brush);
    RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 12, 12);
    SelectObject(dis->hDC, old_pen);
    SelectObject(dis->hDC, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, fg);

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
    if (is_minimized_ && nIDEvent == IDT_UI) {
        return;
    }
    if (nIDEvent == IDT_HOTPLUG) {
        if (!serial_mgr_.is_connected()) {
            auto ports = serial_mgr_.scan_ports();
            bool changed = ports.size() != known_ports_.size();
            if (!changed) {
                for (size_t i = 0; i < ports.size(); ++i) {
                    if (ports[i].device != known_ports_[i].device) {
                        changed = true;
                        break;
                    }
                }
            }
            if (changed) {
                update_port_combo(ports);
                show_status_message(L"COM: List updated", 2000);
            }
        }
    } else if (nIDEvent == IDT_UI) {
        flush_pending_lines();
    } else if (nIDEvent == IDT_AUTO) {
        ::SendMessageW(m_hWnd, WM_COMMAND, IDC_BTN_REFRESH, 0);
    } else if (nIDEvent == IDT_STATUS) {
        flash_active_ = false;
        KillTimer(IDT_STATUS);
        ::SendMessageW(status_, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(left_status_.c_str()));
    }
    CDialogEx::OnTimer(nIDEvent);
}

BOOL CMainDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(wParam)) {
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
    return CDialogEx::OnCommand(wParam, lParam);
}

void CMainDialog::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) {
    draw_owner_button(lpDrawItemStruct);
    CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMainDialog::OnMouseMove(UINT nFlags, CPoint point) {
    POINT pt = { point.x, point.y };
    update_hover_button(pt, false);
    CDialogEx::OnMouseMove(nFlags, point);
}

void CMainDialog::OnMouseLeave() {
    update_hover_button(POINT{}, true);
    CDialogEx::OnMouseLeave();
}

LRESULT CMainDialog::OnChannelChanged(WPARAM, LPARAM) {
    sync_channels();
    plot_view_.request_temporary_fit(now_seconds(), 0.5);
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
    disconnect();
    stop_serial_thread();
    if (btn_font_) {
        DeleteObject(btn_font_);
        btn_font_ = nullptr;
    }
    CDialogEx::OnDestroy();
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
