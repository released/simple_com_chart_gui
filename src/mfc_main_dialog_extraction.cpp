#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include <sstream>

void CMainDialog::load_extraction() {
    std::vector<wchar_t> text(32768);
    const auto n=GetPrivateProfileStringW(L"Extraction",L"Configuration",L"",text.data(),static_cast<DWORD>(text.size()),settings_path_.c_str());
    std::string error;
    try {if(n>=text.size()-1) throw std::runtime_error("Oversized parser settings");const auto c=decode_extraction(message_utf8(text.data()));if(!extractor_.configure(c,error)) throw std::runtime_error(error);}
    catch(...) {extractor_=FieldExtractor{};show_status_message(L"Invalid saved parser settings; Key/value fallback active. Review UART > Extraction.",7000);}
}
void CMainDialog::extraction_request(const AdvancedTools::Request& request) {
    if(request.fields.size()!=8) return;
    ExtractionConfig c;std::string error;
    c.mode=static_cast<ExtractionConfig::Mode>(::SendMessageW(advanced_tools_.field_hwnd(AdvancedTools::Extraction,0),CB_GETCURSEL,0,0));
    c.columns=message_utf8(request.fields[1]);c.pattern=message_utf8(request.fields[2]);c.time_field=message_utf8(request.fields[6]);
    double numbers[3]{};for(int i=0;i<3;++i) if(!uart_number(request.fields[i+3],0,8,numbers[i]) || numbers[i]!=static_cast<int>(numbers[i])) {advanced_tools_.notice(request.page,L"Capture group numbers must be integers 0..8.");return;}
    c.name_group=static_cast<int>(numbers[0]);c.value_group=static_cast<int>(numbers[1]);c.time_group=static_cast<int>(numbers[2]);
    FieldExtractor next;if(!next.configure(c,error)) {advanced_tools_.notice(request.page,uart_wide(error));return;}
    if(request.action==0) {
        const auto result=next.extract(message_utf8(request.fields[7]));std::wostringstream preview;
        preview<<L"Preview only; "<<result.values.size()<<L" numeric fields";
        if(!result.error.empty()) preview<<L" | "<<uart_wide(result.error);if(result.limited) preview<<L" | work limit";
        if(result.seconds) preview<<L" | extracted time "<<*result.seconds<<L" s";
        for(const auto& value:result.values) preview<<L"\r\n"<<uart_wide(value.first)<<L" = "<<value.second;
        advanced_tools_.state(request.page,preview.str());return;
    }
    if(profile_job_.busy() || replay_load_.busy()) {advanced_tools_.notice(request.page,L"Wait for background settings/file work.");return;}
    if(workbench_&&workbench_->requires_receipt_clock()&&(!c.time_field.empty()||(c.mode==ExtractionConfig::Regex&&c.time_group>0))){advanced_tools_.notice(request.page,L"Disable Port B text / binary charts and stop RTU polling before selecting a separate device-time clock.");return;}
    const auto encoded=uart_wide(encode_extraction(c));
    if(!WritePrivateProfileStringW(L"Extraction",L"Configuration",encoded.c_str(),settings_path_.c_str())) {advanced_tools_.notice(request.page,L"Cannot save parser settings; previous parser retained.");return;}
    extractor_=std::move(next);apply_filter(false);advanced_tools_.notice(request.page,L"Parser saved/applied; new numeric capture. Raw Messages and recording unchanged.");
}
void CMainDialog::update_source_status() {
    const auto recorded=recorder_.status();const auto offline=replay_.data();
    const auto& raw=message_page_.buffer();
    std::wstring value=offline?L"OFFLINE":serial_mgr_->is_connected()?L"LIVE":L"LIVE (disconnected)";
    value+=recorded.lost || recorded.detail.find(L"ERROR")!=std::wstring::npos?L" | REC ERROR":recorded.limit_reached?L" | REC LIMIT":recorded.active?L" | REC on":L" | REC off";value+=automation_.active()?L" | AUTO on":L" | AUTO off";
    if(file_tx_busy()) value+=L" | FILE TX";
    if(workbench_&&workbench_->any_binary())value+=std::wstring(L" | Binary:")+(workbench_->binary_enabled(0)?L" A":L"")+(workbench_->binary_enabled(1)?L" B":L"");
    value+=L" | Clock: ";value+=extracted_clock_?L"field seconds":offline?(offline->receipt_times?L"PC receipt (file)":L"synthetic"):L"PC receipt";
    value+=L" | Expired: "+std::to_wstring(raw.offset())+L" B / "+std::to_wstring(capture_.dropped())+L" rows";
    value+=L" | Lost: "+std::to_wstring(raw.skipped())+L" B / "+std::to_wstring(pending_line_loss_)+L" lines";
    value+=L" | Skipped: "+std::to_wstring(line_filter_.limited()+parser_skipped_+oversized_lines_.load());
    wchar_t old[1024]{};::GetWindowTextW(source_status_,old,1024);if(value!=old) ::SetWindowTextW(source_status_,value.c_str());
}
