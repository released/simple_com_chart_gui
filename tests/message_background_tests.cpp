#include "message_page.h"
#include "channel_panel.h"
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
    ChannelPanel channels;
    HWND host = nullptr;
    Fixture() {
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES};
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
bool panel_background(HWND panel) {
    RECT area{}; GetClientRect(panel, &area);
    const auto dc=GetDC(panel);
    if(!dc) throw std::runtime_error("Cannot inspect native channel surface");
    bool clean=true;
    for(const LONG x:{1L,area.right-2}) for(const LONG y:{1L,23L,49L,79L})
        clean=clean && GetPixel(dc,x,y)==GetSysColor(COLOR_BTNFACE);
    ReleaseDC(panel,dc); return clean;
}
void verify_channels() {
    Fixture fixture;
    check(fixture.channels.create(fixture.host,0,0,330,500,701),"create actual channel panel");
    fixture.channels.set_ui_font(reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    const std::unordered_map<std::string,double> values{{"P2_ma",8},{"P3_ma",5677},{"P4_ma",5303},{"P5_ma",13},{"P1_ma",7915},{"P6_ma",8489}};
    for(const auto& value:values)fixture.channels.ensure_channel(value.first,true,RGB(0,128,0));
    fixture.channels.update_values(values);
    const auto selection=fixture.channels.get_checkbox_state_map();
    fixture.page.append("Previous Messages page\r\nretained RX\r\n");fixture.page.refresh();
    const auto raw=fixture.page.buffer().bytes();
    WNDCLASSW previous{};previous.lpfnWndProc=DefWindowProcW;previous.hInstance=GetModuleHandleW(nullptr);
    previous.lpszClassName=L"SccPreviousPageSurface";previous.hbrBackground=reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
    RegisterClassW(&previous);
    const auto poison=CreateWindowExW(0,previous.lpszClassName,L"",WS_CHILD,0,0,330,500,fixture.host,nullptr,previous.hInstance,nullptr);
    if(!poison)throw std::runtime_error("Cannot create previous-page paint control");
    check((GetWindowLongPtrW(fixture.channels.hwnd(),GWL_STYLE)&WS_CLIPCHILDREN)!=0,"channel background clips its real child controls");
    for(const int width:{330,280,340}) {
        ShowWindow(fixture.channels.hwnd(),SW_HIDE);ShowWindow(fixture.page.hwnd(),SW_HIDE);
        MoveWindow(poison,0,0,width,500,TRUE);ShowWindow(poison,SW_SHOW);
        RedrawWindow(poison,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW);pump();
        const auto dc=GetDC(poison);const auto old=GetPixel(dc,1,23);ReleaseDC(poison,dc);
        check(old!=CLR_INVALID && old!=GetSysColor(COLOR_BTNFACE),"negative control leaves visible previous-page pixels");
        ShowWindow(poison,SW_HIDE);MoveWindow(fixture.channels.hwnd(),0,0,width,500,TRUE);ShowWindow(fixture.channels.hwnd(),SW_SHOW);
        RedrawWindow(fixture.host,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);pump();
        // Inspect resident pixels before any WM_PRINT/PrintWindow can repair them.
        check(panel_background(fixture.channels.hwnd()),"chart return clears previous-page pixels in exposed channel margins");
        const auto list=GetDlgItem(fixture.channels.hwnd(),3);
        check(IsWindowVisible(fixture.channels.hwnd()) && !IsWindowVisible(fixture.page.hwnd()) && ListView_GetItemCount(list)==6,"page switch has one visible page and six actual channels");
        check(fixture.channels.get_checkbox_state_map()==selection && fixture.page.buffer().bytes()==raw,"repaint preserves channel selection and retained RX");
        RECT cell{};GetClientRect(list,&cell);auto list_dc=GetDC(list);
        const auto before=GetPixel(list_dc,10,cell.bottom-10);ReleaseDC(list,list_dc);
        RedrawWindow(fixture.channels.hwnd(),nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_NOCHILDREN|RDW_UPDATENOW);pump();
        check(panel_background(fixture.channels.hwnd()),"partial channel erase keeps a clean native background");
        list_dc=GetDC(list);const auto after=GetPixel(list_dc,10,cell.bottom-10);ReleaseDC(list,list_dc);
        check(before!=CLR_INVALID && after==before,"channel-only background erase does not overwrite its list child");
        const auto dirty=GetDC(fixture.channels.hwnd());
        RECT left{0,0,3,100},right{width-3,0,width,100};
        FillRect(dirty,&left,reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
        FillRect(dirty,&right,reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
        ReleaseDC(fixture.channels.hwnd(),dirty);
        check(!panel_background(fixture.channels.hwnd()),"negative control reproduces dirty native pixels in exposed panel margins");
        RedrawWindow(fixture.channels.hwnd(),nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_NOCHILDREN|RDW_UPDATENOW);pump();
        check(panel_background(fixture.channels.hwnd()),"channel owns erase of dirty pixels without repainting hidden Messages");
    }
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
int wmain(int argc,wchar_t** argv) {
    try {
        PrivateDesktop desktop;
        if(argc==2 && std::wstring(argv[1])==L"--channel-panel")verify_channels();
        else if(argc==1)verify();
        else return 2;
        desktop.close();
    } catch (const std::exception& error) {
        std::cout << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return passed ? 0 : 1;
}
