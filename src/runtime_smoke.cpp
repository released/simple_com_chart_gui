#include "mfc_main_dialog.h"

#include <filesystem>
#include <cmath>
#include "ui_metrics.h"

namespace {
bool save_frame(HWND hwnd, const std::filesystem::path& path) {
    RECT rc{}; ::GetWindowRect(hwnd, &rc);
    HDC dc = ::GetWindowDC(hwnd), memory = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, rc.right - rc.left, rc.bottom - rc.top);
    const auto old = SelectObject(memory, bitmap);
    const bool painted = PrintWindow(hwnd, memory, 2) != FALSE;
    SelectObject(memory, old);
    CLSID encoder{}; UINT count = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&count, &size);
    std::vector<unsigned char> buffer(size);
    const auto encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    Gdiplus::GetImageEncoders(count, size, encoders);
    bool saved = false;
    for (UINT i = 0; i < count; ++i) if (wcscmp(encoders[i].MimeType, L"image/png") == 0) {
        encoder = encoders[i].Clsid;
        Gdiplus::Bitmap image(bitmap, nullptr);
        saved = image.Save(path.c_str(), &encoder) == Gdiplus::Ok; break;
    }
    DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(hwnd, dc);
    return painted && saved;
}
}

bool CMainDialog::verify_y_axis_autoscale() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"y-axis-result.txt");
    bool pass = true;
    const auto clock = [] {
        LARGE_INTEGER frequency{}, counter{};
        QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&counter);
        return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
    };
    const auto check = [&](const char* name, bool ok) {
        const auto axis = plot_view_.y_range();
        ok = ok && std::isfinite(axis.first) && std::isfinite(axis.second) && axis.first < axis.second;
        report << (ok ? "PASS: " : "FAIL: ") << name << " [" << axis.first << ", " << axis.second << "]\n";
        pass = ok && pass;
    };
    const auto apply = [&](const wchar_t* include) {
        ::SetWindowTextW(edit_include_, include); ::SetWindowTextW(edit_exclude_, L"");
        ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
        check("valid Apply", apply_filter(false));
    };
    const auto receive = [&](const std::string& line, double timestamp) {
        { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.lines.push_back({line, timestamp}); }
        flush_pending_lines();
    };
    const auto near_201 = [&] {
        const auto axis = plot_view_.y_range();
        return axis.first > 190 && axis.first <= 201 && axis.second >= 201 && axis.second < 210;
    };
    const bool original_overlay = overlay_enabled_;
    plot_view_.set_overlay_enabled(false);
    model_.set_time_window(30); plot_view_.set_time_window(30);
    apply(L""); receive("fan,value:1000", clock());
    auto axis = plot_view_.y_range();
    check("1000 exceeds old initial limit", axis.first > 900 && axis.first <= 1000 && axis.second > 1000);
    apply(L"fan,");
    check("Apply resets old bounds", plot_view_.y_range() == std::make_pair(0.0, 50.0));
    receive("ac,value:1000", clock()); receive("fan,value:200", clock()); receive("fan,value:201", clock());
    check("filtered 1000 is excluded, 200/201 auto-fit", near_201() && model_.get_series("value").size() == 2);
    const auto filtered_axis = plot_view_.y_range();
    ::SetWindowTextW(edit_include_, L"["); ::SendMessageW(combo_filter_, CB_SETCURSEL, 1, 0);
    check("invalid Apply preserves bounds", !apply_filter(false) && plot_view_.y_range() == filtered_axis);
    apply(L"fan,"); receive("fan,value:200", clock()); receive("fan,value:201", clock());
    // Capture a real filtered chart with the normal end-value overlay enabled.
    plot_view_.set_overlay_enabled(true); plot_view_.update_from_model(clock());
    const auto overlay_axis = plot_view_.y_range();
    for (int i = 0; i < 20; ++i) plot_view_.update_from_model(clock());
    check("overlay padding does not accumulate", plot_view_.y_range() == overlay_axis && near_201());
    check("filtered chart capture", save_frame(m_hWnd, evidence / L"y-axis-201.png"));
    plot_view_.set_overlay_enabled(false);

    apply(L""); receive("low:201,high:1000", clock());
    axis = plot_view_.y_range(); check("combined selected channels", axis.first <= 201 && axis.second >= 1000);
    const auto list = ::FindWindowExW(channel_panel_.hwnd(), nullptr, WC_LISTVIEWW, nullptr);
    bool found_high = false;
    for (int row = 0; list && row < ListView_GetItemCount(list); ++row) {
        wchar_t key[160] = {}; ListView_GetItemText(list, row, 0, key, 160);
        if (std::wstring(key) == L"high") { ListView_SetCheckState(list, row, FALSE); found_high = true; }
    }
    SendMessage(WM_APP + 1);
    check("unchecking 1000 shrinks to 201", found_high && !model_.is_enabled("high") && near_201());

    apply(L"");
    const double now = clock();
    receive("value:1000", now - 20); receive("value:201", now);
    axis = plot_view_.y_range(); check("30-second window includes old peak", axis.second >= 1000);
    ::SendMessageW(combo_time_, CB_SETCURSEL, 0, 0);
    SendMessage(WM_COMMAND, MAKEWPARAM(206, CBN_SELCHANGE), reinterpret_cast<LPARAM>(combo_time_));
    check("5-second window excludes old peak", model_.get_time_window() == 5 && near_201());
    const auto snapshot_axis = plot_view_.y_range();
    OnSnapshotClicked(); receive("value:2000", clock());
    check("Snapshot freezes Y axis", snapshot_ && plot_view_.y_range() == snapshot_axis);
    SendMessage(WM_COMMAND, MAKEWPARAM(202, BN_CLICKED));
    check("Fit uses frozen samples, not live 2000", plot_view_.y_range() == snapshot_axis);
    OnSnapshotClicked(); axis = plot_view_.y_range();
    check("Live resumes auto scaling", !snapshot_ && axis.second >= 2000);

    apply(L""); receive("value:-0.0025", clock()); receive("value:-0.0015", clock());
    axis = plot_view_.y_range();
    check("negative decimals without artificial zero", axis.first <= -.0025 && axis.second >= -.0015 && axis.second < 0 && axis.second - axis.first < .01);
    SendMessage(WM_COMMAND, MAKEWPARAM(201, BN_CLICKED));
    check("Refresh clears old bounds", plot_view_.y_range() == std::make_pair(0.0, 50.0));
    receive("value:0", clock()); axis = plot_view_.y_range();
    check("constant zero has finite nonzero span", axis.first < 0 && axis.second > 0 && axis.second - axis.first < .01);
    for (int row = 0; list && row < ListView_GetItemCount(list); ++row) ListView_SetCheckState(list, row, FALSE);
    SendMessage(WM_APP + 1);
    check("no selected data uses empty fallback", plot_view_.y_range() == std::make_pair(0.0, 50.0));
    // Restore a clean capture and the normal toolbar state for the remaining smoke tests.
    ::SendMessageW(combo_time_, CB_SETCURSEL, 2, 0);
    SendMessage(WM_COMMAND, MAKEWPARAM(206, CBN_SELCHANGE), reinterpret_cast<LPARAM>(combo_time_));
    apply(L""); plot_view_.set_overlay_enabled(original_overlay);
    report << (pass ? "PASS" : "FAIL") << ": automatic Y-axis regression\n";
    return pass;
}

