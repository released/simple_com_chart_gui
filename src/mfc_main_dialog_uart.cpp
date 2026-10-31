#include "mfc_main_dialog.h"
#include "build_version.h"
#include "uart_workbench.h"

void CMainDialog::show_uart_console() {
    if (!uart_console_.show(this, settings_path_, ui_font_)) { show_status_message(L"Cannot create UART Console.", 5000); return; }
    uart_console_.set_controls(desired_controls_); poll_uart_operations();
}

LRESULT CMainDialog::OnUartRequest(WPARAM, LPARAM parameter) {
    if (!parameter || closing_serial_) return 0;
    const auto request = *reinterpret_cast<const UartConsole::Request*>(parameter);
    const auto generation = uart_operations_.generation();
    const auto notice = [&](const std::wstring& text) { uart_console_.notice(text); show_status_message(text, 8000); };
    if ((automation_.active() || file_tx_busy() || (workbench_&&workbench_->tx_reserved(0))) && request.action != UartConsole::Request::Action::ResetStatistics) {
        notice(L"Stop automation / Workbench operation before manual TX / changing serial controls / Break."); return 0;
    }
    if (request.action == UartConsole::Request::Action::ResetStatistics) {
        uart_statistics_.reset(); notice(L"Traffic/error statistics reset. Capture, recording and message history are unchanged."); return 0;
    }
    if (request.action == UartConsole::Request::Action::Controls) {
        if (!valid_serial_controls(request.controls)) { notice(L"Invalid serial controls."); return 0; }
        if (controls_pending_id_) { notice(L"Wait for the pending serial-control operation."); return 0; }
        const auto mode = connection_.mode();
        if (disconnect_pending_ || (mode != SerialConnectionState::Mode::Idle && mode != SerialConnectionState::Mode::Connected)) { notice(L"Wait for connection/cancellation before changing serial controls."); return 0; }
        if (mode == SerialConnectionState::Mode::Idle) {
            if (!save_uart_controls(settings_path_, request.controls)) { notice(L"Cannot save serial controls: " + settings_path_); return 0; }
            desired_controls_ = request.controls;
            notice(L"Serial options saved for the next manual connection. DTR/RTS/handshake may reset or affect a connected board; verify wiring before Connect."); return 0;
        }
    }
    SerialOperation operation;
    operation.target = connection_.active().port.device;
    if (request.action == UartConsole::Request::Action::Send) {
        std::wstring error;
        if (!make_tx_payload(request.tx, operation.bytes, error)) { notice(error); return 0; }
    } else {
        operation.kind = request.action == UartConsole::Request::Action::Controls ? SerialOperation::Kind::Controls : SerialOperation::Kind::Break;
        operation.controls = request.controls;
        const auto confirm = uart_notice_;
        const auto owner = uart_console_.GetSafeHwnd() ? uart_console_.GetSafeHwnd() : m_hWnd;
        const int answer = confirm(owner,
            operation.kind == SerialOperation::Kind::Break ?
            L"Send one approximately 250 ms UART Break?\n\nThis can interrupt/reset some devices. It is not Ctrl+C. Check the target before continuing. Nothing will be replayed after reconnect." :
            L"Apply serial flow/DTR/RTS settings now?\n\nChanging these signals can reset the target, change boot mode or pause TX. Check wiring and target state. Nothing will be replayed as a command after reconnect.",
            L"UART hardware action", MB_OKCANCEL | MB_ICONWARNING);
        if (!::IsWindow(m_hWnd) || closing_serial_ || answer != IDOK) return 0;
    }
    // A confirmation runs a nested loop: revalidate after it, never use cached connection state.
    if (disconnect_pending_ || connection_.mode() != SerialConnectionState::Mode::Connected || !serial_running_ || !serial_mgr_->is_connected() || !uart_operations_.current(generation)) {
        notice(L"Connection changed, disconnected or stopping; the operation was not queued. Review the target and submit again."); return 0;
    }
    std::wstring error; uint64_t id = 0;
    if (!uart_operations_.submit(operation, id, error)) { notice(error); return 0; }
    if (operation.kind == SerialOperation::Kind::Controls) controls_pending_id_ = id;
    if (operation.kind == SerialOperation::Kind::Write) uart_console_.remember(request.tx);
    uart_console_.notice(L"Queued #" + std::to_wstring(id) + L" for " + operation.target + L". No automatic retry or reconnect replay.");
    return 0;
}

