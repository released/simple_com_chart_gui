#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"

bool CMainDialog::verify_serial_connection() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"serial-connection-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; report.flush(); pass = ok && pass; };
    const auto original_path = settings_path_;
    const auto original_ports = known_ports_; const auto original_hint = selection_hint_; const auto original_remembered = remembered_serial_;
    const auto original_scanner = port_scanner_; const auto original_factory = transport_factory_;
    const auto original_notice = connection_notice_;
    unsigned notices = 0; std::wstring notice_text;
    connection_notice_ = [&](HWND owner, LPCWSTR message, LPCWSTR title, UINT flags) {
        ++notices; notice_text = message;
        check("manual error is an owned OK/warning message box", owner == m_hWnd &&
            std::wstring(title) == L"Connection failed" && flags == (MB_OK | MB_ICONWARNING));
        check("warning follows restoration of the idle/retry-ready controls", connection_.mode() == SerialConnectionState::Mode::Idle &&
            ::IsWindowEnabled(combo_port_) && ::IsWindowEnabled(btn_scan_));
        return IDOK;
    };
    const bool original_demo = demo_;
    disconnect(); demo_ = false;
    auto original_transport = std::move(serial_mgr_);
    auto fake = std::make_shared<FakeSerialState>();
    SerialPortInfo target{L"COM4", L"Test bridge", L"USB\\BRIDGE-A", L"fake://bridge-a"};
    SerialPortInfo other{L"COM3", L"Other bridge", L"USB\\BRIDGE-B", L"fake://bridge-b"};
    fake->ports = {other, target};
    port_scanner_ = [fake](bool* ok) {
        std::lock_guard<std::mutex> lock(fake->mutex); *ok = fake->scan_ok; return fake->ports;
    };
    transport_factory_ = [fake] { return std::make_unique<FakeSerialTransport>(fake); };
    serial_mgr_ = std::make_unique<FakeSerialTransport>(fake);
    const auto pump = [&](const std::function<bool()>& ready) {
        const auto end = GetTickCount64() + 2000;
        while (GetTickCount64() < end) {
            poll_serial_connection(); flush_pending_lines();
            if (ready()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ready();
    };
    const auto click = [&] { OnCommand(MAKEWPARAM(103, BN_CLICKED), 0); };
    const auto stored_port = [&] {
        wchar_t value[32] = {}; GetPrivateProfileStringW(L"Serial", L"Port", L"", value, 32, settings_path_.c_str()); return std::wstring(value);
    };
    settings_path_ = (evidence / L"serial-connection-test.ini").wstring();
    WritePrivateProfileStringW(L"Serial", L"Port", L"COM4", settings_path_.c_str());
    WritePrivateProfileStringW(L"Serial", L"DeviceInstanceId", target.instance_id.c_str(), settings_path_.c_str());
    WritePrivateProfileStringW(L"Serial", L"Baud", L"230400", settings_path_.c_str());
    WritePrivateProfileStringW(L"Serial", L"DataBits", L"7", settings_path_.c_str());
    WritePrivateProfileStringW(L"Serial", L"Parity", L"EVEN", settings_path_.c_str());
    WritePrivateProfileStringW(L"Serial", L"StopBits", L"2", settings_path_.c_str());
    load_serial_settings(); scan_ports();
    check("INI selects remembered identity and settings without opening hardware", ::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0) == 1 &&
        remembered_serial_.baud == 230400 && remembered_serial_.data_bits == 7 && remembered_serial_.parity == EVENPARITY &&
        remembered_serial_.stop_bits == TWOSTOPBITS && fake->attempt_count() == 0);
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L""); ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0); apply_filter(false);
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->fail_opens = 1; fake->open_error = L"Open failed (5): Access is denied."; }
    ::SendMessageW(combo_port_, CB_SETCURSEL, 0, 0); click();
    check("failed manual open returns idle and does not overwrite last successful port", pump([&] { return connection_.mode() == SerialConnectionState::Mode::Idle; }) && stored_port() == L"COM4");
    check("busy/access-denied manual open warns once with PORT, original error and checks", notices == 1 &&
        notice_text.find(L"COM3") != std::wstring::npos && notice_text.find(L"Open failed (5)") != std::wstring::npos &&
        notice_text.find(L"Tera Term") != std::wstring::npos && notice_text.find(L"Device Manager") != std::wstring::npos &&
        notice_text.find(L"will not be closed automatically") != std::wstring::npos);
    { std::ofstream text(evidence / L"manual-connection-warning.txt", std::ios::binary);
      for (const auto ch : notice_text) text.put(static_cast<char>(ch));
      text.flush(); check("warning text evidence written without newline translation", text.good()); }
    poll_serial_connection(); check("completed failure does not repeat the warning on later polls", notices == 1);
    ::SendMessageW(combo_port_, CB_SETCURSEL, 1, 0); click();
    check("manual connection runs through background open", connection_.opening() && !::IsWindowEnabled(combo_port_) &&
        pump([&] { return connection_.mode() == SerialConnectionState::Mode::Connected; }));
    check("successful open saves confirmed port immediately", stored_port() == L"COM4" && serial_mgr_->is_connected());
    check("successful connection adds no warning", notices == 1);
    { std::lock_guard<std::mutex> lock(fake->mutex);
      check("open uses enumerated device interface and remembered serial parameters", !fake->attempts.empty() &&
          fake->attempts.back().port.open_path == target.open_path && fake->attempts.back().baud == 230400 &&
          fake->attempts.back().data_bits == 7 && fake->attempts.back().parity == EVENPARITY && fake->attempts.back().stop_bits == TWOSTOPBITS);
      fake->raw = "value:201\r\nplain message\r\n"; }
    check("reader captures fake bridge bytes", pump([&] { return capture_.rows().size() == 2; }));
    const auto quiet_count = fake->attempt_count(); poll_serial_ports();
    check("quiet connected stream does not trigger reconnect", connection_.mode() == SerialConnectionState::Mode::Connected && fake->attempt_count() == quiet_count);
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->scan_ok = false; }
    poll_serial_ports(); check("failed enumeration is not mistaken for device removal", connection_.mode() == SerialConnectionState::Mode::Connected);
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->scan_ok = true; fake->read_error = true; }
    { std::lock_guard<std::mutex> lock(pending_mutex_); pending_.raw.append("tail:7\r\n"); pending_.lines.push_back({"tail:7", 0}); }
    check("read error arms automatic recovery and drains queued capture", pump([&] { return connection_.retrying(); }) && capture_.rows().size() == 3 &&
        message_page_.buffer().bytes().find("tail:7") != std::string::npos);
    const auto retained = capture_.rows().size(); const auto raw = message_page_.buffer().bytes();
    const auto attempts = fake->attempt_count(); auto reused = other; reused.device = L"COM4";
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->ports = {reused}; }
    poll_serial_ports(GetTickCount64() + 100000);
    check("another bridge reusing COM4 is never opened", fake->attempt_count() == attempts && connection_.retrying() && ::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0) == CB_ERR);
    TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page();
    check("waiting state screenshot", save_runtime_frame(m_hWnd, evidence / L"serial-reconnect-waiting.png"));
    auto returned = target; returned.device = L"COM9";
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->ports = {reused, returned}; fake->fail_opens = 1; }
    poll_serial_ports(GetTickCount64() + 100000);
    check("busy original bridge schedules another retry without clearing history", pump([&] { return connection_.mode() == SerialConnectionState::Mode::Waiting; }) &&
        capture_.rows().size() == retained && message_page_.buffer().bytes() == raw);
    check("automatic reconnect failure stays non-modal", notices == 1);
    poll_serial_ports(GetTickCount64() + 100000);
    check("original bridge can recover on a new COM number", pump([&] { return connection_.mode() == SerialConnectionState::Mode::Connected; }) &&
        remembered_serial_.port.device == L"COM9" && stored_port() == L"COM9");
    check("automatic success preserves chart capture and raw history", capture_.rows().size() == retained && message_page_.buffer().bytes() == raw);
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->raw = "value:202\r\n"; }
    check("new reader appends after recovery", pump([&] { return capture_.rows().size() == retained + 1; }));
    check("reconnected state screenshot", save_runtime_frame(m_hWnd, evidence / L"serial-reconnected.png"));
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->ports.clear(); }
    poll_serial_ports(); check("PnP removal also triggers recovery without a read error", connection_.retrying());
    click(); const auto cancelled_count = fake->attempt_count();
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->ports = {returned}; }
    poll_serial_ports(GetTickCount64() + 100000);
    check("manual cancel prevents any automatic reopening", connection_.mode() == SerialConnectionState::Mode::Idle && fake->attempt_count() == cancelled_count && ::IsWindowEnabled(combo_port_));
    { std::lock_guard<std::mutex> lock(fake->mutex); fake->hold_open = true; }
    click(); check("cancellation fixture enters a pending open", pump([&] { return fake->attempt_count() > cancelled_count; }));
    click();
    const auto notices_before_pending = notices;
    check("new open cannot accumulate while cancelled driver call is pending", !connect_with_validation() && fake->attempt_count() == cancelled_count + 1);
    check("pending previous open reports why a new manual attempt cannot start", notices == notices_before_pending + 1 &&
        notice_text.find(L"Previous open is still closing") != std::wstring::npos);
    fake->release();
    const auto notices_before_late = notices;
    check("late successful open is discarded after user cancellation", pump([&] { return !serial_open_.busy(); }) && !serial_mgr_->is_connected() && connection_.mode() == SerialConnectionState::Mode::Idle);
    check("cancelled late result adds no warning", notices == notices_before_late);
    ::SendMessageW(combo_port_, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    check("missing selection shows actionable manual error without opening", !connect_with_validation() && notices == notices_before_late + 1 &&
        notice_text.find(L"No available COM port selected") != std::wstring::npos && !serial_open_.busy());
    ::SendMessageW(combo_port_, CB_SETCURSEL, 0, 0); ::SetWindowTextW(combo_baud_, L"invalid");
    check("invalid settings show actionable manual error without opening", !connect_with_validation() && notices == notices_before_late + 2 &&
        notice_text.find(L"Invalid serial settings") != std::wstring::npos && !serial_open_.busy());
    WritePrivateProfileStringW(L"Serial", L"OpenPath", L"C:\\not-a-device", settings_path_.c_str()); load_serial_settings();
    check("INI cannot supply an executable/file/device interface path", remembered_serial_.port.open_path.empty() && !serial_open_.busy());
    WritePrivateProfileStringW(L"Serial", L"Port", L"C:\\not-a-port", settings_path_.c_str()); load_serial_settings();
    check("malformed INI port/settings restore safely without an open", remembered_serial_.port.device.empty() && !serial_mgr_->is_connected());
    disconnect(); fake->release(); pump([&] { return !serial_open_.busy(); });
    serial_mgr_ = std::move(original_transport); port_scanner_ = original_scanner; transport_factory_ = original_factory;
    connection_notice_ = original_notice;
    settings_path_ = original_path; remembered_serial_ = original_remembered; selection_hint_ = original_hint;
    ::SendMessageW(combo_port_, CB_SETCURSEL, static_cast<WPARAM>(-1), 0); update_port_combo(original_ports); load_serial_settings();
    demo_ = original_demo; update_connection_ui();
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": simulated serial persistence, loss/retry/renumber, history preservation and cancellation; NO real COM opens\n";
    return pass;
}
