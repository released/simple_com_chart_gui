#include "message_page.h"
#include "ui_language.h"
#include <commctrl.h>
#include <richedit.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
bool passed = true;
void check(bool ok, const char* name) {
    std::cout << (ok ? "PASS: " : "FAIL: ") << name << '\n';
    passed = passed && ok;
}
class PrivateDesktop {
public:
    PrivateDesktop() : original_desktop_(GetThreadDesktop(GetCurrentThreadId())) {
        const auto name = L"SccCopy_" + std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(GetTickCount64());
        desktop_ = CreateDesktopW(name.c_str(), nullptr, nullptr, 0,
            DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS, nullptr);
        if (!desktop_ || !SetThreadDesktop(desktop_)) {
            if (desktop_) CloseDesktop(desktop_);
            desktop_ = nullptr;
            throw std::runtime_error("Cannot select private non-input desktop");
        }
    }
    ~PrivateDesktop() { close(); }
    void close() {
        if (!desktop_) return;
        check(SetThreadDesktop(original_desktop_) != FALSE, "original thread desktop restored without input switching");
        const bool ok = CloseDesktop(desktop_) != FALSE;
        check(ok, "private desktop closed without touching user clipboard");
        if (ok) desktop_ = nullptr;
    }
private:
    HDESK original_desktop_ = nullptr, desktop_ = nullptr;
};
struct KeyboardState {
    std::array<BYTE, 256> previous{};
    KeyboardState() {
        if (!GetKeyboardState(previous.data())) throw std::runtime_error("Cannot read private thread keyboard state");
        std::array<BYTE, 256> control{};
        control[VK_CONTROL] = control[VK_LCONTROL] = 0x80;
        if (!SetKeyboardState(control.data())) throw std::runtime_error("Cannot seed private thread Ctrl state");
    }
    ~KeyboardState() { SetKeyboardState(previous.data()); }
};
struct CopyProbe {
    HWND view; bool rich, busy = false; unsigned requests = 0; std::wstring text;
    CopyProbe(HWND control, bool is_rich) : view(control), rich(is_rich) {
        if (!SetWindowSubclass(view, proc, 7, reinterpret_cast<DWORD_PTR>(this))) throw std::runtime_error("Cannot intercept copy in owned control");
    }
    ~CopyProbe() { if (IsWindow(view)) RemoveWindowSubclass(view, proc, 7); }
    static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data) {
        auto& self = *reinterpret_cast<CopyProbe*>(data);
        if (message != WM_COPY) return DefSubclassProc(window, message, w, l);
        ++self.requests;
        // Intercept only the native clipboard boundary; never open the user's clipboard.
        if (!self.busy) {
            DWORD start = 0, end = 0;
            if (self.rich) { CHARRANGE range{}; SendMessageW(window, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&range)); start = range.cpMin; end = range.cpMax; }
            else SendMessageW(window, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
            std::wstring shown(static_cast<size_t>(GetWindowTextLengthW(window)) + 1, L'\0');
            GetWindowTextW(window, shown.data(), static_cast<int>(shown.size())); shown.pop_back();
            const auto first = std::min<size_t>(start, shown.size()), last = std::min<size_t>(end, shown.size());
            self.text = shown.substr(first, last - first);
        }
        return 0;
    }
};
struct Fixture {
    MessagePage page;
    HWND host = nullptr;
    std::vector<MessagePage::KeyboardTx> tx;
    std::vector<WPARAM> generations;
    static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM w, LPARAM l) {
        if (message == WM_NCCREATE) SetWindowLongPtrW(window, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
        const auto self = reinterpret_cast<Fixture*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self && message == MessagePage::kKeyboardTx) {
            self->tx.push_back(*reinterpret_cast<const MessagePage::KeyboardTx*>(l));
            self->generations.push_back(w); return 1;
        }
        return DefWindowProcW(window, message, w, l);
    }
    Fixture() {
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
        WNDCLASSW type{}; type.lpfnWndProc = proc; type.hInstance = GetModuleHandleW(nullptr); type.lpszClassName = L"SccMessageCopyTest";
        RegisterClassW(&type);
        host = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, type.lpszClassName, L"Private clipboard test",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0, 900, 700, nullptr, nullptr, type.hInstance, this);
        if (!host) throw std::runtime_error("Cannot create clipboard fixture host");
        if (!page.create(host, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)))) {
            DestroyWindow(host); host = nullptr; throw std::runtime_error("Cannot create Messages page");
        }
        ShowWindow(host, SW_SHOWNOACTIVATE); ShowWindow(page.hwnd(), SW_SHOW);
        MoveWindow(page.hwnd(), 0, 0, 880, 650, TRUE);
        SendMessageW(page.follow_control(), BM_SETCHECK, BST_UNCHECKED, 0);
    }
    ~Fixture() { if (host) DestroyWindow(host); }
    void option(std::wstring_view caption) {
        const auto menu = page.build_options_menu(); UINT command = 0;
        for (int i = 0; i < GetMenuItemCount(menu); ++i) {
            wchar_t text[128]{}; GetMenuStringW(menu, i, text, 128, MF_BYPOSITION);
            if (std::wstring_view(text) == ui::tr(caption)) command = GetMenuItemID(menu, i);
        }
        DestroyMenu(menu);
        if (!command) throw std::runtime_error("Missing Messages option");
        SendMessageW(page.hwnd(), WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
    void wrap(bool enabled) { if (page.wraps() != enabled) option(L"Word wrap"); }
    bool key(WPARAM key) {
        MSG message{}; message.hwnd = page.view(); message.message = WM_KEYDOWN; message.wParam = key;
        return page.translate_key(message);
    }
};
void verify(Fixture& fixture, bool rich) {
    std::cout << "MODE: " << (rich ? "RichEdit" : "EDIT") << '\n';
    fixture.wrap(rich); fixture.page.reset(); fixture.tx.clear(); fixture.generations.clear();
    MessageDisplayOptions options; options.encoding = MessageEncoding::Utf8; fixture.page.configure_display(options);
    wchar_t class_name[64]{}; GetClassNameW(fixture.page.view(), class_name, 64);
    check(_wcsicmp(class_name, rich ? MSFTEDIT_CLASS : L"EDIT") == 0, "fixture exercises requested native control");
    CopyProbe copy(fixture.page.view(), rich);
    const std::wstring sample = L"alpha \u03a9 \u4e2d\u6587 \U0001f642";
    fixture.page.append(message_utf8(sample)); fixture.page.refresh();
    fixture.page.set_keyboard_available(true, 77, true);
    check(fixture.page.keyboard_armed(), "valid connection still enables keyboard TX by default");
    copy.text = L"sentinel";
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, 5);
    check(fixture.key('C') && copy.text == L"alpha" && copy.requests == 1 && fixture.tx.empty(), "Ctrl+C forwards exact selected text to WM_COPY without any UART request");
    check(fixture.page.keyboard_armed(), "copy does not disarm keyboard TX");
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, -1);
    SendMessageW(fixture.page.view(), WM_KEYDOWN, 'C', 0);
    check(copy.text == sample && copy.requests == 2 && fixture.tx.empty(), "control subclass forwards Unicode and surrogate pairs to WM_COPY without TX");
    check(fixture.page.buffer().bytes() == message_utf8(sample) && fixture.page.export_text() == message_utf8(sample), "copy leaves raw history and export unchanged");
    copy.text = L"unchanged";
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, 0);
    check(fixture.key('C') && fixture.tx.size() == 1 && fixture.tx.back().bytes == "\x03" &&
        !fixture.tx.back().paste && fixture.generations.back() == 77 && copy.text == L"unchanged" && copy.requests == 2,
        "unselected Ctrl+C still sends exactly byte03 for the current generation");
    fixture.tx.clear();
    fixture.page.arm_keyboard(false); SendMessageW(fixture.page.view(), EM_SETSEL, 0, 5);
    check(fixture.key('C') && copy.text == L"alpha" && fixture.tx.empty() && !fixture.page.keyboard_armed(), "manually disabled keyboard TX still permits copy without rearming");
    fixture.page.set_keyboard_available(false, 77, false);
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, -1);
    check(fixture.key('C') && copy.text == sample && fixture.tx.empty(), "disconnected Messages still forwards retained selection to copy");
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, 0);
    check(!fixture.key('C') && fixture.tx.empty(), "disconnected unselected Ctrl+C never queues TX");
    fixture.page.set_keyboard_available(true, 78, true);
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, 5);
    copy.busy = true;
    const auto requests = copy.requests;
    check(fixture.key('C') && fixture.tx.empty() && copy.requests == requests + 1, "failed copy sink never falls through from selected copy to UART");
    copy.busy = false;
    check(fixture.key('A') && fixture.key('C') && copy.text == sample && fixture.tx.empty(), "Ctrl+A then Ctrl+C forwards all text to copy without TX");
    options.encoding = MessageEncoding::Hex; fixture.page.configure_display(options);
    SendMessageW(fixture.page.view(), EM_SETSEL, 0, 8);
    check(fixture.key('C') && copy.text == L"00000000" && fixture.tx.empty(), "HEX selection copies displayed text without UART");
    options.encoding = MessageEncoding::Utf8; fixture.page.configure_display(options);
    fixture.page.reset();
    fixture.option(L"Light terminal display (CR / BS / erase line)");
    const auto large = std::string(120000, 'x') + message_utf8(L"copy \u6f22\u5b57");
    fixture.page.append(large); fixture.page.refresh();
    SendMessageW(fixture.page.view(), EM_SETSEL, 120000, 120007);
    check(fixture.key('C') && copy.text == L"copy \u6f22\u5b57" && fixture.tx.empty(), "selection beyond65535 characters forwards exact text to copy without UART");
    check(fixture.page.buffer().bytes() == large, "long copy preserves exact retained RX bytes");
    fixture.option(L"Light terminal display (CR / BS / erase line)");
}
}
int wmain() {
    try {
        PrivateDesktop session;
        {
            KeyboardState keys;
            check(GetKeyState(VK_CONTROL) < 0, "Ctrl state is confined to the private test thread");
            Fixture fixture; verify(fixture, true); verify(fixture, false);
        }
        session.close();
    } catch (const std::exception& error) {
        std::cout << "FAIL: " << error.what() << '\n'; return 1;
    }
    return passed ? 0 : 1;
}
