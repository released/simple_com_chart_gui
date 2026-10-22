#include "mfc_main_dialog.h"
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
    stop_serial_thread(); serial_running_ = true;
    serial_thread_ = std::thread([this] {
        while (serial_running_) {
            std::wstring error; std::string raw;
            auto lines = serial_mgr_->read_lines(&error, &raw);
            if (!error.empty()) {
                { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_ = error; }
                serial_error_pending_ = true; break;
            }
            if (!lines.empty() || !raw.empty()) {
                const auto time = serial_seconds();
                std::lock_guard<std::mutex> lock(pending_mutex_); pending_.raw.append(raw);
                for (const auto& line : lines) pending_.lines.push_back({line, time});
                if (pending_.lines.size() > 2000) {
                    const auto excess = pending_.lines.size() - 2000;
                    pending_.lines.erase(pending_.lines.begin(), pending_.lines.begin() + excess);
                    pending_dropped_ += static_cast<int>(excess);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
}

void CMainDialog::stop_serial_thread() {
    serial_running_ = false;
    if (serial_thread_.joinable()) {
        CancelSynchronousIo(serial_thread_.native_handle());
        serial_thread_.join();
    }
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
    const bool idle = mode == SerialConnectionState::Mode::Idle;
    const auto text = idle ? L"Connect" : mode == SerialConnectionState::Mode::Connected ? L"Disconnect" : L"Cancel";
    if (caption(btn_connect_) != text) ::SetWindowTextW(btn_connect_, text);
    for (auto control : {combo_port_, combo_baud_, combo_data_, combo_parity_, combo_stop_, btn_scan_})
        if ((::IsWindowEnabled(control) != FALSE) != idle) ::EnableWindow(control, idle);
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
    if (!::SendMessageW(combo_port_, CB_GETCOUNT, 0, 0)) scan_ports();
    const int selected = static_cast<int>(::SendMessageW(combo_port_, CB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(known_ports_.size())) {
        return fail_manual_connection(L"", L"No available COM port selected");
    }
    SerialSettings settings; settings.port = known_ports_[selected];
    settings.baud = positive_integer(caption(combo_baud_)); settings.data_bits = positive_integer(caption(combo_data_));
    const auto parity = caption(combo_parity_), stop = caption(combo_stop_);
    settings.parity = parity == L"NONE" ? NOPARITY : parity == L"ODD" ? ODDPARITY : parity == L"EVEN" ? EVENPARITY : -1;
    settings.stop_bits = stop == L"1" ? ONESTOPBIT : stop == L"1.5" ? ONE5STOPBITS : stop == L"2" ? TWOSTOPBITS : -1;
    if (!valid_serial_settings(settings)) return fail_manual_connection(settings.port.device, L"Invalid serial settings");
    return begin_serial_open(settings, false);
}

bool CMainDialog::begin_serial_open(const SerialSettings& settings, bool automatic) {
    if (closing_serial_) return false;
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
    std::unique_ptr<SerialTransport> opened; std::wstring error;
    if (!serial_open_.poll(opened, error)) return;
    if (closing_serial_ || !connection_.opening()) return; // Cancelled results never become live.
    const bool automatic = connection_.mode() == SerialConnectionState::Mode::Reopening;
    const auto requested = connection_.active();
    bool scanned = false; const auto ports = port_scanner_(&scanned);
    const int match = scanned ? find_serial_port(ports, requested.port) : -1;
    const bool same_path = match >= 0 && (requested.port.open_path.empty() || same_serial_name(requested.port.open_path, ports[match].open_path));
    if (opened && opened->is_connected() && match >= 0 && same_path) {
        stop_serial_thread(); serial_mgr_ = std::move(opened);
        connection_.connected(ports[match]); remembered_serial_ = connection_.active(); selection_hint_ = ports[match];
        update_port_combo(ports);
        serial_error_pending_ = false; { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_.clear(); }
        if (!automatic) {
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
    if (!demo_ && !serial_open_.busy() && connection_.retry_due(now)) {
        const int match = find_serial_port(ports, connection_.active().port);
        // Never retry a reused COM name or an unidentifiable legacy interface.
        if (match >= 0 && !ports[match].open_path.empty()) {
            auto settings = connection_.active(); settings.port = ports[match]; begin_serial_open(settings, true);
        }
    }
}

void CMainDialog::disconnect(bool unexpected) {
    serial_open_.cancel(); stop_serial_thread(); serial_mgr_->disconnect();
    serial_error_pending_ = false; { std::lock_guard<std::mutex> lock(error_mutex_); serial_error_.clear(); }
    if (unexpected) {
        flush_pending_lines(); connection_.lost(GetTickCount64());
        set_serial_status(connection_.retrying() ? L"COM: Waiting for original bridge " + connection_.active().port.device + L" (automatic reconnect)" :
            L"COM: Disconnected; reconnect manually (no stable device identity)");
    } else {
        connection_.cancel(); set_serial_status(L"COM: Disconnected");
        ::SendMessageW(combo_auto_, CB_SETCURSEL, 0, 0); KillTimer(IDT_AUTO);
        { std::lock_guard<std::mutex> lock(pending_mutex_);
          pending_.lines.clear(); message_page_.append(pending_.raw.bytes(), pending_.raw.offset()); pending_.raw.reset(); pending_dropped_ = 0; }
        message_page_.refresh();
    }
    update_connection_ui();
}
