#include "mfc_main_dialog.h"

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

    int filter_y = bottom_y + bottom_h;
    int field_w = std::max(130, (w - 670) / 2);
    ::MoveWindow(label_include_, padding, filter_y + 4, 52, 22, TRUE);
    ::MoveWindow(edit_include_, 64, filter_y, field_w, 26, TRUE);
    ::MoveWindow(label_exclude_, 74 + field_w, filter_y + 4, 52, 22, TRUE);
    ::MoveWindow(edit_exclude_, 130 + field_w, filter_y, field_w, 26, TRUE);
    int x = 140 + 2 * field_w;
    ::MoveWindow(combo_filter_, x, filter_y, 90, 150, TRUE);
    ::MoveWindow(label_limit_, x + 100, filter_y + 4, 60, 22, TRUE);
    ::MoveWindow(edit_limit_, x + 164, filter_y, 60, 26, TRUE);
    ::MoveWindow(btn_apply_, x + 234, filter_y, 75, 26, TRUE);
    ::MoveWindow(label_filter_, padding, filter_y + 29, w - 2 * padding, 22, TRUE);

    int main_top = filter_y + 60;
    int main_h = h - main_top - status_h - padding;
    int left_w = 360;

    ::MoveWindow(channel_panel_.hwnd(), padding, main_top, left_w, main_h, TRUE);
    ::MoveWindow(plot_view_.hwnd(), padding + left_w + padding, main_top,
        w - left_w - (3 * padding), main_h, TRUE);

    ::SendMessageW(status_, WM_SIZE, 0, 0);
    int parts[2] = {std::max(400, w - 320), -1};
    ::SendMessageW(status_, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
}
