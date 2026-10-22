#include "mfc_main_dialog.h"
#include "runtime_capture.h"

bool CMainDialog::verify_startup_tab() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"startup-tab-result.txt");
    bool pass = true;
    const auto check = [&](const char* name, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << name << '\n'; pass = ok && pass; };
    struct Choice { int page; UINT command; const wchar_t* token; };
    const Choice choices[] = {{kMessagesTab, ID_STARTUP_MESSAGES, L"Messages"},
        {kChartTab, ID_STARTUP_CHART, L"Chart"}, {kDataTab, ID_STARTUP_FOUR_CHARTS, L"4Charts"}};
    const auto command = std::wstring(AfxGetApp()->m_lpCmdLine);
    const int expected = command.find(L"--expected-startup=Messages") != std::wstring::npos ? kMessagesTab :
        command.find(L"--expected-startup=4Charts") != std::wstring::npos ? kDataTab :
        command.find(L"--expected-startup=Chart") != std::wstring::npos ? kChartTab : startup_tab_;
    const auto visible = [&](int page) {
        return (::IsWindowVisible(message_page_.hwnd()) != FALSE) == (page == kMessagesTab) &&
            (::IsWindowVisible(data_page_.hwnd()) != FALSE) == (page == kDataTab) &&
            (::IsWindowVisible(plot_view_.hwnd()) != FALSE) == (page == kChartTab) &&
            (::IsWindowEnabled(btn_refresh_) != FALSE) == (page == kChartTab) && ::IsWindowEnabled(btn_save_);
    };
    const auto radio = [&] {
        const auto settings = ::GetSubMenu(::GetMenu(m_hWnd), 0);
        const auto menu = settings ? ::GetSubMenu(settings, 0) : nullptr;
        if (!::IsMenu(menu)) return false;
        for (const auto& choice : choices) {
            const auto state = ::GetMenuState(menu, choice.command, MF_BYCOMMAND);
            if (state == static_cast<UINT>(-1) || ((state & MF_CHECKED) != 0) != (choice.page == startup_tab_)) return false;
        }
        return true;
    };
    check("actual process startup restores expected INI tab, focus and page controls", startup_tab_ == expected &&
        TabCtrl_GetCurSel(tabs_) == expected && TabCtrl_GetCurFocus(tabs_) == expected && visible(expected));
    check("startup menu has exactly the confirmed radio choice", radio());
    check("startup preference never opens hardware", !serial_mgr_->is_connected() && !serial_open_.busy());
    ShowWindow(SW_RESTORE); SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    check("startup page screenshot", save_runtime_frame(m_hWnd, evidence / L"startup-page.png"));

    const auto original_path = settings_path_; const auto original_choice = startup_tab_;
    settings_path_ = (evidence / L"startup-tab-test.ini").wstring();
    OnCommand(ID_STARTUP_CHART, 0); // Also exercises new-file/BOM creation on the first run.
    { std::ifstream file(std::filesystem::path(settings_path_), std::ios::binary);
      check("new preference INI uses UTF-16 BOM", file.get() == 0xff && file.get() == 0xfe); }
    WritePrivateProfileStringW(L"Preservation", L"Sentinel", L"keep", settings_path_.c_str());
    const auto read = [&] {
        wchar_t value[64] = {}; GetPrivateProfileStringW(L"UI", L"StartupTab", L"", value, 64, settings_path_.c_str());
        return std::wstring(value);
    };
    const auto rows = capture_.rows().size(); const auto raw = message_page_.buffer().bytes();
    const auto active = TabCtrl_GetCurSel(tabs_);
    for (const auto& choice : choices) {
        OnCommand(choice.command, 0);
        check("menu command immediately persists named token and updates radio", read() == choice.token && startup_tab_ == choice.page && radio());
        check("choosing a startup page does not change current page or capture", TabCtrl_GetCurSel(tabs_) == active && visible(active) &&
            capture_.rows().size() == rows && message_page_.buffer().bytes() == raw);
        load_startup_tab(); apply_startup_tab();
        check("production startup loader restores each choice and visibility", TabCtrl_GetCurSel(tabs_) == choice.page && visible(choice.page) && radio());
        TabCtrl_SetCurSel(tabs_, active); switch_page();
    }
    const auto stored = read();
    for (const auto& choice : choices) {
        TabCtrl_SetCurSel(tabs_, choice.page);
        NMHDR change{tabs_, 601, TCN_SELCHANGE}; LRESULT result = 0;
        OnNotify(601, reinterpret_cast<LPARAM>(&change), &result);
        check("ordinary tab switching preserves the explicit startup preference", visible(choice.page) && read() == stored);
    }
    for (const auto value : {L"", L"invalid", L"99", L"-1"}) {
        WritePrivateProfileStringW(L"UI", L"StartupTab", value[0] ? value : nullptr, settings_path_.c_str());
        load_startup_tab(); apply_startup_tab();
        check("missing/invalid INI value safely falls back to Chart", startup_tab_ == kChartTab && visible(kChartTab) && radio());
    }
    OnCommand(ID_STARTUP_CHART, 0);
    WritePrivateProfileStringW(nullptr, nullptr, nullptr, settings_path_.c_str());
    const auto attributes = GetFileAttributesW(settings_path_.c_str());
    check("read-only fixture prepared", attributes != INVALID_FILE_ATTRIBUTES &&
        SetFileAttributesW(settings_path_.c_str(), attributes | FILE_ATTRIBUTE_READONLY));
    OnCommand(ID_STARTUP_MESSAGES, 0);
    check("failed save retains previous choice and reports the problem", startup_tab_ == kChartTab && read() == L"Chart" && radio() &&
        flash_active_ && left_flash_.find(L"Cannot save startup tab") != std::wstring::npos);
    check("read-only fixture attributes restored", SetFileAttributesW(settings_path_.c_str(), attributes) != FALSE);
    wchar_t sentinel[32] = {}; GetPrivateProfileStringW(L"Preservation", L"Sentinel", L"", sentinel, 32, settings_path_.c_str());
    check("unrelated INI data/capture/raw bytes preserved", std::wstring(sentinel) == L"keep" &&
        capture_.rows().size() == rows && message_page_.buffer().bytes() == raw);
    settings_path_ = original_path; startup_tab_ = original_choice; apply_startup_tab();
    report << (pass ? "PASS" : "FAIL") << ": startup-tab options, actual startup/INI restore, menu persistence, fallback and failed-write isolation\n";
    return pass;
}
