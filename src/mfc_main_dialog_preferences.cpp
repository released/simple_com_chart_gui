#include "mfc_main_dialog.h"

void CMainDialog::update_language_menu(){
    const auto root=::GetMenu(m_hWnd);if(!::IsMenu(root))return;
    ::CheckMenuRadioItem(root,ID_LANGUAGE_TW,ID_LANGUAGE_EN,ui::saved_language(settings_path_)=="en"?ID_LANGUAGE_EN:ID_LANGUAGE_TW,MF_BYCOMMAND);
}
void CMainDialog::choose_language(const std::string& code){
    if(!ui::save_language(settings_path_,code)){show_status_message(ui::tr(L"Cannot save language. Use a writable EXE folder."),6000);return;}
    update_language_menu();show_status_message(ui::tr(code=="en"?L"English selected. Restart the tool to apply; current connection and data are unchanged.":L"Traditional Chinese selected. Restart the tool to apply; current connection and data are unchanged."),10000);
}

void CMainDialog::load_startup_tab() {
    wchar_t value[64] = {};
    GetPrivateProfileStringW(L"UI", L"StartupTab", L"Chart", value, 64, settings_path_.c_str());
    startup_tab_ = std::wstring(value) == L"Messages" ? kMessagesTab : std::wstring(value) == L"4Charts" ? kDataTab : kChartTab;
    update_startup_menu();
}

void CMainDialog::update_startup_menu() {
    if (!::IsWindow(m_hWnd)) return;
    const auto root = ::GetMenu(m_hWnd);
    if (!::IsMenu(root)) return;
    const auto settings = ::GetSubMenu(root, 0);
    const auto menu = settings ? ::GetSubMenu(settings, 0) : nullptr;
    if (!::IsMenu(menu)) return; // The INI is read before the menu is created.
    const auto selected = startup_tab_ == kMessagesTab ? ID_STARTUP_MESSAGES :
        startup_tab_ == kDataTab ? ID_STARTUP_FOUR_CHARTS : ID_STARTUP_CHART;
    ::CheckMenuRadioItem(menu, ID_STARTUP_MESSAGES, ID_STARTUP_FOUR_CHARTS, selected, MF_BYCOMMAND);
    ::DrawMenuBar(m_hWnd);
}

void CMainDialog::apply_startup_tab() {
    if (!::IsWindow(tabs_)) return;
    TabCtrl_SetCurSel(tabs_, startup_tab_);
    TabCtrl_SetCurFocus(tabs_, startup_tab_);
    update_startup_menu();
    switch_page();
}

bool CMainDialog::save_startup_tab(int page) {
    if (settings_path_.empty()) return false;
    if (GetFileAttributesW(settings_path_.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // CREATE_NEW cannot truncate an existing INI if another writer creates it first.
        HANDLE file = CreateFileW(settings_path_.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            const unsigned char bom[] = {0xff, 0xfe}; DWORD written = 0;
            const bool ok = WriteFile(file, bom, sizeof(bom), &written, nullptr) && written == sizeof(bom);
            CloseHandle(file);
            if (!ok) return false;
        } else {
            const auto error = GetLastError();
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) return false;
        }
    }
    const auto value = page == kMessagesTab ? L"Messages" : page == kDataTab ? L"4Charts" : L"Chart";
    return WritePrivateProfileStringW(L"UI", L"StartupTab", value, settings_path_.c_str()) != FALSE;
}

void CMainDialog::choose_startup_tab(int page) {
    if (page != kMessagesTab && page != kChartTab && page != kDataTab) return;
    if (!save_startup_tab(page)) {
        show_status_message(L"Cannot save startup tab to INI. Use a writable EXE folder.", 5000);
        return; // Keep the previously confirmed choice/radio check on failure.
    }
    startup_tab_ = page; update_startup_menu();
    const auto name = page == kMessagesTab ? L"Messages" : page == kDataTab ? L"4 Charts" : L"Chart";
    show_status_message(std::wstring(L"Startup tab: ") + name + L" (next launch).", 5000);
}
