#include "mfc_main_dialog.h"

#include <filesystem>
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
    pass = pass && Header_GetItemCount(ListView_GetHeader(data_page_.table())) == static_cast<int>(capture_.keys().size() + 2);
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
        << ": MFC startup, labels, tabs, frozen headers, rows/retention, auto-scroll, independent charts/time axes, minimized capture, Refresh preservation, filters, INI, async Save As, close guard\n";
    smoke_exit_code_ = pass ? 0 : 1;
    PostMessage(WM_CLOSE);
}
