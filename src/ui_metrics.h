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

inline int control_wrapped_text_height(HWND control, int width) {
    if (!IsWindow(control)) return 0;
    std::wstring text(static_cast<size_t>(GetWindowTextLengthW(control)) + 1, L'\0');
    GetWindowTextW(control, text.data(), static_cast<int>(text.size()));
    HDC dc = GetDC(control);
    if (!dc) return 0;
    const auto font = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    const auto old = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
    RECT bounds{0, 0, std::max(1, width), 0};
    DrawTextW(dc, text.c_str(), -1, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    if (old && old != HGDI_ERROR) SelectObject(dc, old);
    ReleaseDC(control, dc);
    return bounds.bottom + 4;
}
