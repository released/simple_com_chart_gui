#include "mfc_main_dialog.h"
#include "capture_filename.h"
#include "runtime_capture.h"
#include "ui_metrics.h"

bool CMainDialog::verify_chart_preferences() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"chart-preferences-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    const auto original_path = settings_path_;
    settings_path_ = (evidence / L"chart-preferences-test.ini").wstring();
    const auto none = [&] {
        bool ok = true;
        for (size_t i = 0; i < 4; ++i) ok = data_page_.selected_field(i).empty() &&
            data_page_.chart_model(i).get_keys().empty() &&
            ::SendMessageW(::GetDlgItem(data_page_.hwnd(), static_cast<int>(10 + i)), CB_GETCURSEL, 0, 0) == 0 && ok;
        return ok;
    };
    const auto receive = [&](const std::string& line, double time) {
        { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.lines.push_back({line, time}); }
        flush_pending_lines();
    };
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    data_page_.set_selected_fields({}); apply_filter(false); save_settings(); load_settings();
    TabCtrl_SetCurSel(tabs_, kDataTab); switch_page();
    wchar_t caption[32] = {}; TCITEMW tab{}; tab.mask = TCIF_TEXT; tab.pszText = caption; tab.cchTextMax = 32;
    TabCtrl_GetItem(tabs_, kDataTab, &tab); check("short tab caption", std::wstring(caption) == L"4 Charts");
    check("no saved selections means four none", none());
    receive("duty:920,fg_hz:70.8,fg_rpm:212,temp:-1234567890.125", 30000000);
    check("discovery never automatically assigns plots", none());
    const auto choose = [&](int chart, const wchar_t* key) {
        const auto selector = ::GetDlgItem(data_page_.hwnd(), 10 + chart);
        const auto index = ::SendMessageW(selector, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(key));
        ::SendMessageW(selector, CB_SETCURSEL, index, 0);
        ::SendMessageW(data_page_.hwnd(), WM_COMMAND, MAKEWPARAM(10 + chart, CBN_SELCHANGE), reinterpret_cast<LPARAM>(selector));
    };
    choose(0, L"fg_hz"); choose(1, L"fg_rpm"); choose(3, L"duty");
    wchar_t persisted[160] = {};
    GetPrivateProfileStringW(L"FourCharts", L"Chart1", L"missing", persisted, 160, settings_path_.c_str());
    check("actual user selector immediately writes INI", std::wstring(persisted) == L"key:fg_hz");
    const auto table = data_page_.table();
    int total_width = 0;
    for (int column = 0; column < Header_GetItemCount(ListView_GetHeader(table)); ++column) {
        LVCOLUMNW info{}; info.mask = LVCF_FMT | LVCF_WIDTH;
        ListView_GetColumn(table, column, &info); total_width += info.cx;
        check("table columns explicitly left-aligned", (info.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT);
    }
    RECT box{}; ::GetClientRect(table, &box);
    check("content-sized columns do not stretch across empty viewport", total_width < box.right - 80);
    wchar_t numeric[80] = {}; swprintf_s(numeric, L"%.10g", -1234567890.125);
    HDC dc = ::GetDC(table); const auto font = reinterpret_cast<HFONT>(::SendMessageW(table, WM_GETFONT, 0, 0));
    const auto old = SelectObject(dc, font); SIZE extent{};
    GetTextExtentPoint32W(dc, numeric, static_cast<int>(wcslen(numeric)), &extent);
    SelectObject(dc, old); ::ReleaseDC(table, dc);
    check("wide numeric values receive enough measured width", ListView_GetColumnWidth(table, 5) >= extent.cx + 24);
    check("compact left-aligned table screenshot", save_runtime_frame(m_hWnd, evidence / L"data-auto-columns.png"));
    data_page_.set_selected_fields({"other", "", "", ""});
    load_settings();
    check("INI restore preserves selected names and unused none slot", data_page_.selected_field(0) == "fg_hz" &&
        data_page_.selected_field(1) == "fg_rpm" && data_page_.selected_field(2).empty() && data_page_.selected_field(3) == "duty");
    check("restored names wait safely before discovery", capture_.rows().empty() && data_page_.chart_model(0).get_keys().empty());
    receive("other:999", 30000001);
    check("unrelated new field does not replace remembered selection", data_page_.selected_field(0) == "fg_hz" && data_page_.chart_model(0).get_keys().empty());
    receive("fg_hz:69.1,fg_rpm:2074,duty:920", 30000002);
    const auto hz = data_page_.chart_model(0).get_series("fg_hz"), rpm = data_page_.chart_model(1).get_series("fg_rpm");
    check("saved plots resume when their fields arrive", !hz.empty() && hz.back().v == 69.1 &&
        !rpm.empty() && rpm.back().v == 2074 && data_page_.selected_field(2).empty());
    check("Apply preserves saved selections while clearing old samples", apply_filter(false) &&
        data_page_.selected_field(0) == "fg_hz" && data_page_.chart_model(0).get_series("fg_hz").empty());
    ::SetWindowTextW(edit_limit_, L"1"); apply_filter(false);
    receive("a:1,fg_hz:999", 30000002);
    check("remembered names cannot bypass Max CH", data_page_.selected_field(0) == "fg_hz" &&
        data_page_.chart_model(0).get_series("fg_hz").empty() && capture_.keys().size() == 1);
    ::SetWindowTextW(edit_limit_, L"256"); apply_filter(false);
    const std::string unicode = u8"溫度";
    data_page_.select_field(2, unicode); save_settings(); data_page_.select_field(2, ""); load_settings();
    check("Unicode field name round-trips through portable INI", data_page_.selected_field(2) == unicode);
    data_page_.select_field(2, "\"quoted\""); save_settings(); data_page_.select_field(2, ""); load_settings();
    check("quoted field name round-trips without profile quote stripping", data_page_.selected_field(2) == "\"quoted\"");
    ::SendMessageW(::GetDlgItem(data_page_.hwnd(), 4), BM_CLICK, 0, 0);
    check("Reset to none clears all four plots", none());
    load_settings(); check("reset is persisted for next launch", none());
    receive("fg_hz:71.0,fg_rpm:2100,duty:920", 30000003);
    check("reset plots stay none after new data", none());
    const auto reset_button = ::GetDlgItem(data_page_.hwnd(), 4);
    ::GetClientRect(reset_button, &box);
    check("Reset caption fully measured", box.right >= control_text_width(reset_button, 20));
    SYSTEMTIME time{}; time.wYear = 2026; time.wMonth = 1; time.wDay = 2; time.wHour = 3; time.wMinute = 4; time.wSecond = 5;
    check("Save As filename includes local date and second, zero-padded", capture_filename(time) == L"20260102_030405.log");
    settings_path_ = original_path;
    load_serial_settings();
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L""); apply_filter(false);
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": persisted plot selections, reset, table widths and timestamp filename\n";
    return pass;
}
