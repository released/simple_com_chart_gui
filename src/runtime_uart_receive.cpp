#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "runtime_serial_fixture.h"
#include <richedit.h>

bool MessagePage::verify_receive_controls(const std::wstring& folder) {
    std::ofstream report(std::filesystem::path(folder) / L"uart-display-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = pass && ok; };
    const auto visible_text = [&] { std::wstring value(static_cast<size_t>(GetWindowTextLengthW(view_)) + 1, 0); GetWindowTextW(view_, value.data(), static_cast<int>(value.size())); value.pop_back(); return value; };
    MessageDisplayOptions display; display.encoding = MessageEncoding::Utf8; configure_display(display);
    reset(); append("\xE4\xB8"); refresh(); check("UTF-8 incomplete prefix is pending", visible_text().empty());
    append("\xAD\xE6\x96\x87\r\nERROR value:201\r\nwarn value:1000"); refresh();
    check("UTF-8 display/export and raw preservation", visible_text().find(L"中文") != std::wstring::npos && export_text().find("\xE4\xB8\xAD") != std::string::npos && buffer_.bytes().find("ERROR") != std::string::npos);
    options_command(kSearchToggle); SetWindowTextW(search_, L"ERROR"); SendMessageW(regex_, BM_SETCHECK, BST_UNCHECKED, 0); find();
    DWORD first = 0, last = 0; SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&first), reinterpret_cast<LPARAM>(&last));
    check("literal next highlights correct selection and disables follow", last - first == 5 && !follows());
    options_command(kMark); check("bookmark selected line", bookmarks_.size() == 1);
    append("\r\nnext"); refresh(); check("incoming append preserves bookmark", bookmarks_.size() == 1);
    options_command(kNextMark); options_command(kClearMarks); check("bookmark navigation / clear", bookmarks_.empty());
    SetWindowTextW(search_, L"ERROR|warn"); SendMessageW(regex_, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(highlight_, BM_SETCHECK, BST_CHECKED, 0); options_command(kHighlight);
    check("multi-match highlighting uses bounded RichEdit view", rich_view_ && view_ == rich_view_ && matches_.size() == 2);
    SendMessageW(view_, EM_SETSEL, matches_.front().start, matches_.front().start + matches_.front().length);
    CHARFORMAT2W cf{}; cf.cbSize = sizeof(cf); SendMessageW(view_, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    check("highlight has actual non-default background", cf.crBackColor == RGB(230,210,70));
    SetWindowTextW(search_, L"["); find(); check("invalid search regex is visible, data retained", !search_status_.empty() && matches_.empty() && !buffer_.bytes().empty());
    SetWindowTextW(search_, L""); SendMessageW(highlight_, BM_SETCHECK, BST_UNCHECKED, 0); options_command(kHighlight);
    options_command(kPause); const auto frozen = visible_text(); append(" received while paused"); refresh();
    check("display pause continues raw capture", visible_text() == frozen && buffer_.bytes().find("received while paused") != std::string::npos);
    options_command(kPause); check("display resume catches up", visible_text().find(L"received while paused") != std::wstring::npos);
    display.clock = MessageClock::Local; configure_display(display); check("PC timestamp added without modifying raw bytes", visible_text().find(L"] ") != std::wstring::npos && buffer_.bytes().front() != '[');
    display.ansi_sgr = true; display.clock = MessageClock::None; configure_display(display); reset(); append("\x1b[31mRED\x1b[0m green"); refresh();
    SendMessageW(view_, EM_SETSEL, 0, 3); cf = {}; cf.cbSize = sizeof(cf); SendMessageW(view_, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    check("ANSI SGR produces real colored text", visible_text().find(L"RED") != std::wstring::npos && cf.crTextColor == RGB(220,70,70));
    options_command(kWrap);
    const auto path = std::filesystem::path(folder) / L"uart-display-settings.ini";
    check("options saved to isolated INI", save_options(path.wstring()));
    configure_display({}); load_options(path.wstring()); check("display preferences restore", options_.encoding == MessageEncoding::Utf8 && options_.ansi_sgr && wrap_);
    for (int width : {1060, 1450}) {
        RECT page{}; GetWindowRect(hwnd_, &page); MapWindowPoints(nullptr, GetParent(hwnd_), reinterpret_cast<POINT*>(&page), 2);
        MoveWindow(hwnd_, page.left, page.top, width, 500, TRUE);
        RECT client{}; GetClientRect(hwnd_, &client);
        bool fits = true;
        for (const auto control : {label_, mode_, follow_, clear_, font_label_, font_down_, font_size_, font_up_, options_button_, search_toggle_, search_, regex_, case_, previous_, next_, highlight_, mark_, next_mark_, clear_marks_}) {
            RECT r{}; GetWindowRect(control, &r); MapWindowPoints(nullptr, hwnd_, reinterpret_cast<POINT*>(&r), 2);
            fits = fits && r.left >= 0 && r.right <= client.right;
        }
        check("toolbar/search controls fit tested width", fits);
    }
    RECT parent_client{}; GetClientRect(GetParent(hwnd_), &parent_client);
    SendMessageW(GetParent(hwnd_), WM_SIZE, SIZE_RESTORED, MAKELPARAM(parent_client.right, parent_client.bottom));
    check("new display screenshot", save_runtime_frame(GetParent(hwnd_), std::filesystem::path(folder) / L"uart-display.png"));
    options_command(kWrap); display = {}; configure_display(display); reset();
    SendMessageW(follow_, BM_SETCHECK, BST_CHECKED, 0);
    if (search_visible_) options_command(kSearchToggle);
    return pass;
}

bool CMainDialog::verify_uart_receive() {
    const auto folder = std::filesystem::path(settings_path_).parent_path();
    TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page();
    const bool controls = message_page_.verify_receive_controls(folder.wstring());
    // Only the existing paths affected by byte transfer / formatting / shared fonts.
    const bool raw = verify_messages();
    const bool exported = verify_message_export();
    const bool font = verify_message_font();
    const bool serial = verify_serial_connection();
    std::ofstream report(folder / L"uart-recording-integration-result.txt"); bool recorded = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; recorded = recorded && ok; };
    auto original = std::move(serial_mgr_); const bool demo = demo_; demo_ = false;
    auto fake = std::make_shared<FakeSerialState>();
    serial_mgr_ = std::make_unique<FakeSerialTransport>(fake);
    check("fixture transport opens without Windows COM API", serial_mgr_->connect(L"fake://recording", 115200, 8, 0, 0, nullptr));
    RecordingOptions recording; recording.base = folder / (L"integration_" + std::to_wstring(GetTickCount64()));
    check("recorder start", recorder_.start(recording));
    const std::string payload = std::string(300000, 'x') + "\r\nfan,value:201\r\n";
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->raw = payload; }
    ::SetWindowTextW(edit_include_, L"no-such-line"); ::SetWindowTextW(edit_exclude_, L""); apply_filter(false);
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page(); start_serial_thread();
    const auto until = GetTickCount64() + 3000;
    while (recorder_.status().bytes < payload.size() && GetTickCount64() < until) Sleep(1);
    check("recording proceeds before UI flush while Messages is hidden", recorder_.status().bytes == payload.size());
    flush_pending_lines(); message_page_.reset();
    check("filter / display clear do not stop recorder", capture_.rows().empty() && recorder_.status().active);
    disconnect(); OnCancel();
    check("normal close defers to UART/recording drain without destroying dialog", (close_after_recording_ || close_after_serial_) && ::IsWindow(m_hWnd));
    const auto stop_deadline = GetTickCount64() + 3000;
    while (close_after_serial_ && GetTickCount64() < stop_deadline) { poll_serial_stop(); Sleep(1); }
    check("disconnect and close-stop drain recording", !close_after_serial_ && recorder_.wait_idle(3000)); close_after_recording_ = false;
    std::ifstream input(std::filesystem::path(recording.base.wstring() + L"_1.bin"), std::ios::binary);
    const std::string saved(std::istreambuf_iterator<char>(input), {});
    check("raw file exceeds UI retention and preserves exact payload", saved == payload && saved.size() > MessageBuffer::kCapacity);
    serial_mgr_ = std::move(original); demo_ = demo;
    ::SetWindowTextW(edit_include_, L""); apply_filter(false);
    return controls && raw && exported && font && serial && recorded;
}
