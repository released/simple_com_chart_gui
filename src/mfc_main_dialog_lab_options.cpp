#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "input_framing.h"
#include <cmath>
#include <cstring>
void CMainDialog::load_input_framing(){
    std::lock_guard<std::mutex> lock(pending_mutex_);
    for(unsigned i=0;i<3;++i){try{numeric_framing_[i]=saved_text_framing(settings_path_,i);numeric_framing_valid_[i]=true;}catch(...){numeric_framing_[i]={};numeric_framing_valid_[i]=false;show_status_message(L"Invalid saved numeric framing: numeric input disabled for that source; review Laboratory > Numeric input framing.",8000);}}
    source_lines_.configure(numeric_framing_[replay_.data()?2:0]);
}
void CMainDialog::lab_input_request(const UartLabWindow::Request& r){
    if(r.fields.size()!=4)return;const unsigned source=r.fields[0]==L"Port B"?1:r.fields[0]==L"Offline replay"?2:0;
    const std::vector<std::wstring> modes{L"LF",L"CR",L"CRLF",L"Custom delimiter",L"Receipt idle gap"};
    if(r.action==0){const auto& c=numeric_framing_[source];lab_.field(r.page,1,modes[c.mode]);lab_.field(r.page,2,uart_wide(uart_hex(c.delimiter)));lab_.field(r.page,3,std::to_wstring(c.idle_ms));lab_.notice(r.page,numeric_framing_valid_[source]?L"Saved framing loaded; no live configuration changed.":L"Invalid stored framing; review and Apply valid values before numeric input.");return;}
    if(!advanced_idle()||replay_.data()||recorder_.status().active||exporting_)throw std::runtime_error("Disconnect ports, unload replay and finish recording/export/file work before applying framing.");
    TextFraming c;const auto mode=std::find(modes.begin(),modes.end(),r.fields[1]);if(mode==modes.end())throw std::runtime_error("Invalid framing mode.");c.mode=static_cast<TextFraming::Mode>(mode-modes.begin());double ms=0;if(!uart_number(r.fields[3],1,60000,ms)||ms!=std::floor(ms))throw std::runtime_error("Idle gap must be 1..60000 whole milliseconds.");c.idle_ms=static_cast<unsigned>(ms);
    if(c.mode==TextFraming::Delimiter){TxEntry tx{r.fields[2],true,TxEnding::None};std::wstring error;if(!make_tx_payload(tx,c.delimiter,error))throw std::runtime_error(message_utf8(error));}
    const auto encoded=uart_wide(encode_text_framing(c));const wchar_t* keys[]={L"A",L"B",L"Replay"};if(!WritePrivateProfileStringW(L"InputFraming",keys[source],encoded.c_str(),settings_path_.c_str()))throw std::runtime_error("Cannot save framing; previous settings retained.");
    load_input_framing();if(workbench_)workbench_->reload_preferences();apply_filter(false);lab_.notice(r.page,L"Numeric framing saved/applied. New numeric capture; raw history/recordings unchanged. No port opened.");
}
void CMainDialog::lab_devices_request(const UartLabWindow::Request& r){
    if(r.action==0){if(r.fields.size()!=1)return;const auto search=r.fields[0];const auto scan=device_scanner_;start_lab_file(r.page,r.action,[scan,search](const std::atomic<bool>& cancel){return LabFileResult{message_utf8(device_catalog_report(scan(cancel),search)),{}};});return;}
    if(r.action!=1)return;const auto& text=lab_.reports_[UartLabWindow::Devices];if(text.empty())throw std::runtime_error("Refresh device information first.");
    const auto bytes=(text.size()+1)*sizeof(wchar_t);const auto memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory)throw std::runtime_error("Cannot allocate clipboard text.");
    const auto block=GlobalLock(memory);if(!block){GlobalFree(memory);throw std::runtime_error("Cannot lock clipboard text.");}std::memcpy(block,text.c_str(),bytes);GlobalUnlock(memory);
    if(!::OpenClipboard(lab_.m_hWnd)){GlobalFree(memory);throw std::runtime_error("Clipboard is busy; device data was not copied.");}bool copied=false;if(::EmptyClipboard())copied=::SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;::CloseClipboard();if(!copied){GlobalFree(memory);throw std::runtime_error("Cannot copy device information.");}
}
void CMainDialog::lab_crc_request(const UartLabWindow::Request& r){
    if(r.fields.size()!=5)return;const auto spec=parse_crc_spec(message_utf8(r.fields[0]));double first=0,count=0;if(!uart_number(r.fields[2],0,65536,first)||!uart_number(r.fields[3],0,65536,count)||first!=std::floor(first)||count!=std::floor(count))throw std::runtime_error("CRC byte range must use nonnegative integers.");
    if(r.action==1){lab_.field(UartLabWindow::Packet,5,L"None");lab_.field(UartLabWindow::Packet,6,r.fields[4]);lab_.field(UartLabWindow::Packet,10,uart_wide(encode_crc_spec(spec)));lab_.field(UartLabWindow::Packet,11,r.fields[2]);lab_.field(UartLabWindow::Packet,12,r.fields[3]);lab_.select(UartLabWindow::Packet);lab_.notice(UartLabWindow::Packet,L"Custom CRC copied to packet definition only. Preview before explicit Send once.");return;}
    if(r.action==2){auto directive=encode_crc_spec(spec);std::replace(directive.begin(),directive.end(),':',',');lab_.notice(r.page,L"Binary layout directive:\r\n@crc,"+uart_wide(directive)+L","+r.fields[3]+L"\r\nSet standard Checksum=None, Checksum from="+r.fields[2]+L", byte order="+r.fields[4]+L". Add to the selected live/offline binary schema; explicit Apply is required.");return;}
    std::string input;if(!r.fields[1].empty()){std::wstring error;if(!make_tx_payload({r.fields[1],true,TxEnding::None},input,error))throw std::runtime_error(message_utf8(error));}
    if(first>input.size()||(count&&count>input.size()-static_cast<size_t>(first)))throw std::runtime_error("CRC preview range exceeds the provided bytes.");
    const auto n=count?static_cast<size_t>(count):input.size()-static_cast<size_t>(first);const auto crc=CustomCrc(spec).calculate(std::string_view(input).substr(static_cast<size_t>(first),n));std::string result;for(size_t i=0;i<spec.width/8;++i)result+=static_cast<char>(crc>>(8*(r.fields[4]==L"Big"?spec.width/8-1-i:i)));
    lab_.notice(r.page,L"Preview only: "+std::to_wstring(n)+L" input bytes; CRC decimal "+std::to_wstring(crc)+L"; output HEX "+uart_wide(uart_hex(result))+L". No TX.");
}
