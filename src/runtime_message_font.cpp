#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "ui_metrics.h"

bool CMainDialog::verify_message_font() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"message-font-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    const auto original_path = settings_path_;
    const int original_points = message_page_.font_points();
    settings_path_ = (evidence / L"message-font-test.ini").wstring();
    WritePrivateProfileStringW(L"Messages", L"FontSizePt", nullptr, settings_path_.c_str());
    WritePrivateProfileStringW(L"Sentinel", L"Keep", L"unchanged", settings_path_.c_str());
    const auto saved_points = [&] { return GetPrivateProfileIntW(L"Messages", L"FontSizePt", 0, settings_path_.c_str()); };
    const auto height = [&] {
        LOGFONTW font{}; const auto handle = reinterpret_cast<HFONT>(::SendMessageW(message_page_.view(), WM_GETFONT, 0, 0));
        return GetObjectW(handle, sizeof(font), &font) == sizeof(font) ? std::abs(font.lfHeight) : 0;
    };
    const auto mode = [&](int value) {
        ::SendMessageW(message_page_.mode_control(), CB_SETCURSEL, value, 0);
        ::SendMessageW(message_page_.hwnd(), WM_COMMAND, MAKEWPARAM(1, CBN_SELCHANGE), 0);
    };
    load_settings();
    check("missing font preference restores default 10 pt", message_page_.font_points() == MessagePage::kDefaultFontPoints);
    message_page_.reset(); TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page(); mode(0);
    std::string raw;
    for (int i = 0; i < 180; ++i) raw += "fan,rpm:" + std::to_string(2000 + i) + "\r\n";
    message_page_.append(raw); message_page_.refresh();
    const auto snapshot = message_page_.export_text();
    capture_.append("font-check:201", {{"font-check", 201}}, 60000000);
    const auto rows = capture_.rows().size();
    const auto other_font = ::SendMessageW(edit_include_, WM_GETFONT, 0, 0);
    const auto button_font = ::SendMessageW(message_page_.font_up_control(), WM_GETFONT, 0, 0);
    ::SendMessageW(message_page_.follow_control(), BM_CLICK, 0, 0);
    ::SendMessageW(message_page_.view(), EM_SETSEL, 0, 0); ::SendMessageW(message_page_.view(), EM_SCROLLCARET, 0, 0);
    ::SendMessageW(message_page_.view(), EM_LINESCROLL, 0, 12);
    const auto first = ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0);
    const int initial_height = height();
    ::SendMessageW(message_page_.font_up_control(), BM_CLICK, 0, 0);
    check("plus click increases actual font by one point and persists immediately", message_page_.font_points() == 11 && height() > initial_height && saved_points() == 11);
    check("font resize preserves inspected row with follow off", ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0) == first);
    check("font resize keeps bytes, export text and captured chart rows", message_page_.buffer().bytes() == raw && message_page_.export_text() == snapshot && capture_.rows().size() == rows);
    check("only message body font changes", ::SendMessageW(edit_include_, WM_GETFONT, 0, 0) == other_font &&
        ::SendMessageW(message_page_.font_up_control(), WM_GETFONT, 0, 0) == button_font);
    ::SendMessageW(message_page_.font_down_control(), BM_CLICK, 0, 0);
    check("minus click reduces actual font and saves immediately", message_page_.font_points() == 10 && height() == initial_height && saved_points() == 10);
    message_page_.set_font_points(22); load_settings();
    check("actual INI load routine restores saved size", message_page_.font_points() == 10 && height() == initial_height && message_page_.buffer().bytes() == raw);
    wchar_t sentinel[32] = {};
    GetPrivateProfileStringW(L"Sentinel", L"Keep", L"", sentinel, 32, settings_path_.c_str());
    check("saving font preserves unrelated INI keys", std::wstring(sentinel) == L"unchanged");
    const auto ascii_font = ::SendMessageW(message_page_.view(), WM_GETFONT, 0, 0);
    mode(1); check("ASCII and HEX share the same selected font", ::SendMessageW(message_page_.view(), WM_GETFONT, 0, 0) == ascii_font && message_page_.buffer().bytes() == raw);
    message_page_.set_font_points(1000);
    check("upper bound is 32 pt and plus is disabled", message_page_.font_points() == 32 && !::IsWindowEnabled(message_page_.font_up_control()) && ::IsWindowEnabled(message_page_.font_down_control()));
    message_page_.set_font_points(-10);
    check("lower bound is 8 pt and minus is disabled", message_page_.font_points() == 8 && !::IsWindowEnabled(message_page_.font_down_control()) && ::IsWindowEnabled(message_page_.font_up_control()));
    WritePrivateProfileStringW(L"Messages", L"FontSizePt", L"1000", settings_path_.c_str()); load_settings();
    check("oversized INI font is safely clamped", message_page_.font_points() == 32);
    WritePrivateProfileStringW(L"Messages", L"FontSizePt", L"invalid", settings_path_.c_str()); load_settings();
    check("malformed INI font cannot create an invalid size", message_page_.font_points() >= 8 && message_page_.font_points() <= 32 && height() > 0);
    const auto resources = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    for (int repeat = 0; repeat < 24; ++repeat) {
        message_page_.set_font_points(12); message_page_.set_font_points(24);
    }
    check("repeated font replacement does not accumulate GDI objects", GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= resources + 2 && height() > 0);
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    check("font can restore while Messages is hidden", message_page_.set_font_points(18) && message_page_.font_points() == 18);
    TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page();
    ::SendMessageW(message_page_.follow_control(), BM_CLICK, 0, 0);
    message_page_.append("newest\r\n"); message_page_.refresh();
    ::SendMessageW(message_page_.font_up_control(), BM_CLICK, 0, 0);
    check("font change with follow on still shows newest rows", ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0) > 100 && saved_points() == 19);
    message_page_.reset(); message_page_.append("fan,duty:920,fg_rpm:2074\r\nASCII / HEX message font\r\n"); message_page_.refresh();
    message_page_.set_font_points(24); mode(0);
    check("large ASCII font screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-font-large-ascii.png"));
    mode(1); check("large HEX font screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-font-large-hex.png"));
    for (int width : {1120, 1500}) {
        SetWindowPos(nullptr, 0, 0, width, 780, SWP_NOMOVE | SWP_NOZORDER);
        RECT previous{};
        for (auto control : {message_page_.clear_control(), message_page_.font_down_control(), message_page_.font_up_control()}) {
            RECT bounds{}, client{}; ::GetWindowRect(control, &bounds); ::GetClientRect(control, &client);
            check("font buttons remain measured and non-overlapping after resize", bounds.left >= previous.right && client.right >= control_text_width(control, 20));
            previous = bounds;
        }
        for (int points : {8, 10, 32}) {
            message_page_.set_font_points(points);
            RECT field{}, page{}; ::GetWindowRect(message_page_.mode_control(), &field); ::GetWindowRect(message_page_.hwnd(), &page);
            check("toolbar uses the closed combo field, not dropdown height", field.bottom - field.top <= control_font_height(message_page_.mode_control()) + 12);
            int controls = 0;
            for (auto control = ::GetWindow(message_page_.hwnd(), GW_CHILD); control; control = ::GetWindow(control, GW_HWNDNEXT)) {
                wchar_t caption[32] = {}; ::GetWindowTextW(control, caption, 32);
                const std::wstring text = caption;
                const bool label = text == L"Display:" || text == L"Font:" || text == std::to_wstring(points) + L" pt";
                if (!label && control != message_page_.mode_control() && control != message_page_.follow_control() &&
                    control != message_page_.clear_control() && control != message_page_.font_down_control() && control != message_page_.font_up_control()) continue;
                RECT bounds{}; ::GetWindowRect(control, &bounds); ++controls;
                check("all Messages toolbar controls share top, bottom and center line", bounds.top == field.top && bounds.bottom == field.bottom &&
                    bounds.left >= page.left && bounds.right <= page.right);
                if (label) check("toolbar labels and point size are vertically centered", (::GetWindowLongPtrW(control, GWL_STYLE) & SS_CENTERIMAGE) != 0);
            }
            check("alignment covers labels, combo, checkbox and all three buttons", controls == 8);
        }
    }
    message_page_.set_font_points(10); mode(0);
    check("aligned toolbar screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-toolbar-aligned.png"));
    SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    settings_path_ = original_path;
    load_serial_settings();
    message_page_.set_font_points(original_points); mode(0); message_page_.reset();
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": Messages font buttons, INI restore, scroll/data isolation and font lifetime\n";
    return pass;
}
