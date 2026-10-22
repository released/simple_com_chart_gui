#include "hover_help.h"
#include <commctrl.h>

bool HoverHelp::create(HWND parent, HFONT font) {
    parent_ = parent;
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_NOANIMATE | TTS_NOFADE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hwnd_) return false;
    SendMessageW(hwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    SendMessageW(hwnd_, TTM_SETMAXTIPWIDTH, 0, 460);
    SendMessageW(hwnd_, TTM_SETDELAYTIME, TTDT_INITIAL, 600);
    SendMessageW(hwnd_, TTM_SETDELAYTIME, TTDT_RESHOW, 100);
    SendMessageW(hwnd_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 12000);
    return true;
}

void HoverHelp::add(HWND control, const wchar_t* text) {
    if (!IsWindow(hwnd_) || !IsWindow(control)) return;
    // Match MFC's compatible TOOLINFO prefix; this app need not enable themed v6 controls.
    TOOLINFOW tool{TTTOOLINFOW_V1_SIZE};
    tool.hwnd = parent_; tool.uId = reinterpret_cast<UINT_PTR>(control);
    // The tooltip must see the real child HWND under the cursor, especially BUTTONs.
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
    tool.lpszText = const_cast<wchar_t*>(text);
    if (!SendMessageW(hwnd_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool))) return;
    TOOLINFOW area = tool;
    area.uId = controls_.size() + 1;
    area.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT;
    GetWindowRect(control, &area.rect);
    MapWindowPoints(nullptr, parent_, reinterpret_cast<POINT*>(&area.rect), 2);
    // Transparent STATICs and disabled children deliver mouse input to their parent.
    if (!SendMessageW(hwnd_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&area))) {
        SendMessageW(hwnd_, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        return;
    }
    controls_.push_back(control);
}

void HoverHelp::layout() {
    if (!IsWindow(hwnd_)) return;
    for (size_t index = 0; index < controls_.size(); ++index) {
        TOOLINFOW tool{TTTOOLINFOW_V1_SIZE}; tool.hwnd = parent_; tool.uId = index + 1;
        GetWindowRect(controls_[index], &tool.rect);
        MapWindowPoints(nullptr, parent_, reinterpret_cast<POINT*>(&tool.rect), 2);
        SendMessageW(hwnd_, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&tool));
    }
}

void HoverHelp::destroy() {
    if (IsWindow(hwnd_)) DestroyWindow(hwnd_);
    hwnd_ = parent_ = nullptr; controls_.clear();
}
