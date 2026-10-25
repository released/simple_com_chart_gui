#include "mfc_main_dialog.h"
#include "uart_workbench.h"
void CMainDialog::cancel_file_tx(const std::wstring& reason) {
    if(!file_tx_active_ && !file_tx_job_.busy()) return;
    file_tx_active_=false;file_tx_job_.cancel();file_tx_next_.clear();if(file_tx_id_) uart_operations_.cancel_pending(file_tx_id_);
    advanced_tools_.notice(AdvancedTools::FileTx,reason+L" Driver-accepted bytes cannot be revoked; nothing resumes after reconnect.");
    advanced_tools_.state(AdvancedTools::FileTx,reason+L" | driver accepted "+std::to_wstring(file_tx_accepted_)+L" bytes");file_tx_.reset();
}
void CMainDialog::file_tx_request(const AdvancedTools::Request& request) {
    const auto tell=[&](const std::wstring& s){advanced_tools_.notice(AdvancedTools::FileTx,s);};
    if(request.action==4) {cancel_file_tx(L"File TX cancelled.");return;}
    if(request.action==2) {file_tx_paused_=true;tell(L"Paused after any in-flight chunk; no new chunk will be queued.");return;}
    if(request.action==3) {if(file_tx_active_) {file_tx_paused_=false;file_tx_at_=GetTickCount64();}return;}
    if(request.action!=1 || request.fields.size()!=5) return;
    if(file_tx_busy()||file_tx_id_||automation_.active()||controls_pending_id_||uart_operations_.busy()||(workbench_&&workbench_->tx_reserved(0)) || !serial_running_||disconnect_pending_||!serial_mgr_->is_connected()) {tell(L"Connect and wait for pending TX/controls, stop automation/Workbench/file work before Start.");return;}
    FileTransferOptions options;options.text=request.fields[1]==L"UTF-8 text";options.newline=static_cast<int>(::SendMessageW(advanced_tools_.field_hwnd(request.page,2),CB_GETCURSEL,0,0));
    double chunk=0,delay=0;if(!uart_number(request.fields[3],1,4096,chunk)||chunk!=static_cast<int>(chunk)||!uart_number(request.fields[4],0,60000,delay)||delay!=static_cast<int>(delay)) {tell(L"Chunk must be 1..4096 bytes; delay 0..60000 ms (integers).");return;}
    options.chunk=static_cast<size_t>(chunk);options.delay_ms=static_cast<unsigned>(delay);
    const auto generation=uart_operations_.generation();const auto target=connection_.active().port.device;
    const auto text=L"Send this file to "+target+L"?\n\n"+request.fields[0]+L"\nMode: "+request.fields[1]+L"; chunk "+std::to_wstring(options.chunk)+L" bytes; delay "+std::to_wstring(options.delay_ms)+L" ms.\n\nAll file bytes (including BOM/control bytes) are sent, except explicitly selected text newline conversion. File contents may execute target commands. This is not a firmware flashing protocol; there is no ACK/retry.";
    const auto confirm=uart_notice_;if(confirm(advanced_tools_.GetSafeHwnd(),text.c_str(),L"Confirm file TX",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return;
    if(!::IsWindow(m_hWnd)||closing_serial_||!serial_running_||disconnect_pending_||!serial_mgr_->is_connected()||!uart_operations_.current(generation)||file_tx_busy()||automation_.active()||(workbench_&&workbench_->tx_reserved(0))) return;
    const auto path=std::filesystem::path(request.fields[0]);
    if(!file_tx_job_.start([path,options](const std::atomic<bool>& cancel){FileTxWork work;work.transfer=FileTransfer::prepare(path,options,cancel);work.chunk=work.transfer->next(cancel);return work;})) {tell(L"Cannot start file preparation worker.");return;}
    file_tx_generation_=generation;file_tx_active_=true;file_tx_paused_=false;file_tx_accepted_=0;file_tx_source_=0;file_tx_end_=false;file_tx_next_.clear();file_tx_.reset();
    tell(L"Preparing in background; UTF-8 text is validated completely before any TX.");
}
void CMainDialog::poll_file_tx() {
    FileTxWork work;std::wstring error;
    if(file_tx_job_.take(work,error)) {
        if(file_tx_active_) {
            if(!error.empty()) {cancel_file_tx(error);advanced_tools_.state(AdvancedTools::FileTx,error);}
            else {file_tx_=std::move(work.transfer);file_tx_next_=std::move(work.chunk.bytes);file_tx_source_=work.chunk.source_read;file_tx_end_=work.chunk.end;}
        }
    }
    if(!file_tx_active_) return;
    if(!serial_running_||disconnect_pending_||!serial_mgr_->is_connected()||!uart_operations_.current(file_tx_generation_)) {cancel_file_tx(L"File TX stopped: connection changed.");return;}
    const auto now=GetTickCount64();
    if(!file_tx_paused_ && !file_tx_id_ && !file_tx_job_.busy() && now>=file_tx_at_ && file_tx_) {
        if(!file_tx_next_.empty()) {
            SerialOperation operation;operation.target=connection_.active().port.device;operation.bytes=file_tx_next_;
            if(!uart_operations_.submit(operation,file_tx_id_,error)) {cancel_file_tx(L"File TX queue failed: "+error);return;}file_tx_next_.clear();
        } else if(file_tx_end_) {file_tx_active_=false;advanced_tools_.notice(AdvancedTools::FileTx,L"Completed: driver accepted "+std::to_wstring(file_tx_accepted_)+L" bytes. This is not a target ACK.");}
        else {const auto transfer=file_tx_;file_tx_job_.start([transfer](const std::atomic<bool>& cancel){return FileTxWork{transfer,transfer->next(cancel)};});}
    }
    advanced_tools_.state(AdvancedTools::FileTx,std::wstring(file_tx_active_?(file_tx_paused_?L"PAUSED":L"SENDING"):L"COMPLETED")+L" | driver accepted "+std::to_wstring(file_tx_accepted_)+L" bytes | source read "+std::to_wstring(file_tx_source_)+L" / "+std::to_wstring(file_tx_?file_tx_->size():0)+L" | pending "+(file_tx_id_?L"yes":L"no"));
}