void CMainDialog::poll_uart_operations() {
    if (closing_serial_ || !::IsWindow(m_hWnd)) return;
    for (const auto& result : uart_operations_.results()) {
        if(workbench_)workbench_->operation_result(0,result);
        if(result.operation.id==file_tx_id_) {
            file_tx_id_=0;file_tx_accepted_+=result.accepted;
            if(!result.success || result.cancelled || !uart_operations_.current(file_tx_generation_)) cancel_file_tx(L"File TX failed/partial/cancelled; no retry.");
            else file_tx_at_=GetTickCount64()+(file_tx_?file_tx_->delay():0);
        }
        if (result.operation.id == automation_id_) {
            automation_id_ = 0;
            automation_.complete(result.success && !result.cancelled && uart_operations_.current(automation_generation_), GetTickCount64());
        }
        uart_console_.result(result);
        const bool current = !result.cancelled && uart_operations_.current(result.operation.generation) && !disconnect_pending_;
        if (result.operation.kind == SerialOperation::Kind::Controls && result.operation.id == controls_pending_id_) {
            controls_pending_id_ = 0;
            if (result.success && current) {
                desired_controls_ = result.operation.controls; remembered_serial_.controls = desired_controls_; connection_.update_controls(desired_controls_);
                uart_console_.set_controls(desired_controls_);
                if (!save_uart_controls(settings_path_, desired_controls_)) {
                    const auto text = L"Hardware settings updated, but cannot save INI: " + settings_path_ + L". Session settings remain active.";
                    uart_console_.notice(text); show_status_message(text, 15000);
                }
            }
        }
        const auto text = std::wstring(result.operation.kind == SerialOperation::Kind::Write ? L"TX" : L"UART control") + L" #" +
            std::to_wstring(result.operation.id) + L" [" + result.operation.target + L"]" + (result.cancelled ? L" cancelled/previous session" : result.success ? L" completed" : L" failed") +
            L" | driver accepted " + std::to_wstring(result.accepted) + L" bytes" + (result.error.empty() ? L"" : L" | " + result.error);
        recorder_.event(message_utf8(text), message_unix_ms());
        if (!result.success || result.cancelled) { show_status_message(text, 8000); log_line(text); }
    }
    const bool connected = connection_.mode() == SerialConnectionState::Mode::Connected && serial_running_ && !disconnect_pending_ && serial_mgr_->is_connected();
    const bool configurable = !disconnect_pending_ && (connection_.mode() == SerialConnectionState::Mode::Idle || connected);
    message_page_.set_keyboard_available(connected && !automation_.active() && !file_tx_busy() && (!workbench_||!workbench_->tx_reserved(0)),uart_operations_.generation(),connected);
    if (uart_console_.GetSafeHwnd()) {
        const auto target = connected ? connection_.active().port.device : disconnect_pending_ ? L"Stopping" : connection_.retrying() ? L"Reconnect waiting" : L"Disconnected";
        const auto title = L"UART Console - " + std::wstring(target) + L" | " + CHART_VERSION_W;
        CString current; uart_console_.GetWindowTextW(current);
        if (current != title.c_str()) uart_console_.SetWindowTextW(title.c_str());
    }
    uart_console_.update(connected, configurable, controls_pending_id_ != 0, uart_statistics_.snapshot(GetTickCount64()), uart_operations_.queued(), GetTickCount64());
}
