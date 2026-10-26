#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include <limits>

namespace {
double serial_seconds() {
    static LARGE_INTEGER frequency = [] { LARGE_INTEGER value; QueryPerformanceFrequency(&value); return value; }();
    LARGE_INTEGER value; QueryPerformanceCounter(&value);
    return static_cast<double>(value.QuadPart) / static_cast<double>(frequency.QuadPart);
}
std::wstring caption(HWND control) {
    wchar_t value[64] = {}; ::GetWindowTextW(control, value, 64); return value;
}
int positive_integer(const std::wstring& value) {
    int number = 0;
    if (value.empty()) return 0;
    for (const auto ch : value) {
        if (ch < L'0' || ch > L'9' || number > (std::numeric_limits<int>::max() - (ch - L'0')) / 10) return 0;
        number = number * 10 + (ch - L'0');
    }
    return number;
}
}

void CMainDialog::start_serial_thread() {
    if (serial_running_) return;
    if (!stop_serial_thread()) return;
    uart_operations_.begin(); serial_running_ = true; serial_done_ = false; serial_operation_done_ = false;
    try {
    {std::lock_guard<std::mutex> lock(pending_mutex_);source_lines_.configure(numeric_framing_[0]);}
    const auto traffic_generation=uart_operations_.generation();
    serial_thread_ = std::thread([this,traffic_generation] {
        struct Done { std::atomic<bool>& value; ~Done() { value = true; } } done{serial_done_};
        try {
        while (serial_running_) {
            std::wstring error; std::string raw;
            auto lines = serial_mgr_->read_lines(&error, &raw);
            uart_statistics_.observe(serial_mgr_->sample_status());
            if (!lines.empty() || !raw.empty()) {
                const auto time = serial_seconds();
                const auto receipt = message_unix_ms();
                uart_statistics_.receive(raw.size(), GetTickCount64());
                automation_.receive(raw);
                recorder_.append(raw, receipt);
                if(workbench_feed_enabled_)primary_traffic_->append(raw,receipt,false,time,traffic_generation);
                std::lock_guard<std::mutex> lock(pending_mutex_); pending_.raw.append(raw, 0, receipt);
                if(!raw.empty()) {if(workbench_numeric_owned_||!numeric_framing_valid_[0])source_lines_.skip_bytes(raw.size());else for(auto& mapped:source_lines_.feed(raw,time))pending_.lines.push_back({std::move(mapped.text),mapped.seconds,mapped.offset,mapped.count,traffic_generation});pending_.source_end=source_lines_.end();oversized_lines_=source_lines_.skipped();}
                else for (const auto& line : lines) pending_.lines.push_back({line,time,UINT64_MAX,0,traffic_generation});
                bound_pending_lines(pending_,pending_dropped_);
            }
            {std::lock_guard<std::mutex> lock(pending_mutex_);if(!workbench_numeric_owned_&&numeric_framing_valid_[0])for(auto& mapped:source_lines_.drain(serial_seconds()))pending_.lines.push_back({std::move(mapped.text),mapped.seconds,mapped.offset,mapped.count,traffic_generation});bound_pending_lines(pending_,pending_dropped_);}
            if (!error.empty()) {
                if (serial_running_) {
                    { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_ = error; }
                    serial_error_pending_ = true;
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        } catch (...) { if (serial_running_) { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_ = L"UART receive worker failed"; serial_error_pending_ = true; } }
    });
    serial_operation_thread_ = std::thread([this] {
        struct Done { std::atomic<bool>& value; ~Done() { value = true; } } done{serial_operation_done_};
        while (serial_running_) {
            if (!serial_mgr_->is_connected()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
            SerialOperation operation;
            if (!uart_operations_.take(operation)) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue; }
            SerialOperationResult result; result.operation = operation;
            if (!serial_running_ || !uart_operations_.current(operation.generation)) { result.cancelled = true; uart_operations_.complete(std::move(result)); continue; }
            try {
                if (operation.kind == SerialOperation::Kind::Write) {
                    result.accepted = paced_write(operation.bytes,operation.pacing,[&](std::string_view part,std::wstring& error){return serial_mgr_->write_bytes(part,&error);},[&]{return serial_running_&&uart_operations_.current(operation.generation)&&(!operation.cancel||!operation.cancel->load());},result.error);
                    result.success = result.error.empty() && result.accepted == operation.bytes.size();
                    uart_statistics_.transmit(result.accepted, !result.success, GetTickCount64());
                    if (result.accepted) recorder_.append(std::string_view(operation.bytes).substr(0, result.accepted), message_unix_ms(), true);
                    if(result.accepted&&workbench_feed_enabled_)primary_traffic_->append(std::string_view(operation.bytes).substr(0,result.accepted),message_unix_ms(),true,0,operation.generation);
                } else if (operation.kind == SerialOperation::Kind::Controls) result.success = serial_mgr_->set_controls(operation.controls, &result.error);
                else result.success = serial_mgr_->send_break(&result.error);
            } catch (...) { result.error = L"UART operation worker failed; outcome may be uncertain. No automatic retry."; }
            uart_operations_.complete(std::move(result));
        }
    });
    } catch (...) {
        serial_running_ = false; uart_operations_.cancel();
        if (!serial_thread_.joinable()) serial_done_ = true;
        if (!serial_operation_thread_.joinable()) serial_operation_done_ = true;
        { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_ = L"Cannot start UART workers"; } serial_error_pending_ = true;
    }
}

bool CMainDialog::stop_serial_thread() {
    serial_running_ = false;
    uart_operations_.cancel();
    if (serial_thread_.joinable() || serial_operation_thread_.joinable()) serial_mgr_->cancel_io();
    bool complete = true;
    const auto finish = [&](std::thread& thread, bool done) {
        if (!thread.joinable()) return;
        CancelSynchronousIo(thread.native_handle());
        if (done && WaitForSingleObject(thread.native_handle(), 0) == WAIT_OBJECT_0) thread.join();
        else complete = false;
    };
    finish(serial_thread_, serial_done_); finish(serial_operation_thread_, serial_operation_done_);
    return complete;
}

void CMainDialog::poll_serial_stop() {
    if (!disconnect_pending_ || !stop_serial_thread()) return;
    const bool unexpected = disconnect_unexpected_; disconnect_pending_ = false;
    disconnect(unexpected);
    if (close_after_serial_) { close_after_serial_ = false; OnCancel(); }
}

void CMainDialog::scan_ports() {
    bool ok = false; auto ports = port_scanner_(&ok);
    if (ok) update_port_combo(ports);
}

void CMainDialog::update_port_combo(const std::vector<SerialPortInfo>& ports) {
    const int selected = static_cast<int>(::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0));
    if (selected >= 0 && selected < static_cast<int>(known_ports_.size())) selection_hint_ = known_ports_[selected];
    if (connection_.mode() != SerialConnectionState::Mode::Idle) selection_hint_ = connection_.active().port;
    known_ports_ = ports;
    ::SendMessageW(combo_port_, CB_RESETCONTENT, 0, 0);
    for (const auto& port : ports) {
        const auto label = port.device + L" - " + port.description;
        ::SendMessageW(combo_port_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    int index = find_serial_port(ports, selection_hint_);
    if (selection_hint_.device.empty() && selection_hint_.instance_id.empty() && !ports.empty()) index = 0;
    ::SendMessageW(combo_port_, CB_SETCURSEL, index, 0);
    if (index >= 0) selection_hint_ = ports[index];
}

void CMainDialog::set_serial_status(const std::wstring& message) {
    flash_active_ = false; KillTimer(IDT_STATUS); set_left_status(message);
}

void CMainDialog::update_connection_ui() {
    const auto mode = connection_.mode();
    const bool idle = mode == SerialConnectionState::Mode::Idle && !disconnect_pending_;
    const auto text = disconnect_pending_ ? L"Stopping" : idle ? L"Connect" : mode == SerialConnectionState::Mode::Connected ? L"Disconnect" : L"Cancel";
    if (caption(btn_connect_) != text) ::SetWindowTextW(btn_connect_, text);
    for (auto control : {combo_port_, combo_baud_, combo_data_, combo_parity_, combo_stop_, btn_scan_})
        if ((::IsWindowEnabled(control) != FALSE) != idle) ::EnableWindow(control, idle);
    ::EnableWindow(btn_connect_, !disconnect_pending_);
    // Layout reserves the widest caption: state changes must not move/repaint every page.
    ::InvalidateRect(btn_connect_, nullptr, FALSE);
}

void CMainDialog::on_connect_toggle() {
    if (connection_.mode() != SerialConnectionState::Mode::Idle || serial_mgr_->is_connected()) disconnect();
    else connect_with_validation();
}

bool CMainDialog::fail_manual_connection(const std::wstring& port, const std::wstring& reason) {
    if (closing_serial_ || !::IsWindow(m_hWnd)) return false;
    const auto target = port.empty() ? L"the selected COM port" : port;
    set_serial_status(L"COM: Cannot connect to " + target + L" - " + reason);
    log_line(L"Connect failed: " + target + L" - " + reason);
    const auto message = L"Cannot connect to " + target + L".\r\n\r\nReason: " + reason +
        L"\r\n\r\nThe port may be in use, disconnected, or unavailable.\r\n"
        L"Please check:\r\n"
        L"1. Disconnect this COM port in Tera Term, PuTTY, or another copy of this tool.\r\n"
        L"   Close that connection/application if needed, then try again.\r\n"
        L"2. Check the bridge/USB cable and the COM number in Device Manager > Ports.\r\n"
        L"3. Select the correct port and baud/data/parity/stop settings, then press Connect.\r\n\r\n"
        L"Other applications will not be closed automatically.";
    // The native modal loop may close its owner. Keep dispatch local; touch no members afterwards.
    const auto notify = connection_notice_;
    notify(m_hWnd, message.c_str(), L"Connection failed", MB_OK | MB_ICONWARNING);
    return false;
}

bool CMainDialog::connect_with_validation() {
    if (replay_.data() || replay_load_.busy() || profile_job_.busy())
        return fail_manual_connection(selection_hint_.device, L"Unload offline replay / finish profile work before connecting");
    if (disconnect_pending_) return fail_manual_connection(selection_hint_.device, L"Previous UART I/O is still stopping; wait and try again");
    std::wstring controls_error; load_uart_controls(settings_path_, controls_error);
    if (!controls_error.empty()) return fail_manual_connection(selection_hint_.device,
        controls_error + L" Open UART Console, review and Apply serial options before connecting.");
    if (!::SendMessageW(combo_port_, CB_GETCOUNT, 0, 0)) scan_ports();
    const int selected = static_cast<int>(::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(known_ports_.size())) {
        return fail_manual_connection(L"", L"No available COM port selected");
    }
    SerialSettings settings; settings.port = known_ports_[selected];
    settings.controls = desired_controls_;
    settings.baud = positive_integer(caption(combo_baud_)); settings.data_bits = positive_integer(caption(combo_data_));
    const auto parity = caption(combo_parity_), stop = caption(combo_stop_);
    settings.parity = parity == L"NONE" ? NOPARITY : parity == L"ODD" ? ODDPARITY : parity == L"EVEN" ? EVENPARITY : -1;
    settings.stop_bits = stop == L"1" ? ONESTOPBIT : stop == L"1.5" ? ONE5STOPBITS : stop == L"2" ? TWOSTOPBITS : -1;
    if (!valid_serial_settings(settings)) return fail_manual_connection(settings.port.device, L"Invalid serial settings");
    return begin_serial_open(settings, false);
}

bool CMainDialog::begin_serial_open(const SerialSettings& settings, bool automatic) {
    if (closing_serial_) return false;
    if(workbench_&&workbench_->secondary_owns(settings.port))return automatic?false:fail_manual_connection(settings.port.device,L"The Workbench Port B already owns this device. Select a different port or disconnect B.");
    if (serial_open_.busy()) {
        return automatic ? false : fail_manual_connection(settings.port.device, L"Previous open is still closing; wait and try again");
    }
    if (connection_.mode() != (automatic ? SerialConnectionState::Mode::Waiting : SerialConnectionState::Mode::Idle) ||
        !valid_serial_settings(settings)) return false;
    connection_.begin(settings, automatic);
    if (!serial_open_.start(settings, transport_factory_)) {
        connection_.failed(GetTickCount64()); update_connection_ui();
        if (!automatic) return fail_manual_connection(settings.port.device, L"Cannot start connection worker");
        set_serial_status(L"COM: Cannot start connection worker"); return false;
    }
    update_connection_ui();
    set_serial_status(std::wstring(automatic ? L"COM: Reconnecting " : L"COM: Connecting ") + settings.port.device + L"...");
    return true;
}

void CMainDialog::poll_serial_connection() {
    poll_serial_stop();
    if (disconnect_pending_ || closing_serial_ || !::IsWindow(m_hWnd)) return;
    std::unique_ptr<SerialTransport> opened; std::wstring error;
    if (!serial_open_.poll(opened, error)) return;
    if (closing_serial_ || !connection_.opening()) return; // Cancelled results never become live.
    const bool automatic = connection_.mode() == SerialConnectionState::Mode::Reopening;
    const auto requested = connection_.active();
    bool scanned = false; const auto ports = port_scanner_(&scanned);
    const int match = scanned ? find_serial_port(ports, requested.port) : -1;
    const bool same_path = match >= 0 && (requested.port.open_path.empty() || same_serial_name(requested.port.open_path, ports[match].open_path));
    if (opened && opened->is_connected() && match >= 0 && same_path) {
        if (!stop_serial_thread()) return;
        serial_mgr_ = std::move(opened);
        connection_.connected(ports[match]); remembered_serial_ = connection_.active(); selection_hint_ = ports[match];
        update_port_combo(ports);
        serial_error_pending_ = false; { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_.clear(); }
        if (!automatic) {
            primary_traffic_->clear();
            uart_statistics_.reset();
            model_.reset(); capture_.reset(static_cast<size_t>(model_.get_max_channels())); data_page_.reset(); message_page_.reset();
            { std::lock_guard<std::mutex> lock(pending_mutex_); pending_ = PendingData{}; pending_dropped_ = 0; }
            model_.set_time_window(_wtof(caption(combo_time_).c_str())); plot_view_.reset_visual(); channel_panel_.reset();
        }
        start_serial_thread(); update_connection_ui();
        const auto parity = remembered_serial_.parity == ODDPARITY ? L"ODD" : remembered_serial_.parity == EVENPARITY ? L"EVEN" : L"NONE";
        const auto stop = remembered_serial_.stop_bits == ONE5STOPBITS ? L"1.5" : remembered_serial_.stop_bits == TWOSTOPBITS ? L"2" : L"1";
        const auto status = std::wstring(automatic ? L"COM: Reconnected " : L"COM: Connected ") + remembered_serial_.port.device +
            L" (" + std::to_wstring(remembered_serial_.baud) + L", " + std::to_wstring(remembered_serial_.data_bits) +
            L"/" + parity + L"/" + stop + L")";
        set_serial_status(status); log_line(status); retry_log_error_.clear();
        recorder_.event(message_utf8(status), message_unix_ms());
        save_settings(); // Keep an INI failure visible instead of overwriting it with Connected.
        return;
    }
    if (error.empty()) error = L"Device identity changed or port is unavailable";
    opened.reset(); // Release any rejected transport before asking the user to retry.
    const auto now = GetTickCount64(); connection_.failed(now); update_connection_ui();
    if (!automatic) { fail_manual_connection(requested.port.device, error); return; }
    set_serial_status(L"COM: Waiting to reconnect " + requested.port.device + L" - " + error);
    if (error != retry_log_error_ || now - retry_log_at_ >= 30000) {
        log_line(L"Reconnect failed: " + error);
        retry_log_at_ = now; retry_log_error_ = error;
    }
}

void CMainDialog::poll_serial_ports(uint64_t now) {
    if (closing_serial_) return;
    bool ok = false; const auto ports = port_scanner_(&ok); if (!ok) return;
    const bool changed = ports.size() != known_ports_.size() || !std::equal(ports.begin(), ports.end(), known_ports_.begin(),
        [](const SerialPortInfo& a, const SerialPortInfo& b) { return a.device == b.device && a.description == b.description && a.instance_id == b.instance_id && a.open_path == b.open_path; });
    if (changed) update_port_combo(ports);
    if (connection_.mode() == SerialConnectionState::Mode::Connected && find_serial_port(ports, connection_.active().port) < 0) {
        log_line(L"Serial device removed"); disconnect(true);
    }
    if (!serial_open_.busy() && connection_.retry_due(now)) {
#ifdef CHART_RUNTIME_CHECKS
        if (demo_) return;
#endif
        const int match = find_serial_port(ports, connection_.active().port);
        // Never retry a reused COM name or an unidentifiable legacy interface.
        if (match >= 0 && !ports[match].open_path.empty()) {
            auto settings = connection_.active(); settings.port = ports[match]; begin_serial_open(settings, true);
        }
    }
}

void CMainDialog::disconnect(bool unexpected) {
    cancel_file_tx(L"File TX stopped: disconnected.");
    stop_automation(L"Stopped: UART disconnected; no reconnect resume");
    if (!stop_serial_thread()) {
        if (!disconnect_pending_) disconnect_unexpected_ = unexpected;
        else if (!unexpected) disconnect_unexpected_ = false;
        disconnect_pending_ = true; serial_open_.cancel();
        update_connection_ui(); set_serial_status(L"COM: Cancelling pending UART I/O..."); return;
    }
    disconnect_pending_ = false; controls_pending_id_ = 0;
    recorder_.event(unexpected ? "Unexpected disconnect; waiting for same bridge" : "Manual disconnect", message_unix_ms());
    serial_open_.cancel(); serial_mgr_->disconnect();
    serial_error_pending_ = false; { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_.clear(); }
    if (unexpected) {
        flush_pending_lines(); connection_.lost(GetTickCount64());
        set_serial_status(connection_.retrying() ? L"COM: Waiting for original bridge " + connection_.active().port.device + L" (automatic reconnect)" :
            L"COM: Disconnected; reconnect manually (no stable device identity)");
    } else {
        connection_.cancel(); set_serial_status(L"COM: Disconnected");
        ::SendMessageW(combo_auto_, CB_SETCURSEL, 0, 0); KillTimer(IDT_AUTO);
        { std::lock_guard<std::mutex> lock(pending_mutex_);
          pending_.lines.clear(); message_page_.append_history(pending_.raw); pending_.raw.reset(); pending_dropped_ = 0; }
        message_page_.refresh();
    }
    update_connection_ui();
}