bool CMainDialog::verify_data_display() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"data-display-result.txt");
    bool pass = true;
    const auto check = [&](const char* name, bool ok) {
        report << (ok ? "PASS: " : "FAIL: ") << name << '\n'; pass = ok && pass;
    };
    const auto paint = [&] {
        RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    };
    const auto receive = [&](const std::string& raw, double timestamp) {
        { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.lines.push_back({raw, timestamp}); }
        flush_pending_lines(); paint();
    };
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    check("fresh display fixture", apply_filter(false));
    TabCtrl_SetCurSel(tabs_, 1); switch_page();
    receive("a:1,b:2,c:3,d:4,e:5,z:1000,offset:-0.0025", 20000000);
    const auto table = data_page_.table();
    const auto panel = ::FindWindowExW(data_page_.hwnd(), nullptr, L"ChannelPanelWnd", nullptr);
    const auto fields = ::FindWindowExW(panel, nullptr, WC_LISTVIEWW, nullptr);
    check("all discovered fields selectable, only first four shown by default", fields &&
        ListView_GetItemCount(fields) == 7 && Header_GetItemCount(ListView_GetHeader(table)) == 6);
    const auto select_all = [&](bool all) {
        const auto button = ::GetDlgItem(panel, all ? 1 : 2);
        ::SendMessageW(button, BM_CLICK, 0, 0); paint();
    };
    select_all(true);
    check("All displays all fields", Header_GetItemCount(ListView_GetHeader(table)) == 9);
    select_all(false);
    check("None retains Row and Time only", Header_GetItemCount(ListView_GetHeader(table)) == 2);
    for (int i = 0; fields && i < ListView_GetItemCount(fields); ++i) {
        wchar_t key[160] = {}; ListView_GetItemText(fields, i, 0, key, 160);
        if (std::wstring(key) == L"e" || std::wstring(key) == L"z" || std::wstring(key) == L"offset")
            ListView_SetCheckState(fields, i, TRUE);
    }
    paint();
    check("arbitrary three columns, not the first three", Header_GetItemCount(ListView_GetHeader(table)) == 5);
    const auto cells_match = [&](int row, bool sparse) {
        bool correct = true;
        for (int column = 2; column < 5; ++column) {
            wchar_t title[160] = {}, value[80] = {};
            LVCOLUMNW col{}; col.mask = LVCF_TEXT; col.pszText = title; col.cchTextMax = 160;
            ListView_GetColumn(table, column, &col);
            NMLVDISPINFOW info{}; info.hdr.hwndFrom = table; info.hdr.code = LVN_GETDISPINFOW;
            info.item.mask = LVIF_TEXT; info.item.iItem = row; info.item.iSubItem = column;
            info.item.pszText = value; info.item.cchTextMax = 80;
            ::SendMessageW(data_page_.hwnd(), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&info));
            const std::wstring key(title), actual(value);
            const auto expected = key == L"z" ? (sparse ? L"1001" : L"1000") :
                sparse ? L"" : key == L"e" ? L"5" : L"-0.0025";
            correct = actual == expected && correct;
        }
        return correct;
    };
    check("visible columns map to original capture indices", cells_match(0, false));
    receive("z:1001", 20000001);
    check("sparse selected cells stay blank", cells_match(1, true));
    const auto snapshot = capture_.snapshot();
    check("hidden fields preserved in full capture/export snapshot", snapshot.keys.size() == 7 &&
        snapshot.rows.size() == 2 && snapshot.rows[0]->cells.size() == 7);
    SCROLLINFO scroll{sizeof(scroll), SIF_ALL}; ::GetScrollInfo(table, SB_HORZ, &scroll);
    check("three chosen columns fit without horizontal scrolling", scroll.nMax < static_cast<int>(scroll.nPage) || scroll.nMax == 0);
    data_page_.select_field(0, "e"); data_page_.select_field(1, "z");
    data_page_.select_field(2, "offset"); data_page_.select_field(3, "d"); paint();
    const auto labels_match = [&](bool names) {
        bool correct = true;
        for (size_t i = 0; i < 4; ++i) {
            const auto& plot = data_page_.chart_plot(i);
            const auto& labels = plot.end_labels();
            RECT client{}; ::GetClientRect(plot.hwnd(), &client);
            correct = labels.size() == 1 && correct;
            if (labels.size() != 1) continue;
            const auto series = data_page_.chart_model(i).get_series(data_page_.selected_field(i));
            const auto& label = labels.front();
            const bool has_name = label.text.find(L": ") != std::wstring::npos;
            wchar_t number[64] = {}; swprintf_s(number, L"%.10g", label.value);
            const std::wstring value_text(number);
            correct = has_name == names && !series.empty() && label.value == series.back().v &&
                label.text.size() >= value_text.size() && label.text.compare(label.text.size() - value_text.size(), value_text.size(), value_text) == 0 &&
                label.bounds.left >= 0 && label.bounds.right <= client.right && label.bounds.top >= 0 &&
                label.bounds.bottom <= client.bottom && label.point.x >= 0 && label.point.x <= client.right &&
                label.point.y >= 0 && label.point.y <= client.bottom && correct;
        }
        return correct;
    };
    check("default Show name and singleton latest-point labels on all four charts",
        ::SendMessageW(check_show_name_, BM_GETCHECK, 0, 0) == BST_CHECKED && labels_match(true));
    receive("e:6.25,z:1002,offset:-0.00125,d:4.5", 20000002);
    check("latest values update on every receive", labels_match(true) && data_page_.chart_plot(1).end_labels()[0].value == 1002);
    check("small plot Y axes remain independent", data_page_.chart_plot(1).y_range().second >= 1002 &&
        data_page_.chart_plot(2).y_range().second < 0 && data_page_.chart_plot(0).y_range().second < 10);
    check("named four-chart screenshot", save_frame(m_hWnd, evidence / L"data-selected-names.png"));
    ::SendMessageW(check_show_name_, BM_CLICK, 0, 0); paint();
    check("Show name off retains numeric labels in all four plots", labels_match(false));
    check("numeric-only screenshot", save_frame(m_hWnd, evidence / L"data-selected-values.png"));
    TabCtrl_SetCurSel(tabs_, 0); switch_page(); paint();
    const auto& numeric = plot_view_.end_labels();
    check("same name option applies to first tab", !numeric.empty() && std::all_of(numeric.begin(), numeric.end(),
        [](const PlotView::EndLabel& item) { return item.text.find(L": ") == std::wstring::npos; }));
    ::SendMessageW(check_show_name_, BM_CLICK, 0, 0); paint();
    const auto& named = plot_view_.end_labels();
    check("first tab name:value restored", !named.empty() && std::all_of(named.begin(), named.end(),
        [](const PlotView::EndLabel& item) { return item.text.find(L": ") != std::wstring::npos; }));
    TabCtrl_SetCurSel(tabs_, 1); switch_page(); paint();
    check("name setting survives tab switch", labels_match(true));
    const std::string long_key = "long_" + std::string(123, 'x');
    receive(long_key + ":-1234567890.125", 20000003);
    data_page_.select_field(3, long_key); paint();
    check("new fields do not silently expand selected table", Header_GetItemCount(ListView_GetHeader(table)) == 5);
    const auto& long_labels = data_page_.chart_plot(3).end_labels();
    check("long name shortened but latest number remains intact", long_labels.size() == 1 &&
        long_labels[0].text.find(L"...") != std::wstring::npos &&
        long_labels[0].text.find(L"-1234567890") != std::wstring::npos && labels_match(true));
    check("long-name screenshot", save_frame(m_hWnd, evidence / L"data-long-name.png"));
    select_all(false);
    check("hiding all table fields does not change chart selection or labels", Header_GetItemCount(ListView_GetHeader(table)) == 2 && labels_match(true));
    data_page_.select_field(0, ""); paint();
    check("none chart has no stale marker or label", data_page_.chart_plot(0).end_labels().empty());
    data_page_.refresh(20000124); paint();
    bool expired = true;
    for (size_t i = 0; i < 4; ++i) expired = data_page_.chart_plot(i).end_labels().empty() && expired;
    check("expired data has no stale latest labels", expired);
    ::SetWindowTextW(edit_limit_, L"2");
    check("two-field limit fixture", apply_filter(false));
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        for (int i = 0; i < 100; ++i)
            pending_.lines.push_back({"field_" + std::to_string(i) + ":123", 20000200.0 + i});
    }
    flush_pending_lines();
    check("latest-value cache respects channel limit despite rejected fields", capture_.keys().size() == 2 &&
        data_page_.cached_field_count() == 2 && ListView_GetItemCount(fields) == 2);
    ::SetWindowTextW(edit_limit_, L"256");
    check("reset fixture", apply_filter(false));
    check("Apply resets selected table columns", Header_GetItemCount(ListView_GetHeader(table)) == 2);
    TabCtrl_SetCurSel(tabs_, 0); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": selected columns and latest labels\n";
    return pass;
}

