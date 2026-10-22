#include "mfc_main_dialog.h"
#include "button_feedback.h"
#include "runtime_capture.h"

bool CMainDialog::verify_button_theme() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"button-theme-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page(); hover_btn_ = nullptr;
    HDC window = ::GetDC(m_hWnd), dc = CreateCompatibleDC(window);
    HBITMAP bitmap = CreateCompatibleBitmap(window, 160, 40); const auto old = SelectObject(dc, bitmap);
    const auto background = [&](HWND control, UINT state) {
        DRAWITEMSTRUCT item{}; item.CtlType = ODT_BUTTON; item.CtlID = ::GetDlgCtrlID(control); item.hwndItem = control;
        item.hDC = dc; item.itemState = state; item.rcItem = {0, 0, 160, 40};
        draw_owner_button(&item); return GetPixel(dc, 3, 3);
    };
    for (auto control : {btn_refresh_, btn_fit_, btn_snapshot_, btn_overlay_, btn_connect_}) {
        check("idle button matches UI grey", background(control, 0) == GetSysColor(COLOR_BTNFACE));
        check("pressed button uses single red accent", background(control, ODS_SELECTED) == kButtonActiveRed);
        check("disabled overrides hover/pressed to grey", background(control, ODS_DISABLED | ODS_SELECTED | ODS_HOTLIGHT) == GetSysColor(COLOR_BTNFACE));
        {
            ButtonFeedback feedback(control);
            check("executing command remains red after mouse release", background(control, 0) == kButtonActiveRed);
        }
        check("completion returns to UI grey", background(control, 0) == GetSysColor(COLOR_BTNFACE) && !::GetPropW(control, kButtonBusyProperty));
    }
    check("connected palette is red until disconnect", button_colors(false, true, false, false).background == kButtonActiveRed);
    const bool original_snapshot = snapshot_, original_overlay = overlay_enabled_;
    OnCommand(MAKEWPARAM(203, BN_CLICKED), 0);
    check("Snapshot still toggles, without persistent accent", snapshot_ != original_snapshot &&
        background(btn_snapshot_, 0) == GetSysColor(COLOR_BTNFACE));
    OnCommand(MAKEWPARAM(203, BN_CLICKED), 0);
    OnCommand(MAKEWPARAM(204, BN_CLICKED), 0);
    check("Overlay still toggles, without persistent accent", overlay_enabled_ != original_overlay &&
        background(btn_overlay_, 0) == GetSysColor(COLOR_BTNFACE));
    OnCommand(MAKEWPARAM(204, BN_CLICKED), 0);
    HWND temporary = ::CreateWindowW(L"BUTTON", L"fixture", WS_CHILD, 0, 0, 10, 10, m_hWnd, nullptr, nullptr, nullptr);
    check("lifecycle fixture created", temporary != nullptr);
    { ButtonFeedback feedback(temporary); ::DestroyWindow(temporary); }
    check("feedback cleanup tolerates source destruction", !::IsWindow(temporary));
    SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); ::ReleaseDC(m_hWnd, window);
    demo_tick_ = 3; feed_demo(); flush_pending_lines();
    check("neutral toolbar screenshot", save_runtime_frame(m_hWnd, evidence / L"button-theme.png"));
    report << (pass ? "PASS" : "FAIL") << ": neutral toolbar, temporary command accent and connection palette\n";
    return pass;
}
