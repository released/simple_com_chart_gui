#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "ui_metrics.h"

bool CMainDialog::verify_messages() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"messages-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    const auto text = [&] {
        std::wstring value(static_cast<size_t>(::GetWindowTextLengthW(message_page_.view())) + 1, 0);
        ::GetWindowTextW(message_page_.view(), value.data(), static_cast<int>(value.size())); value.pop_back(); return value;
    };
    const auto receive = [&](const std::string& raw, bool line = false) {
        { std::lock_guard<std::mutex> lock(pending_mutex_);
          pending_.raw.append(raw); if (line) pending_.lines.push_back({raw, 40000000}); }
        flush_pending_lines();
    };
    const auto mode = [&](int index) {
        ::SendMessageW(message_page_.mode_control(), CB_SETCURSEL, index, 0);
        ::SendMessageW(message_page_.hwnd(), WM_COMMAND, MAKEWPARAM(1, CBN_SELCHANGE), 0);
    };
    ::SetWindowTextW(edit_include_, L"fan,"); ::SetWindowTextW(edit_exclude_, L"error");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0); check("filter fixture", apply_filter(false));
    message_page_.reset();
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    const std::string binary("A\0\xFF\r", 4);
    receive(binary);
    check("hidden raw capture accepts bytes without newline, no fabricated numeric rows",
        message_page_.buffer().bytes() == binary && capture_.rows().empty());
    TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page();
    check("Messages tab visible, Save As enabled, chart controls inactive", TabCtrl_GetItemCount(tabs_) == 3 &&
        ::IsWindowVisible(message_page_.hwnd()) && !::IsWindowVisible(data_page_.hwnd()) && !::IsWindowVisible(plot_view_.hwnd()) &&
        ::IsWindowEnabled(btn_save_) && !::IsWindowEnabled(btn_snapshot_));
    check("default ASCII renders exact byte escapes", text() == L"A\\x00\\xFF\r\n");
    receive("\n"); check("ASCII incremental split CRLF matches full rendering", text() == message_page_.buffer().ascii());
    receive("fan,value:201\r\n", true); receive("other,value:999\r\n", true); receive("fan,error,value:9\r\n", true);
    check("raw monitor is independent of Include/Exclude", capture_.rows().size() == 1 &&
        text().find(L"other,value:999") != std::wstring::npos && text().find(L"fan,error,value:9") != std::wstring::npos);
    check("valid Apply preserves raw monitor", apply_filter(false) && !message_page_.buffer().bytes().empty());
    check("ASCII screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-ascii.png"));
    const auto raw = message_page_.buffer().bytes();
    mode(1); check("HEX uses original bytes including NUL FF CR LF", text() == message_page_.buffer().hex() &&
        text().find(L"41 00 FF 0D 0A") != std::wstring::npos && raw == message_page_.buffer().bytes());
    for (int i = 0; i < 34; ++i) {
        receive("x"); check("HEX partial rows append identically to full formatting", text() == message_page_.buffer().hex());
    }
    check("HEX screenshot", save_runtime_frame(m_hWnd, evidence / L"messages-hex.png"));
    mode(0); check("switching back preserves entire raw history", text() == message_page_.buffer().ascii());
    std::string lines; for (int i = 0; i < 180; ++i) lines += "message " + std::to_string(i) + "\r\n";
    receive(lines);
    ::SendMessageW(message_page_.follow_control(), BM_CLICK, 0, 0);
    ::SendMessageW(message_page_.view(), EM_SETSEL, 0, 0); ::SendMessageW(message_page_.view(), EM_SCROLLCARET, 0, 0);
    ::SendMessageW(message_page_.view(), EM_LINESCROLL, 0, 12);
    const auto top = ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0);
    receive("newest\r\n");
    check("auto-scroll off preserves inspected line", ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0) == top);
    ::SendMessageW(message_page_.follow_control(), BM_CLICK, 0, 0);
    check("auto-scroll on follows newest", ::SendMessageW(message_page_.view(), EM_GETFIRSTVISIBLELINE, 0, 0) > 100);
    receive("fan,value:205", true); const auto rows = capture_.rows().size();
    ::SendMessageW(message_page_.clear_control(), BM_CLICK, 0, 0);
    check("Clear affects only raw messages, not captured chart/data", text().empty() && message_page_.buffer().bytes().empty() &&
        capture_.rows().size() == rows && !model_.get_series("value").empty());
    { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.raw.append("tail"); }
    disconnect(); check("disconnect retains final queued bytes", text() == L"tail");
    const bool original_demo = demo_; demo_ = false;
    { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.raw.append(" while minimized"); }
    is_minimized_ = true; OnTimer(2); is_minimized_ = false; demo_ = original_demo;
    check("raw updates continue while minimized", text() == L"tail while minimized");
    receive(std::string(MessageBuffer::kCapacity * 2, 'x'));
    check("UI pending and retained buffers stay bounded after an oversized burst",
        message_page_.buffer().bytes().size() == MessageBuffer::kCapacity &&
        message_page_.buffer().skipped() == MessageBuffer::kCapacity && text().size() == MessageBuffer::kCapacity);
    message_page_.append("after gap", 10); message_page_.refresh();
    check("input-loss gap does not concatenate across missing bytes", text() == L"after gap" &&
        message_page_.buffer().skipped() == MessageBuffer::kCapacity + 10);
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L""); apply_filter(false);
    message_page_.reset();
    for (int page = 0; page < 3; ++page) {
        TabCtrl_SetCurSel(tabs_, page); switch_page();
        RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    const auto font = reinterpret_cast<HFONT>(::SendMessageW(message_page_.view(), WM_GETFONT, 0, 0)); LOGFONTW lf{};
    check("message font remains live after tab/repaint", font && GetObjectW(font, sizeof(lf), &lf) == sizeof(lf));
    for (auto control : {message_page_.follow_control(), message_page_.clear_control()}) {
        RECT bounds{}; ::GetClientRect(control, &bounds);
        const int padding = control == message_page_.follow_control() ? GetSystemMetrics(SM_CXMENUCHECK) + 16 : 20;
        check("message captions measured with full indicator/padding", bounds.right >= control_text_width(control, padding));
    }
    mode(0); TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": raw Messages ASCII/HEX regression\n";
    return pass;
}