bool CMainDialog::verify_filter_guidance() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"filter-guidance-result.txt");
    bool pass = true;
    const auto check = [&](const char* name, bool ok) {
        report << (ok ? "PASS: " : "FAIL: ") << name << '\n'; pass = ok && pass;
    };
    const auto caption = [](HWND control) {
        wchar_t text[1024] = {}; ::GetWindowTextW(control, text, 1024); return std::wstring(text);
    };
    ::SetWindowTextW(edit_include_, L"fan,|ac_adc_mv:"); ::SetWindowTextW(edit_exclude_, L"error|timeout");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 1, 0);
    check("apply documented Regex examples", apply_filter(false));
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        for (const auto* line : {"fan,speed:201", "ac_adc_mv:1169", "fan,error,speed:999",
            "ac_adc_mv:1,timeout", "other:999", "fan,Error,speed:202"})
            pending_.lines.push_back({line, 21000000});
    }
    flush_pending_lines();
    check("OR include keeps whole matching lines; OR exclude wins; matching is case-sensitive",
        capture_.rows().size() == 3 && model_.get_series("speed").size() == 2 &&
        model_.get_series("speed").back().v == 202 && model_.get_series("other").empty());
    check("status, Include example and Exclude example are separate controls",
        caption(label_filter_).find(L"Filter applied (Regex, case-sensitive). New capture started.") == 0 &&
        caption(label_filter_include_) == L"Include: Set Regex and use | to display data. Example: fan,|ac_adc_mv:" &&
        caption(label_filter_exclude_) == L"Exclude: Set Regex and use | to exclude data. Example: error|timeout");
    check("guidance layout across widths and tabs", verify_layout_after_repaint());
    check("guidance screenshot", save_frame(m_hWnd, evidence / L"filter-guidance.png"));
    ::SetWindowTextW(edit_include_, L"[");
    check("invalid Regex keeps capture and both help paragraphs", !apply_filter(false) && capture_.rows().size() == 3 &&
        caption(label_filter_).find(L"Previous filter remains active.") != std::wstring::npos &&
        ::IsWindowVisible(label_filter_include_) && ::IsWindowVisible(label_filter_exclude_));
    ::SetWindowTextW(edit_limit_, L"0");
    check("invalid limit keeps capture and both help paragraphs", !apply_filter(false) && capture_.rows().size() == 3 &&
        caption(label_filter_).find(L"Max CH must be") == 0);
    ::SetWindowTextW(edit_limit_, L"256"); ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    check("restore Text mode", apply_filter(false) && caption(label_filter_).find(L"(Text, case-sensitive)") != std::wstring::npos);
    report << (pass ? "PASS" : "FAIL") << ": multi-string filter guidance\n";
    return pass;
}

