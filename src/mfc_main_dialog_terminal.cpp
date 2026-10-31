#include "mfc_main_dialog.h"
#include "uart_workbench.h"
LRESULT CMainDialog::OnKeyboardArm(WPARAM,LPARAM) {
    const auto generation=uart_operations_.generation();
    const auto ready=[&]{return serial_running_ && serial_mgr_->is_connected() && !disconnect_pending_ && !automation_.active() && !file_tx_busy() && (!workbench_||!workbench_->tx_reserved(0)) && connection_.mode()==SerialConnectionState::Mode::Connected;};
    if(!ready()) {show_status_message(L"Connect and stop automation / file TX before enabling keyboard TX.",6000);return 0;}
    const auto confirm=uart_notice_;
    const auto answer=confirm(m_hWnd,L"Enable keyboard transmission in Messages?\n\nTyping sends UTF-8 immediately to the current UART. Enter sends CR; Ctrl+C sends byte 03 (not Break). Paste always requires a separate preview/confirmation. Manual disable is kept for this connection, including temporary TX suspension. A new connection enables keyboard TX by default.",L"Keyboard TX",MB_OKCANCEL|MB_ICONWARNING);
    if(::IsWindow(m_hWnd) && !closing_serial_ && answer==IDOK && ready() && uart_operations_.current(generation)) message_page_.arm_keyboard(true);return 0;
}
LRESULT CMainDialog::OnKeyboardTx(WPARAM generation,LPARAM value) {
    if(!value) return 0;const auto request=*reinterpret_cast<const MessagePage::KeyboardTx*>(value);
    const auto ready=[&]{return serial_running_ && !disconnect_pending_ && serial_mgr_->is_connected() && !automation_.active() && !file_tx_busy() && (!workbench_||!workbench_->tx_reserved(0)) && connection_.mode()==SerialConnectionState::Mode::Connected && uart_operations_.current(generation);};
    if(!ready() || request.bytes.empty() || request.bytes.size()>4096) return 0;
    if(request.paste) {
        const auto text=L"Send this clipboard content to UART?\n\n"+std::to_wstring(request.bytes.size())+L" bytes\nASCII: "+tx_ascii_preview(request.bytes,256)+L"\n\nMultiple lines can execute multiple target commands. No suffix will be added.";
        const auto confirm=uart_notice_;if(confirm(m_hWnd,text.c_str(),L"Confirm UART paste",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return 0;
        if(!::IsWindow(m_hWnd)||closing_serial_||!ready()) return 0;
    }
    SerialOperation operation;operation.target=connection_.active().port.device;operation.bytes=request.bytes;uint64_t id=0;std::wstring error;
    if(!uart_operations_.submit(std::move(operation),id,error)) show_status_message(error,6000);else {message_page_.echo_tx(request.bytes);if(message_page_.local_echo()) show_status_message(L"LOCAL TX ASCII queued: "+tx_ascii_preview(request.bytes,32),5000);uart_console_.notice(L"KEYBOARD queued #"+std::to_wstring(id)+L" (not RX) ASCII: "+tx_ascii_preview(request.bytes));}return 0;
}
