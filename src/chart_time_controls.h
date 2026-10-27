#pragma once
#include "chart_time.h"
#include <windows.h>

namespace chart_time {
inline void show(HWND combo, double seconds) {
    const auto text = label(seconds);
    const auto index = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str()));
    if (index != CB_ERR) SendMessageW(combo, CB_SETCURSEL, index, 0);
    else SetWindowTextW(combo, text.c_str());
}

inline void populate(HWND combo) {
    SendMessageW(combo, CB_LIMITTEXT, 48, 0);
    for (const auto value : {0.25, 0.5}) {
        const auto text = label(value);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    for (int value = 1; value <= 300; ++value) {
        const auto text = label(value);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    show(combo, 30);
}

inline bool read(HWND combo, bool selection, double& seconds) {
    wchar_t text[64]{};
    if (selection) {
        const auto index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
        if (index == CB_ERR || SendMessageW(combo, CB_GETLBTEXTLEN, index, 0) >= 64) return false;
        SendMessageW(combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(text));
    } else GetWindowTextW(combo, text, 64);
    return parse(text, seconds);
}

inline bool owns(HWND combo, HWND target) {
    return combo == target || IsChild(combo, target);
}

inline int control_width(HWND combo) {
    HDC dc=GetDC(combo);
    const auto font=reinterpret_cast<HFONT>(SendMessageW(combo,WM_GETFONT,0,0));
    const auto old=SelectObject(dc,font?font:GetStockObject(DEFAULT_GUI_FONT));
    SIZE size{};GetTextExtentPoint32W(dc,L"250 ms",6,&size);
    SelectObject(dc,old);ReleaseDC(combo,dc);
    return size.cx+40;
}
}
