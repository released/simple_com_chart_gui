#include "mfc_main_dialog.h"
#include "ui_metrics.h"

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

    label_auto_ = CreateWindowW(L"STATIC", L"Auto Refresh (s):", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 120, 20, m_hWnd, nullptr, nullptr, nullptr);
    combo_auto_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 80, 200, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_AUTO)), nullptr, nullptr);
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Off"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"10"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"30"));
    ::SendMessageW(combo_auto_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"60"));
    ::SendMessageW(combo_auto_, CB_SETCURSEL, 0, 0);

    label_time_ = CreateWindowW(L"STATIC", L"Time Window (s):", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 120, 20, m_hWnd, nullptr, nullptr, nullptr);
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

    btn_save_ = CreateWindowW(L"BUTTON", L"Save As...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 100, 26, m_hWnd, reinterpret_cast<HMENU>(207), nullptr, nullptr);
    tabs_ = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
        0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(601), nullptr, nullptr);
    TCITEMW tab{}; tab.mask = TCIF_TEXT;
    tab.pszText = const_cast<wchar_t*>(L"Chart"); TabCtrl_InsertItem(tabs_, 0, &tab);
    tab.pszText = const_cast<wchar_t*>(L"Data / 4 Charts"); TabCtrl_InsertItem(tabs_, 1, &tab);
    check_show_name_ = CreateWindowW(L"BUTTON", L"Show name", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_CHECK_SHOW_NAME), nullptr, nullptr);
    ::SendMessageW(check_show_name_, BM_SETCHECK, BST_CHECKED, 0);

    LOGFONTW lf = {};
    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
    if (std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125") != std::wstring::npos)
        lf.lfHeight = MulDiv(lf.lfHeight, 5, 4);
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
    if (w <= 0 || h <= 0 || !::IsWindow(label_include_) || !::IsWindow(tabs_)) return;
    const int gap = 8, row = std::max(28, control_font_height(label_include_) + 12);
    int x = gap, y = gap;
    const auto place = [&x, &y, row](HWND control, int width, bool combo = false, bool label = false) {
        if (::IsWindow(control)) ::MoveWindow(control, x, y + (label ? 4 : 0), width, combo ? 260 : row - 2, TRUE);
        x += width + 8;
    };
    const auto label = [&place](HWND control) { place(control, control_text_width(control), false, true); };
    label(label_com_); place(combo_port_, std::max(170, std::min(300, w - 880)), true);
    place(btn_scan_, control_text_width(btn_scan_, 24)); place(btn_connect_, control_text_width(btn_connect_, 32));
    label(label_baud_); place(combo_baud_, 90, true); label(label_data_); place(combo_data_, 55, true);
    label(label_parity_); place(combo_parity_, 80, true); label(label_stop_); place(combo_stop_, 55, true);
    y += row + gap; x = gap;
    for (auto button : {btn_refresh_, btn_fit_, btn_snapshot_, btn_overlay_}) place(button, std::max(70, control_text_width(button, 24)));
    label(label_auto_); place(combo_auto_, 65, true); label(label_time_); place(combo_time_, 65, true);
    place(btn_save_, control_text_width(btn_save_, 28));
    y += row + gap; x = gap;
    const int fixed = control_text_width(label_include_) + control_text_width(label_exclude_) +
        control_text_width(label_limit_) + 90 + 55 + 75 + 9 * gap;
    const int field = std::max(80, (w - fixed) / 2);
    label(label_include_); place(edit_include_, field); label(label_exclude_); place(edit_exclude_, field);
    place(combo_filter_, 90, true); label(label_limit_); place(edit_limit_, 55); place(btn_apply_, 75);
    y += row;
    for (auto help : {label_filter_, label_filter_include_, label_filter_exclude_}) {
        const int height = control_wrapped_text_height(help, w - 2 * gap);
        ::MoveWindow(help, gap, y, w - 2 * gap, height, TRUE);
        y += height;
    }
    y += 4;
    const int name_width = control_text_width(check_show_name_, GetSystemMetrics(SM_CXMENUCHECK) + 24);
    ::MoveWindow(tabs_, gap, y, w - name_width - 3 * gap, row, TRUE);
    ::MoveWindow(check_show_name_, w - name_width - gap, y, name_width, row, TRUE);
    const int main_top = y + row + gap, main_h = std::max(100, h - main_top - 24 - gap);
    const int left_w = 330;
    ::MoveWindow(channel_panel_.hwnd(), gap, main_top, left_w, main_h, TRUE);
    ::MoveWindow(plot_view_.hwnd(), left_w + 2 * gap, main_top, w - left_w - 3 * gap, main_h, TRUE);
    if (::IsWindow(data_page_.hwnd())) ::MoveWindow(data_page_.hwnd(), gap, main_top, w - 2 * gap, main_h, TRUE);
    ::SendMessageW(status_, WM_SIZE, 0, 0);
    int parts[2] = {std::max(400, w - 320), -1};
    ::SendMessageW(status_, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
}

void CMainDialog::switch_page() {
    if (!::IsWindow(tabs_)) return;
    const bool data = TabCtrl_GetCurSel(tabs_) == 1;
    ::ShowWindow(channel_panel_.hwnd(), data ? SW_HIDE : SW_SHOW);
    ::ShowWindow(plot_view_.hwnd(), data ? SW_HIDE : SW_SHOW);
    ::ShowWindow(data_page_.hwnd(), data ? SW_SHOW : SW_HIDE);
    for (auto control : {btn_refresh_, btn_fit_, btn_snapshot_, btn_overlay_, combo_auto_, combo_time_, label_auto_, label_time_})
        ::EnableWindow(control, !data);
    if (data) data_page_.refresh(-1);
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

BOOL CMainDialog::OnNotify(WPARAM wp, LPARAM lp, LRESULT* result) {
    const auto header = reinterpret_cast<NMHDR*>(lp);
    if (header && header->hwndFrom == tabs_ && header->code == TCN_SELCHANGE) {
        switch_page(); *result = 0; return TRUE;
    }
    return CDialogEx::OnNotify(wp, lp, result);
}
