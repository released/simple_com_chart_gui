#include "message_page.h"
#include "ui_metrics.h"
#include <algorithm>

bool MessagePage::create(HWND parent, HFONT font) {
    WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TelemetryMessages"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); RegisterClassW(&wc);
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 500, 500, parent, nullptr, wc.hInstance, this);
    if (!hwnd_) return false;
    label_ = CreateWindowW(L"STATIC", L"Display:", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    mode_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        0, 0, 1, 100, hwnd_, reinterpret_cast<HMENU>(kMode), nullptr, nullptr);
    for (const auto* caption : {L"ASCII", L"HEX"}) SendMessageW(mode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(caption));
    SendMessageW(mode_, CB_SETCURSEL, 0, 0);
    follow_ = CreateWindowW(L"BUTTON", L"Auto-scroll", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(kFollow), nullptr, nullptr);
    SendMessageW(follow_, BM_SETCHECK, BST_CHECKED, 0);
    clear_ = CreateWindowW(L"BUTTON", L"Clear messages", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(kClear), nullptr, nullptr);
    font_label_ = CreateWindowW(L"STATIC", L"Font:", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    font_down_ = CreateWindowW(L"BUTTON", L"-", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(kFontDown), nullptr, nullptr);
    font_size_ = CreateWindowW(L"STATIC", L"10 pt", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    font_up_ = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(kFontUp), nullptr, nullptr);
    info_ = CreateWindowW(L"STATIC", L"Raw RX (unfiltered)", WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
        0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    view_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    SendMessageW(view_, EM_SETLIMITTEXT, 2 * 1024 * 1024, 0);
    for (auto child = GetWindow(hwnd_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    if (font) GetObjectW(font, sizeof(mono_lf_), &mono_lf_);
    wcscpy_s(mono_lf_.lfFaceName, L"Consolas"); mono_lf_.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    mono_lf_.lfWidth = 0;
    set_font_points(kDefaultFontPoints);
    return true;
}

bool MessagePage::set_font_points(int points) {
    if (!IsWindow(view_)) return false;
    points = std::clamp(points, kMinFontPoints, kMaxFontPoints);
    if (mono_font_ && points == font_points_) return true;
    HDC dc = GetDC(view_);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) ReleaseDC(view_, dc);
    auto lf = mono_lf_; lf.lfHeight = -MulDiv(points, dpi > 0 ? dpi : 96, 72);
    const auto font = CreateFontIndirectW(&lf);
    if (!font) return false;
    const auto first_line = SendMessageW(view_, EM_GETFIRSTVISIBLELINE, 0, 0);
    DWORD start = 0, end = 0;
    SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    const auto previous = mono_font_; mono_font_ = font; font_points_ = points;
    SendMessageW(view_, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    if (previous) DeleteObject(previous);
    if (follows()) {
        const auto length = GetWindowTextLengthW(view_);
        SendMessageW(view_, EM_SETSEL, length, length); SendMessageW(view_, EM_SCROLLCARET, 0, 0);
    } else {
        SendMessageW(view_, EM_SETSEL, start, end);
        SendMessageW(view_, EM_LINESCROLL, 0, first_line - SendMessageW(view_, EM_GETFIRSTVISIBLELINE, 0, 0));
    }
    update_font_controls();
    RECT client{}; GetClientRect(hwnd_, &client); layout(client.right, client.bottom);
    InvalidateRect(view_, nullptr, TRUE);
    return true;
}

void MessagePage::update_font_controls() {
    const auto size = std::to_wstring(font_points_) + L" pt";
    SetWindowTextW(font_size_, size.c_str());
    EnableWindow(font_down_, font_points_ > kMinFontPoints);
    EnableWindow(font_up_, font_points_ < kMaxFontPoints);
}

bool MessagePage::hex_mode() const { return SendMessageW(mode_, CB_GETCURSEL, 0, 0) == 1; }
std::string MessagePage::export_text() const {
    const auto text = hex_mode() ? buffer_.hex() : buffer_.ascii();
    // Both display formats contain only 7-bit text, including explicit byte escapes.
    std::string result; result.reserve(text.size());
    for (const auto character : text) result.push_back(static_cast<char>(character));
    return result;
}
bool MessagePage::follows() const { return SendMessageW(follow_, BM_GETCHECK, 0, 0) == BST_CHECKED; }

void MessagePage::append(std::string_view bytes, uint64_t skipped) {
    if (bytes.empty() && !skipped) return;
    buffer_.append(bytes, skipped); dirty_ = true;
}

void MessagePage::reset() {
    buffer_.reset(); rebuild_ = dirty_ = true;
    if (IsWindow(view_)) SetWindowTextW(view_, L"");
    refresh();
}

void MessagePage::refresh() {
    if (!dirty_ || !IsWindowVisible(hwnd_)) return;
    const auto size = buffer_.bytes().size();
    const bool full = rebuild_ || rendered_offset_ != buffer_.offset() || size < rendered_size_;
    const auto first_line = static_cast<int>(SendMessageW(view_, EM_GETFIRSTVISIBLELINE, 0, 0));
    DWORD selection_start = 0, selection_end = 0;
    SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&selection_start), reinterpret_cast<LPARAM>(&selection_end));
    SendMessageW(view_, WM_SETREDRAW, FALSE, 0);
    if (full) {
        const auto text = hex_mode() ? buffer_.hex() : buffer_.ascii();
        SetWindowTextW(view_, text.c_str());
    } else if (size > rendered_size_) {
        size_t start = rendered_size_;
        int replace_from = GetWindowTextLengthW(view_);
        if (hex_mode() && start % 16) {
            start -= start % 16;
            const int line = std::max(0, static_cast<int>(SendMessageW(view_, EM_GETLINECOUNT, 0, 0)) - 2);
            replace_from = static_cast<int>(SendMessageW(view_, EM_LINEINDEX, line, 0));
        }
        const auto text = hex_mode() ? buffer_.hex(start) : buffer_.ascii(start);
        SendMessageW(view_, EM_SETSEL, replace_from, GetWindowTextLengthW(view_));
        SendMessageW(view_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    }
    // EM_SCROLLCARET needs a visible edit; WM_SETREDRAW(FALSE) temporarily hides it.
    SendMessageW(view_, WM_SETREDRAW, TRUE, 0);
    if (follows()) {
        const auto length = GetWindowTextLengthW(view_);
        SendMessageW(view_, EM_SETSEL, length, length); SendMessageW(view_, EM_SCROLLCARET, 0, 0);
    } else {
        // A retained-history eviction clamps an inspected old view to the start.
        const bool evicted = rendered_offset_ != buffer_.offset();
        const auto length = static_cast<DWORD>(GetWindowTextLengthW(view_));
        SendMessageW(view_, EM_SETSEL, evicted ? 0 : std::min(selection_start, length), evicted ? 0 : std::min(selection_end, length));
        const int current = static_cast<int>(SendMessageW(view_, EM_GETFIRSTVISIBLELINE, 0, 0));
        SendMessageW(view_, EM_LINESCROLL, 0, (evicted ? 0 : first_line) - current);
    }
    InvalidateRect(view_, nullptr, TRUE);
    const auto info = L"Raw RX (unfiltered) | Bytes: " + std::to_wstring(size) + L" / 262144 | Dropped: " +
        std::to_wstring(buffer_.offset()) + L" | Input lost: " + std::to_wstring(buffer_.skipped());
    SetWindowTextW(info_, info.c_str());
    rendered_offset_ = buffer_.offset(); rendered_size_ = size; rebuild_ = dirty_ = false;
}

void MessagePage::layout(int w, int h) {
    if (!IsWindow(view_)) return;
    RECT selection{}; GetWindowRect(mode_, &selection);
    const int height = std::max(1, static_cast<int>(selection.bottom - selection.top));
    const int gap = 8, row = std::max({28, height, control_font_height(label_) + 12});
    const int top = (row - height) / 2;
    int x = 0;
    const auto place = [&](HWND control, int width, bool combo = false) {
        // Dropdown-list height is separate from its native closed selection field.
        MoveWindow(control, x, top, width, combo ? 150 : height, TRUE); x += width + gap;
    };
    place(label_, control_text_width(label_)); place(mode_, 100, true);
    place(follow_, control_text_width(follow_, GetSystemMetrics(SM_CXMENUCHECK) + 24));
    place(clear_, control_text_width(clear_, 24));
    place(font_label_, control_text_width(font_label_));
    place(font_down_, std::max(row, control_text_width(font_down_, 20)));
    place(font_size_, control_text_width(font_size_, 16));
    place(font_up_, std::max(row, control_text_width(font_up_, 20)));
    const int info_height = std::max(row, control_wrapped_text_height(info_, w));
    MoveWindow(info_, 0, row + gap, w, info_height, TRUE);
    const int view_top = row + gap + info_height + gap;
    MoveWindow(view_, 0, view_top, w, std::max(40, h - view_top), TRUE);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK MessagePage::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto self = reinterpret_cast<MessagePage*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        self = static_cast<MessagePage*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        self->hwnd_ = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT MessagePage::handle(UINT msg, WPARAM wp, LPARAM lp) {
    if ((msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT) && reinterpret_cast<HWND>(lp) == view_) {
        const auto dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, RGB(0, 255, 0)); SetBkColor(dc, RGB(0, 0, 0));
        return reinterpret_cast<LRESULT>(GetStockObject(BLACK_BRUSH));
    }
    if (msg == WM_SIZE) { layout(LOWORD(lp), HIWORD(lp)); return 0; }
    if (msg == WM_COMMAND) {
        if ((LOWORD(wp) == kFontDown || LOWORD(wp) == kFontUp) && HIWORD(wp) == BN_CLICKED) {
            const auto before = font_points_;
            if (set_font_points(before + (LOWORD(wp) == kFontUp ? 1 : -1)) && before != font_points_)
                SendMessageW(GetParent(hwnd_), kFontChanged, 0, 0);
            return 0;
        }
        if (LOWORD(wp) == kMode && HIWORD(wp) == CBN_SELCHANGE) { rebuild_ = dirty_ = true; refresh(); return 0; }
        if (LOWORD(wp) == kFollow && HIWORD(wp) == BN_CLICKED) { dirty_ = true; refresh(); return 0; }
        if (LOWORD(wp) == kClear && HIWORD(wp) == BN_CLICKED) { reset(); return 0; }
    }
    if (msg == WM_NCDESTROY) {
        const auto result = DefWindowProcW(hwnd_, msg, wp, lp);
        if (mono_font_) DeleteObject(mono_font_);
        mono_font_ = nullptr; SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0); hwnd_ = nullptr;
        return result;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}
