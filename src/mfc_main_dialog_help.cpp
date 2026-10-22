#include "mfc_main_dialog.h"

void CMainDialog::build_hover_help(HFONT font) {
    if (!hover_help_.create(m_hWnd, font)) return;
    hover_help_.add(btn_refresh_, L"Chart: clear rolling waveform samples, keeping channel choices.\nCaptured log/table and Messages are not cleared.");
    hover_help_.add(btn_fit_, L"Chart: recalculate the Y range for enabled signals in the current time window.\nSnapshot uses frozen samples.");
    hover_help_.add(btn_snapshot_, L"Freeze Chart and its axes; the button changes to Live.\nClick Live to resume. Data capture continues in the background.");
    hover_help_.add(btn_overlay_, L"Chart: show or hide latest-point markers and value labels.\nCaptured data is unchanged.");
    const auto auto_help = L"Automatically clear Chart waveform samples every selected N seconds.\nOff disables timed clearing; this is NOT the receive/update interval.\nCaptured log/table and Messages are kept.";
    hover_help_.add(label_auto_, auto_help); hover_help_.add(combo_auto_, auto_help);
    const auto include = L"Set Regex and use | to display data.\nExample: fan,|ac_adc_mv:\nText: one case-sensitive substring. Empty accepts all.";
    const auto exclude = L"Set Regex and use | to exclude data.\nExample: error|timeout\nExclude takes priority; empty excludes nothing.";
    hover_help_.add(label_include_, include); hover_help_.add(edit_include_, include);
    hover_help_.add(label_exclude_, exclude); hover_help_.add(edit_exclude_, exclude);
}

void CMainDialog::size_tabs(int height) {
    if (!::IsWindow(tabs_)) return;
    HDC dc = ::GetDC(tabs_); const auto old = SelectObject(dc, ui_font_ ? ui_font_ : GetStockObject(DEFAULT_GUI_FONT));
    int width = 0;
    for (const auto* name : {L"Messages", L"Chart", L"4 Charts"}) {
        SIZE text{}; GetTextExtentPoint32W(dc, name, static_cast<int>(wcslen(name)), &text);
        width = std::max(width, static_cast<int>(text.cx) + 32);
    }
    SelectObject(dc, old); ::ReleaseDC(tabs_, dc);
    TabCtrl_SetItemSize(tabs_, width, height);
}

void CMainDialog::draw_tab(const DRAWITEMSTRUCT* item) {
    const bool selected = static_cast<int>(item->itemID) == TabCtrl_GetCurSel(tabs_);
    const auto background = selected ? RGB(64, 64, 64) : GetSysColor(COLOR_BTNFACE);
    const auto brush = CreateSolidBrush(background);
    FillRect(item->hDC, &item->rcItem, brush); DeleteObject(brush);
    FrameRect(item->hDC, &item->rcItem, GetSysColorBrush(COLOR_BTNSHADOW));
    if (selected) {
        RECT underline = item->rcItem; underline.top = underline.bottom - 3;
        FillRect(item->hDC, &underline, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    }
    wchar_t text[32] = {}; TCITEMW tab{}; tab.mask = TCIF_TEXT; tab.pszText = text; tab.cchTextMax = 32;
    TabCtrl_GetItem(tabs_, item->itemID, &tab);
    const auto old = SelectObject(item->hDC, ui_font_ ? ui_font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(item->hDC, TRANSPARENT); SetTextColor(item->hDC, selected ? RGB(255, 255, 255) : GetSysColor(COLOR_BTNTEXT));
    RECT bounds = item->rcItem;
    DrawTextW(item->hDC, text, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(item->hDC, old);
    if ((item->itemState & ODS_FOCUS) && ::GetFocus() == tabs_) {
        InflateRect(&bounds, -4, -4); DrawFocusRect(item->hDC, &bounds);
    }
}
