#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include <cmath>
#include <sstream>
void CMainDialog::stop_automation(const std::wstring& reason) {
    if (!automation_.active() && !automation_id_) return;
    automation_.stop(reason);
    const bool queued=automation_id_ && uart_operations_.cancel_pending(automation_id_);
    automation_id_=0;
    const auto text=reason+(queued?L"; TX cancellation requested; accepted bytes cannot be revoked":L"; an in-flight TX may already have affected the target");
    advanced_tools_.notice(AdvancedTools::Automation,text); show_status_message(text,8000); recorder_.event(message_utf8(text),message_unix_ms());
}
void CMainDialog::automation_request(const AdvancedTools::Request& request) {
    if (request.action==1) { stop_automation(L"Stopped by user"); return; }
    const auto tell=[&](const std::wstring& text) { advanced_tools_.notice(AdvancedTools::Automation,text); };
    if (request.fields.size()!=11 || automation_.active() || file_tx_busy() || automation_id_ || controls_pending_id_ || uart_operations_.busy() || (workbench_&&workbench_->tx_reserved(0)) ||
        !serial_running_ || disconnect_pending_ || connection_.mode()!=SerialConnectionState::Mode::Connected || replay_.data()) { tell(L"Connect first and finish/stop pending work; no offline or overlapping automation runs."); return; }
    AutomationConfig config;
    config.mode=request.fields[0]==L"Batch"?AutomationConfig::Mode::Batch:request.fields[0]==L"Auto reply"?AutomationConfig::Mode::AutoReply:AutomationConfig::Mode::Periodic;
    TxEntry entry; entry.hex=request.fields[1]==L"HEX"; entry.ending=request.fields[2]==L"CR"?TxEnding::CR:request.fields[2]==L"LF"?TxEnding::LF:request.fields[2]==L"CRLF"?TxEnding::CRLF:TxEnding::None;
    const auto checksum=request.fields[3]==L"Sum8"?TxChecksum::Sum8:request.fields[3]==L"XOR8"?TxChecksum::Xor8:request.fields[3]==L"CRC16 Modbus (LE)"?TxChecksum::ModbusCrc16:TxChecksum::None;
    const auto integer=[&](size_t index,unsigned low,unsigned high,unsigned& out) { double value=0; if (!uart_number(request.fields[index],low,high,value) || std::floor(value)!=value) return false; out=static_cast<unsigned>(value); return true; };
    if (!integer(4,100,60000,config.interval_ms) || !integer(5,1,10000,config.count) || !integer(7,100,60000,config.timeout_ms) || !integer(8,0,3,config.retries)) { tell(L"Interval/timeout 100..60000 ms; count 1..10000; timeout retries 0..3 (integers)."); return; }
    std::wstring error; std::wistringstream lines(request.fields[10]); std::wstring line;
    while (std::getline(lines,line)) {
        if (!line.empty() && line.back()==L'\r') line.pop_back(); if (line.empty()) continue;
        entry.text=line; std::string bytes;
        if (config.commands.size()>=64 || !automation_payload(entry,checksum,bytes,error)) { tell(error.empty()?L"Maximum 64 command lines":error); return; } config.commands.push_back(std::move(bytes));
    }
    const auto pattern=[&](size_t index,std::string& bytes) { if (request.fields[index].empty()) return true; TxEntry value; value.text=request.fields[index]; value.hex=entry.hex; return make_tx_payload(value,bytes,error); };
    if (!pattern(6,config.wait_reply) || !pattern(9,config.trigger) || !validate_automation(config,error)) { tell(error); return; }
    const auto generation=uart_operations_.generation();
    const auto owner=advanced_tools_.GetSafeHwnd(); const auto confirm=uart_notice_;
    const int answer=confirm(owner,L"Start finite UART automation on the currently connected target?\n\nCommands can change hardware state. Timeout retries explicitly resend commands and may duplicate effects. Matching raw RX bytes is not a verified protocol ACK; echoes may match. Auto reply is literal, not a script.\n\nStop/close/disconnect ends the run; reconnect never resumes. Check target and settings before continuing.",L"Start UART automation",MB_OKCANCEL|MB_ICONWARNING);
    if (!::IsWindow(m_hWnd) || closing_serial_ || answer!=IDOK) return;
    if (!uart_operations_.current(generation) || disconnect_pending_ || file_tx_busy() || (workbench_&&workbench_->tx_reserved(0)) || !serial_running_ || connection_.mode()!=SerialConnectionState::Mode::Connected) { tell(L"Connection changed during confirmation; nothing started."); return; }
    automation_generation_=generation;
    if (automation_.start(std::move(config),GetTickCount64(),error)) { tell(L"Automation explicitly started. One-hour safety limit; manual Console TX/controls disabled until stopped."); poll_advanced_tools(); }
    else tell(error);
}
