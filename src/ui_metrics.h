#pragma once
#include <windows.h>
#include <algorithm>
#include <string>

inline int control_text_width(HWND control, int padding = 8) {
    wchar_t text[512] = {};
    GetWindowTextW(control, text, 512);
    HDC dc = GetDC(control);
    auto font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    const auto old = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
    SIZE extent{}; GetTextExtentPoint32W(dc, text, static_cast<int>(wcslen(text)), &extent);
    SelectObject(dc, old); ReleaseDC(control, dc);
    return extent.cx + padding;
}

inline int control_font_height(HWND control) {
    HDC dc = GetDC(control);
    auto font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    const auto old = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
    TEXTMETRICW metrics{}; GetTextMetricsW(dc, &metrics);
    SelectObject(dc, old); ReleaseDC(control, dc); return metrics.tmHeight;
}
