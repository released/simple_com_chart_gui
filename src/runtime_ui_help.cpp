#include "mfc_main_dialog.h"
#include "runtime_capture.h"

bool CMainDialog::verify_ui_help() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"ui-help-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    const wchar_t* names[] = {L"Messages", L"Chart", L"4 Charts"};
    HDC window = ::GetDC(m_hWnd), dc = CreateCompatibleDC(window);
    HBITMAP bitmap = CreateCompatibleBitmap(window, 160, 40); const auto old = SelectObject(dc, bitmap);
    for (int page = 0; page < 3; ++page) {
        wchar_t caption[32] = {}; TCITEMW tab{}; tab.mask = TCIF_TEXT; tab.pszText = caption; tab.cchTextMax = 32;
        TabCtrl_GetItem(tabs_, page, &tab); check("Messages / Chart / 4 Charts order", std::wstring(caption) == names[page]);
        TabCtrl_SetCurSel(tabs_, page);
        NMHDR change{tabs_, 601, TCN_SELCHANGE}; LRESULT result = 0;
        OnNotify(601, reinterpret_cast<LPARAM>(&change), &result);
        check("tab notification selects the matching page", (::IsWindowVisible(message_page_.hwnd()) != FALSE) == (page == kMessagesTab) &&
            (::IsWindowVisible(plot_view_.hwnd()) != FALSE) == (page == kChartTab) &&
            (::IsWindowVisible(data_page_.hwnd()) != FALSE) == (page == kDataTab));
        check("Save As enabled on all pages; chart controls follow named pages", ::IsWindowEnabled(btn_save_) &&
            (::IsWindowEnabled(btn_refresh_) != FALSE) == (page == kChartTab));
        for (int item = 0; item < 3; ++item) {
            DRAWITEMSTRUCT draw{}; draw.CtlType = ODT_TAB; draw.hwndItem = tabs_; draw.itemID = item;
            draw.hDC = dc; draw.rcItem = {0, 0, 160, 40};
            OnDrawItem(601, &draw);
            check("only selected tab has dark grey fill and white text", GetPixel(dc, 3, 3) ==
                (page == item ? RGB(64, 64, 64) : GetSysColor(COLOR_BTNFACE)) &&
                GetTextColor(dc) == (page == item ? RGB(255, 255, 255) : GetSysColor(COLOR_BTNTEXT)));
        }
    }
    const auto brush = reinterpret_cast<HBRUSH>(::SendMessageW(message_page_.hwnd(), WM_CTLCOLORSTATIC,
        reinterpret_cast<WPARAM>(dc), reinterpret_cast<LPARAM>(message_page_.view())));
    LOGBRUSH color{}; GetObjectW(brush, sizeof(color), &color);
    check("readonly message edit requests black background / green text", color.lbColor == RGB(0, 0, 0) &&
        GetBkColor(dc) == RGB(0, 0, 0) && GetTextColor(dc) == RGB(0, 255, 0));
    SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); ::ReleaseDC(m_hWnd, window);
    const auto tooltip = hover_help_.hwnd();
    const auto captured_rows = capture_.rows().size();
    check("tooltips hidden until requested", ::IsWindow(tooltip) && !::IsWindowVisible(tooltip));
    check("engineering-style popup has no fade or animation", (::GetWindowLongPtrW(tooltip, GWL_STYLE) &
        (TTS_NOANIMATE | TTS_NOFADE)) == (TTS_NOANIMATE | TTS_NOFADE));
    check("ten targets each have a child HWND tool and parent fallback", ::SendMessageW(tooltip, TTM_GETTOOLCOUNT, 0, 0) == 20);
    const auto tip_text = [&](HWND target) {
        wchar_t text[1024] = {}; TOOLINFOW info{TTTOOLINFOW_V1_SIZE};
        info.hwnd = m_hWnd; info.uId = reinterpret_cast<UINT_PTR>(target); info.lpszText = text;
        ::SendMessageW(tooltip, TTM_GETTEXTW, 1024, reinterpret_cast<LPARAM>(&info)); return std::wstring(text);
    };
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    for (int width : {1120, 1500}) {
        SetWindowPos(nullptr, 0, 0, width, 780, SWP_NOMOVE | SWP_NOZORDER);
        UINT_PTR area_id = 0;
        for (auto target : {btn_refresh_, btn_fit_, btn_snapshot_, btn_overlay_, label_auto_, combo_auto_,
            label_include_, edit_include_, label_exclude_, edit_exclude_}) {
            ++area_id;
            TOOLINFOW native{TTTOOLINFOW_V1_SIZE}; native.hwnd = m_hWnd; native.uId = reinterpret_cast<UINT_PTR>(target);
            check("real child HWND is registered with automatic mouse subclass", ::SendMessageW(tooltip, TTM_GETTOOLINFOW,
                0, reinterpret_cast<LPARAM>(&native)) && (native.uFlags & (TTF_IDISHWND | TTF_SUBCLASS)) == (TTF_IDISHWND | TTF_SUBCLASS));
            TOOLINFOW area{TTTOOLINFOW_V1_SIZE}; area.hwnd = m_hWnd; area.uId = area_id;
            check("parent fallback is separately registered and subclassed", ::SendMessageW(tooltip, TTM_GETTOOLINFOW,
                0, reinterpret_cast<LPARAM>(&area)) && (area.uFlags & (TTF_IDISHWND | TTF_SUBCLASS)) == TTF_SUBCLASS);
            RECT bounds{}; ::GetWindowRect(target, &bounds);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&bounds), 2);
            TTHITTESTINFOW hit{}; hit.hwnd = m_hWnd; hit.pt = {(bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2};
            hit.ti.cbSize = TTTOOLINFOW_V1_SIZE;
            check("hover hit rectangles follow resize", ::SendMessageW(tooltip, TTM_HITTESTW, 0, reinterpret_cast<LPARAM>(&hit)) &&
                (hit.ti.uId == reinterpret_cast<UINT_PTR>(target) || hit.ti.uId == area_id) && !tip_text(target).empty() &&
                EqualRect(&bounds, &area.rect));
        }
    }
    check("Refresh explains sample clearing, not log deletion", tip_text(btn_refresh_).find(L"not cleared") != std::wstring::npos);
    check("Fit explains Y range and frozen samples", tip_text(btn_fit_).find(L"Y range") != std::wstring::npos &&
        tip_text(btn_fit_).find(L"frozen samples") != std::wstring::npos);
    check("Snapshot explains Live and continuing capture", tip_text(btn_snapshot_).find(L"Live") != std::wstring::npos &&
        tip_text(btn_snapshot_).find(L"continues") != std::wstring::npos);
    check("Overlay explains markers and labels", tip_text(btn_overlay_).find(L"markers and value labels") != std::wstring::npos);
    check("Auto Refresh distinguishes timed clear from receive interval", tip_text(combo_auto_).find(L"NOT the receive/update interval") != std::wstring::npos &&
        tip_text(label_auto_) == tip_text(combo_auto_));
    check("Include / Exclude retain brief Regex-pipe examples", tip_text(edit_include_).find(L"Set Regex and use |") != std::wstring::npos &&
        tip_text(edit_include_).find(L"fan,|ac_adc_mv:") != std::wstring::npos &&
        tip_text(edit_exclude_).find(L"error|timeout") != std::wstring::npos &&
        tip_text(edit_exclude_).find(L"takes priority") != std::wstring::npos);
    check("popup delay is a hover delay, not always visible", ::SendMessageW(tooltip, TTM_GETDELAYTIME, TTDT_INITIAL, 0) == 600);
    for (auto target : {btn_refresh_, btn_fit_, btn_snapshot_, btn_overlay_, edit_include_})
        ::SendMessageW(target, WM_MOUSEMOVE, 0, MAKELPARAM(4, 4));
    ::SendMessageW(tooltip, TTM_POP, 0, 0);
    check("native child mouse routing does not execute commands or clear capture", capture_.rows().size() == captured_rows);
    // Render the native popup deterministically without moving the user's cursor.
    const auto body = tip_text(combo_auto_);
    TOOLINFOW popup{TTTOOLINFOW_V1_SIZE}; popup.hwnd = m_hWnd; popup.uId = 0xFFFF;
    popup.uFlags = TTF_TRACK | TTF_ABSOLUTE; popup.lpszText = const_cast<wchar_t*>(body.c_str());
    const bool added = ::SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&popup)) != 0;
    RECT anchor{}; ::GetWindowRect(combo_auto_, &anchor);
    ::SendMessageW(tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(anchor.left, anchor.bottom + 8));
    ::SendMessageW(tooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&popup));
    ::SendMessageW(tooltip, TTM_UPDATE, 0, 0);
    ::RedrawWindow(tooltip, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    check("native floating explanation renders", added && ::IsWindowVisible(tooltip) &&
        save_runtime_frame(tooltip, evidence / L"auto-refresh-tooltip.png"));
    ::SendMessageW(tooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&popup));
    ::SendMessageW(tooltip, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&popup));
    check("popup returns hidden and does not change target count", !::IsWindowVisible(tooltip) && ::SendMessageW(tooltip, TTM_GETTOOLCOUNT, 0, 0) == 20);
    SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    message_page_.reset(); message_page_.append("fan,duty:920,fg_hz:70.8\r\nASCII / HEX raw receive monitor\r\n");
    TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page();
    check("black-green Messages screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-green.png"));
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    check("selected Chart screenshot", save_runtime_frame(m_hWnd, evidence / L"tabs-selected.png"));
    report << (pass ? "PASS" : "FAIL") << ": named tab order, selected styling, black-green messages and hover help\n";
    return pass;
}