bool CMainDialog::verify_layout_after_repaint() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"layout-result.txt");
    bool pass = true;
    for (const int width : {1120, 1180, 1500}) {
        SetWindowPos(nullptr, 0, 0, width, 780, SWP_NOMOVE | SWP_NOZORDER);
        for (int page = 0; page < 2; ++page) {
            TabCtrl_SetCurSel(tabs_, page); switch_page();
            for (int repaint = 0; repaint < 3; ++repaint)
                RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            LOGFONTW ui{}, buttons{};
            const bool live_fonts = ui_font_ && btn_font_ &&
                GetObjectW(ui_font_, sizeof(ui), &ui) == sizeof(ui) &&
                GetObjectW(btn_font_, sizeof(buttons), &buttons) == sizeof(buttons);
            pass = live_fonts && pass;
            report << "width=" << width << " page=" << page << " live-fonts=" << live_fonts << '\n';
            RECT parent{}; GetClientRect(&parent);
            for (HWND item : {label_auto_, label_time_, btn_save_, label_include_, label_exclude_, btn_apply_, check_show_name_}) {
                const auto font = reinterpret_cast<HFONT>(::SendMessageW(item, WM_GETFONT, 0, 0));
                LOGFONTW actual{};
                const bool valid_font = font == ui_font_ && GetObjectW(font, sizeof(actual), &actual) == sizeof(actual);
                wchar_t text[160] = {}; ::GetWindowTextW(item, text, 160);
                HDC dc = ::GetDC(item);
                const auto old = SelectObject(dc, font);
                RECT rendered{};
                DrawTextW(dc, text, -1, &rendered, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                if (old && old != HGDI_ERROR) SelectObject(dc, old);
                ::ReleaseDC(item, dc);
                RECT bounds{}; ::GetClientRect(item, &bounds);
                const int padding = item == check_show_name_ ? GetSystemMetrics(SM_CXMENUCHECK) + 16 :
                    item == btn_save_ || item == btn_apply_ ? 20 : 8;
                RECT position{}; ::GetWindowRect(item, &position);
                ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&position), 2);
                const bool fits = valid_font && bounds.right >= rendered.right + padding &&
                    bounds.bottom >= rendered.bottom + 4 && position.left >= 0 && position.right <= parent.right;
                pass = fits && pass;
                report << "  control=" << ::GetDlgCtrlID(item) << " fits=" << fits << " font=" << valid_font
                    << " box=" << bounds.right << 'x' << bounds.bottom << " text=" << rendered.right << 'x' << rendered.bottom << '\n';
            }
            RECT auto_label{}, auto_combo{}, time_label{}, time_combo{};
            ::GetWindowRect(label_auto_, &auto_label); ::GetWindowRect(combo_auto_, &auto_combo);
            ::GetWindowRect(label_time_, &time_label); ::GetWindowRect(combo_time_, &time_combo);
            pass = auto_combo.left >= auto_label.right + 8 && time_combo.left >= time_label.right + 8 && pass;
            RECT previous{}; ::GetWindowRect(btn_apply_, &previous);
            for (auto help : {label_filter_, label_filter_include_, label_filter_exclude_}) {
                RECT position{}, box{}; ::GetWindowRect(help, &position); ::GetClientRect(help, &box);
                const bool fits = position.top >= previous.bottom && box.bottom >= control_wrapped_text_height(help, box.right) &&
                    (::GetWindowLongPtrW(help, GWL_STYLE) & SS_TYPEMASK) == SS_LEFT;
                report << "  filter-help-fits=" << fits << " height=" << box.bottom << '\n';
                pass = fits && pass; previous = position;
            }
            RECT tab_position{}; ::GetWindowRect(tabs_, &tab_position);
            pass = tab_position.top >= previous.bottom + 4 && pass;
        }
    }
    report << (pass ? "PASS" : "FAIL") << ": live UI fonts and full text after repeated resize/tab/repaint\n";
    SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    TabCtrl_SetCurSel(tabs_, 0); switch_page();
    return pass;
}

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

    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    ShowWindow(SW_RESTORE);
    pass = verify_layout_after_repaint() && pass;
    SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    CRect include_rect, edit_rect;
    ::GetWindowRect(label_include_, &include_rect); ::GetWindowRect(edit_include_, &edit_rect);
    pass = pass && include_rect.Width() >= control_text_width(label_include_) && edit_rect.left >= include_rect.right + 8;
    ::GetWindowRect(btn_save_, &control); GetClientRect(&client);
    ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&control), 2);
    pass = pass && control.right <= client.right;
    pass = pass && save_frame(m_hWnd, evidence / L"chart-page.png");
    TabCtrl_SetCurSel(tabs_, 1); switch_page();
    pass = pass && ::IsWindowVisible(data_page_.hwnd()) && !::IsWindowVisible(plot_view_.hwnd()) && data_page_.follows();
    pass = pass && ListView_GetItemCount(data_page_.table()) == static_cast<int>(capture_.rows().size());
    pass = pass && Header_GetItemCount(ListView_GetHeader(data_page_.table())) == static_cast<int>(std::min<size_t>(4, capture_.keys().size()) + 2);
    data_page_.select_field(0, "fg_hz"); data_page_.select_field(1, "fg_rpm");
    data_page_.select_field(2, "offset"); data_page_.select_field(3, "ntc_mv");
    model_.set_enabled("offset", false);
    pass = pass && !data_page_.chart_model(2).get_series("offset").empty() && data_page_.chart_model(2).get_keys().size() == 1;
    pass = pass && data_page_.selected_field(0) == "fg_hz" && data_page_.selected_field(1) == "fg_rpm";
    const auto time_combo = ::GetDlgItem(data_page_.hwnd(), 20);
    ::SendMessageW(time_combo, CB_SETCURSEL, 4, 0);
    ::SendMessageW(data_page_.hwnd(), WM_COMMAND, MAKEWPARAM(20, CBN_SELCHANGE), reinterpret_cast<LPARAM>(time_combo));
    pass = pass && data_page_.chart_time(0) == 120 && data_page_.chart_time(1) == 30;
    ::SendMessageW(time_combo, CB_SETCURSEL, 2, 0);
    ::SendMessageW(data_page_.hwnd(), WM_COMMAND, MAKEWPARAM(20, CBN_SELCHANGE), reinterpret_cast<LPARAM>(time_combo));
    pass = pass && save_frame(m_hWnd, evidence / L"data-page.png");
    const auto table = data_page_.table();
    for (int i = 0; i < 200; ++i) {
        const double ts = 10000000.0 + i;
        capture_.append("scroll:" + std::to_string(i), {{"scroll", static_cast<double>(i)}}, ts);
        data_page_.append({{"scroll", static_cast<double>(i)}}, ts);
    }
    data_page_.refresh(10000200);
    const int bottom = ListView_GetTopIndex(table);
    pass = pass && bottom > 100;
    RECT frozen_header{}; ::GetWindowRect(ListView_GetHeader(table), &frozen_header);
    data_page_.set_follow(false);
    ListView_EnsureVisible(table, 0, FALSE);
    const int anchor = ListView_GetTopIndex(table);
    const auto field_panel = ::FindWindowExW(data_page_.hwnd(), nullptr, L"ChannelPanelWnd", nullptr);
    ::SendMessageW(::GetDlgItem(field_panel, 2), BM_CLICK, 0, 0);
    pass = pass && ListView_GetTopIndex(table) == anchor;
    capture_.append("scroll:201", {{"scroll", 201}}, 10000201);
    data_page_.append({{"scroll", 201}}, 10000201); data_page_.refresh(10000201);
    pass = pass && ListView_GetTopIndex(table) == anchor;
    RECT current_header{}; ::GetWindowRect(ListView_GetHeader(table), &current_header);
    pass = pass && EqualRect(&frozen_header, &current_header);
    const auto anchor_sequence = capture_.rows()[anchor]->sequence;
    // Force the live table through its retention boundary, preserving the nearest retained row.
    for (int i = 0; i < 10000; ++i) capture_.append("retention", {}, 10000202 + i);
    data_page_.refresh(10010202);
    pass = pass && capture_.dropped() > 0 && ListView_GetTopIndex(table) == 0 &&
        capture_.rows().front()->sequence > anchor_sequence;
    data_page_.set_follow(true); pass = pass && ListView_GetTopIndex(table) > 100;
    data_page_.set_follow(false);
    ListView_EnsureVisible(table, 200, FALSE);
    const auto kept_anchor = capture_.rows()[ListView_GetTopIndex(table)]->sequence;
    for (int i = 0; i < 10; ++i) capture_.append("more", {}, 10010203 + i);
    data_page_.refresh(10010213);
    pass = pass && capture_.rows()[ListView_GetTopIndex(table)]->sequence == kept_anchor;
    data_page_.set_follow(true);
    const auto retained = capture_.rows().size();
    OnCommand(MAKEWPARAM(201, BN_CLICKED), 0);
    pass = pass && capture_.rows().size() == retained;
    TabCtrl_SetCurSel(tabs_, 0); switch_page();
    pass = pass && ::IsWindowVisible(plot_view_.hwnd()) && !::IsWindowVisible(data_page_.hwnd());
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_.lines.push_back({"minimized:42", 10010214});
    }
    is_minimized_ = true; OnTimer(2); is_minimized_ = false;
    pass = pass && capture_.rows().back()->raw == "minimized:42";

    ::SetWindowTextW(edit_include_, L"fan,");
    ::SetWindowTextW(edit_exclude_, L"");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    pass = apply_filter() && pass;
    demo_tick_ = 3;
    feed_demo();
    flush_pending_lines();
    pass = pass && !model_.get_series("fg_hz").empty() && model_.get_series("ac_adc_mv").empty() && capture_.rows().size() == 1;

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

    pass = verify_y_axis_autoscale() && pass;
    pass = verify_data_display() && pass;
    pass = verify_filter_guidance() && pass;
    demo_tick_ = 3; feed_demo(); flush_pending_lines();
    smoke_checks_passed_ = pass; smoke_export_pending_ = true;
    if (!begin_capture_export(evidence / L"runtime-capture.log", CaptureFormat::Text)) {
        smoke_export_pending_ = false; finish_smoke_test(false); return;
    }
    OnCancel(); // Closing during Save As must not destroy the worker's owner.
    smoke_checks_passed_ = smoke_checks_passed_ && exporting_ && ::IsWindow(m_hWnd);
}

void CMainDialog::finish_smoke_test(bool pass) {
    const auto report = std::filesystem::path(settings_path_).parent_path() / L"smoke-result.txt";
    std::ofstream output(report);
    output << (pass ? "PASS" : "FAIL")
        << ": MFC startup, labels, tabs, frozen headers, rows/retention, auto-scroll, independent charts/time axes, automatic Y ranges, minimized capture, Refresh preservation, filters, INI, async Save As, close guard\n";
    smoke_exit_code_ = pass ? 0 : 1;
    PostMessage(WM_CLOSE);
}
