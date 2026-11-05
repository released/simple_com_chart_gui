#include "message_page.h"
#include "resource.h"
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
bool passed = true;
void check(bool ok, const char* name) {
    std::cout << (ok ? "PASS: " : "FAIL: ") << name << '\n';
    passed = passed && ok;
}
void pump() {
    MSG message{};
    unsigned count = 0;
    while (count++ < 128 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
class PrivateDesktop {
public:
    PrivateDesktop() : original_(GetThreadDesktop(GetCurrentThreadId())) {
        const auto name = L"SccBackgroundTest_" + std::to_wstring(GetCurrentProcessId()) +
            L"_" + std::to_wstring(GetTickCount64());
        owned_ = CreateDesktopW(name.c_str(), nullptr, nullptr, 0,
            DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, nullptr);
        if (!owned_) throw std::runtime_error("Cannot create private non-input desktop");
        if (!SetThreadDesktop(owned_)) {
            CloseDesktop(owned_);
            owned_ = nullptr;
            throw std::runtime_error("Cannot select private thread desktop");
        }
    }
    ~PrivateDesktop() { close(); }
    void close() {
        if (!owned_) return;
        const bool restored = SetThreadDesktop(original_) != FALSE;
        check(restored, "original thread desktop restored without switching input desktop");
        const bool closed = CloseDesktop(owned_) != FALSE;
        check(closed, "private desktop closed after all test windows");
        if (closed) owned_ = nullptr;
    }
private:
    HDESK original_ = nullptr, owned_ = nullptr;
};
INT_PTR CALLBACK dialog_proc(HWND, UINT, WPARAM, LPARAM) { return FALSE; }
struct Fixture {
    MessagePage page;
    HWND host = nullptr;
    Fixture() {
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&controls);
        host = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_MAIN_DIALOG),
            nullptr, dialog_proc, 0);
        if (!host) throw std::runtime_error("Cannot create production dialog resource");
        if (!page.create(host, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)))) {
            DestroyWindow(host);
            host = nullptr;
            throw std::runtime_error("Cannot create production Messages page");
        }
        SetWindowLongPtrW(host, GWL_EXSTYLE, GetWindowLongPtrW(host, GWL_EXSTYLE) |
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
        ShowWindow(host, SW_SHOWNOACTIVATE);
        ShowWindow(page.hwnd(), SW_SHOW);
        resize(1100, 650);
        page.reset();
    }
    ~Fixture() { if (host) DestroyWindow(host); }
    void resize(int width, int height) {
        SetWindowPos(host, nullptr, 20, 20, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
        RECT area{};
        GetClientRect(host, &area);
        MoveWindow(page.hwnd(), 0, 0, area.right, area.bottom, TRUE);
        RedrawWindow(host, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        pump();
    }
    void partial_parent_erase() {
        RECT area{};
        GetClientRect(page.view(), &area);
        POINT start{0, 0};
        MapWindowPoints(page.view(), host, &start, 1);
        RECT client{};
        GetClientRect(host, &client);
        RECT damage{0, 0, client.right, start.y + area.bottom * 3 / 5};
        RedrawWindow(host, &damage, nullptr,
            RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
    }
};
std::array<COLORREF, 4> pixels(HWND view) {
    RECT area{};
    GetClientRect(view, &area);
    const auto dc = GetDC(view);
    if (!dc) throw std::runtime_error("Cannot read native Messages surface");
    const LONG x = std::max<LONG>(10, area.right - 100);
    std::array<COLORREF, 4> result{};
    const std::array<LONG, 4> rows{5, area.bottom / 4, area.bottom / 2, area.bottom - 10};
    for (size_t i = 0; i < rows.size(); ++i) result[i] = GetPixel(dc, x, rows[i]);
    ReleaseDC(view, dc);
    return result;
}
bool black(HWND view) {
    const auto values = pixels(view);
    return std::all_of(values.begin(), values.end(), [](COLORREF color) { return color == RGB(0, 0, 0); });
}
bool bottom(HWND view) {
    SCROLLINFO scroll{sizeof(scroll), SIF_RANGE | SIF_PAGE | SIF_POS};
    if (!GetScrollInfo(view, SB_VERT, &scroll)) return false;
    return scroll.nPos >= std::max(scroll.nMin, scroll.nMax - static_cast<int>(scroll.nPage) + 1) - 1;
}
void verify() {
    Fixture fixture;
    const auto style = GetWindowLongPtrW(fixture.host, GWL_STYLE);
    check((style & WS_CLIPCHILDREN) != 0, "production main dialog resource clips child painting");
    for (int i = 1; i <= 200; ++i) {
        fixture.page.append("log(timer)-V1: " + std::to_string(i) + "\r\n");
        fixture.page.refresh();
        pump();
    }
    const auto original = fixture.page.buffer().bytes();
    check(original.size() == 3892 && bottom(fixture.page.view()), "200 received lines fill view and follow newest line");
    check(black(fixture.page.view()), "native surface is black before parent-only repaint");
    // A negative control must reproduce stale gray without forcing the child to paint.
    SetWindowLongPtrW(fixture.host, GWL_STYLE, style & ~WS_CLIPCHILDREN);
    fixture.partial_parent_erase();
    const auto damaged = pixels(fixture.page.view());
    RECT pending{};
    check(damaged[0] == GetSysColor(COLOR_BTNFACE) && damaged[1] == GetSysColor(COLOR_BTNFACE) &&
        damaged[2] == GetSysColor(COLOR_BTNFACE) && damaged[3] == RGB(0, 0, 0),
        "negative control reproduces upper gray background with bottom intact");
    check(!GetUpdateRect(fixture.page.view(), &pending, FALSE), "overpaint does not schedule a child repaint");
    RedrawWindow(fixture.page.view(), nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
    check(black(fixture.page.view()), "child NOERASE paint restores black and is not the root cause");
    SetWindowLongPtrW(fixture.host, GWL_STYLE, style);
    fixture.partial_parent_erase();
    check(black(fixture.page.view()), "production clipping prevents identical parent-only gray overpaint");
    check(fixture.page.buffer().bytes() == original && fixture.page.buffer().skipped() == 0,
        "background repair preserves all raw RX bytes without input loss");
    fixture.resize(920, 720);
    fixture.partial_parent_erase();
    check(black(fixture.page.view()) && bottom(fixture.page.view()), "resize preserves background and follow-tail");
    SendMessageW(fixture.page.follow_control(), BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessageW(fixture.page.view(), WM_VSCROLL, SB_TOP, 0);
    const auto first = SendMessageW(fixture.page.view(), EM_GETFIRSTVISIBLELINE, 0, 0);
    for (int i = 201; i <= 240; ++i) {
        fixture.page.append("log(timer)-V1: " + std::to_string(i) + "\r\n");
        fixture.page.refresh();
        pump();
        fixture.partial_parent_erase();
    }
    check(black(fixture.page.view()) && SendMessageW(fixture.page.view(), EM_GETFIRSTVISIBLELINE, 0, 0) == first,
        "follow-off retains inspected line and black background during new RX");
    SendMessageW(fixture.page.follow_control(), BM_SETCHECK, BST_CHECKED, 0);
    fixture.page.append("tail\r\n");
    fixture.page.refresh();
    pump();
    fixture.partial_parent_erase();
    check(black(fixture.page.view()) && bottom(fixture.page.view()), "follow-on reaches newest line after further RX");
}
}
int wmain() {
    try {
        PrivateDesktop desktop;
        verify();
        desktop.close();
    } catch (const std::exception& error) {
        std::cout << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return passed ? 0 : 1;
}
