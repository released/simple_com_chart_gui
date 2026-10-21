#include "mfc_main_dialog.h"

#include <filesystem>

void CMainDialog::run_smoke_test() {
    smoke_test_ = false;
    bool pass = ::IsWindow(plot_view_.hwnd()) && ::IsWindow(channel_panel_.hwnd());
    pass = pass && model_.get_keys().size() > 16;
    const auto list = ::FindWindowExW(channel_panel_.hwnd(), nullptr, WC_LISTVIEWW, nullptr);
    pass = pass && list && ListView_GetItemCount(list) > 16;
    bool saw_signed_label = false, saw_float_label = false, saw_hex_label = false;
    if (list) {
        for (int row = 0; row < ListView_GetItemCount(list); ++row) {
            wchar_t key[160] = {}, value[80] = {};
            ListView_GetItemText(list, row, 0, key, 160);
            ListView_GetItemText(list, row, 1, value, 80);
            if (std::wstring(key) == L"offset") saw_signed_label = value[0] == '-';
            if (std::wstring(key) == L"fg_hz") saw_float_label = std::wstring(value).find('.') != std::wstring::npos;
            if (std::wstring(key) == L"cmd") saw_hex_label = std::wstring(value) == L"12802";
        }
    }
    pass = pass && saw_signed_label && saw_float_label && saw_hex_label;
    const auto offset = model_.get_series("offset");
    const auto frequency = model_.get_series("fg_hz");
    pass = pass && !offset.empty() && offset.back().v < 0 && !frequency.empty() &&
        frequency.back().v > 60 && frequency.back().v < 80;
    RECT client{}, control{};
    GetClientRect(&client);
    ::GetWindowRect(btn_apply_, &control);
    ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&control), 2);
    pass = pass && control.left >= 0 && control.right <= client.right;

    ::SetWindowTextW(edit_include_, L"fan,");
    ::SetWindowTextW(edit_exclude_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    pass = apply_filter() && pass;
    demo_tick_ = 3;
    feed_demo();
    flush_pending_lines();
    pass = pass && !model_.get_series("fg_hz").empty() && model_.get_series("ac_adc_mv").empty();

    ::SetWindowTextW(edit_include_, L"[");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 1, 0);
    pass = !apply_filter(false) && pass;
    pass = pass && !model_.get_series("fg_hz").empty();

    ::SetWindowTextW(edit_include_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    pass = apply_filter() && pass;
    wchar_t persisted[32] = {};
    GetPrivateProfileStringW(L"Capture", L"MaxChannels", L"", persisted, 32, settings_path_.c_str());
    pass = pass && std::wstring(persisted) == L"256";

    const auto report = std::filesystem::path(settings_path_).parent_path() / L"smoke-result.txt";
    std::ofstream output(report);
    output << (pass ? "PASS" : "FAIL")
        << ": MFC startup, controls, >16 channels, signed floats, Apply, invalid regex recovery, portable INI\n";
    smoke_exit_code_ = pass ? 0 : 1;
    PostMessage(WM_CLOSE);
}
