#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "settings_fields.h"
#include "input_framing.h"
#include <cmath>
namespace {
std::vector<std::wstring> defaults(){return {L"Disabled",L"",L"",L"Text",L"Key/value",L"",L"",L"1",L"2"};}
bool config(const std::vector<std::wstring>& f,TextTelemetryConfig& c,std::wstring& error){
    if(f.size()!=9){error=L"Invalid text parser fields.";return false;}
    const std::vector<std::wstring> modes{L"Key/value",L"CSV",L"Tab",L"JSON",L"Regex"};const auto found=std::find(modes.begin(),modes.end(),f[4]);
    if((f[0]!=L"Disabled"&&f[0]!=L"Enabled")||(f[3]!=L"Text"&&f[3]!=L"Regex")||found==modes.end()){error=L"Invalid text parser mode.";return false;}
    c.enabled=f[0]==L"Enabled";c.regex=f[3]==L"Regex";c.include=message_utf8(f[1]);c.exclude=message_utf8(f[2]);c.extraction.mode=static_cast<ExtractionConfig::Mode>(found-modes.begin());c.extraction.columns=message_utf8(f[5]);c.extraction.pattern=message_utf8(f[6]);
    double a=0,b=0;if(!uart_number(f[7],1,8,a)||!uart_number(f[8],1,8,b)||a!=std::floor(a)||b!=std::floor(b)){error=L"Capture group indexes must be 1..8.";return false;}c.extraction.name_group=static_cast<int>(a);c.extraction.value_group=static_cast<int>(b);
    TextTelemetry test;std::string detail;if(!test.configure(c,detail)){error=uart_wide(detail);return false;}return true;
}
}
bool UartWorkbench::apply_text(const std::vector<std::wstring>& f,std::wstring& error,bool persist){
    TextTelemetryConfig c;if(!config(f,c,error))return false;
    TextFraming framing;try{framing=saved_text_framing(ini_,1);}catch(const std::exception& e){error=uart_wide(e.what());return false;}
    if(persist){Request check;check.action=Request::CheckClock;if(!request(check)||tx_reserved(1)){error=L"Finish offline/profile/TX work and clear text device-time settings before applying Port B charts.";return false;}
        try{const auto encoded=pack_text_settings(f);if(!WritePrivateProfileStringW(L"Workbench.Text.B",L"Configuration",encoded.c_str(),ini_.c_str())){error=L"Cannot save settings; previous parser retained.";return false;}}catch(const std::exception& e){error=uart_wide(e.what());return false;}}
    text_b_.framing(framing);
    std::string detail;text_b_.configure(c,detail);text_fields_=f;const auto tail=secondary_.traffic.after(UINT64_MAX,1);text_sequence_=tail.newest;text_revision_=tail.revision;text_lost_=0;
    if(persist){Request reset;reset.action=Request::ResetNumeric;request(reset);}return true;
}
void UartWorkbench::load_text(){text_b_=TextTelemetry{};text_fields_=defaults();std::vector<wchar_t> value(32768);const auto n=GetPrivateProfileStringW(L"Workbench.Text.B",L"Configuration",L"",value.data(),static_cast<DWORD>(value.size()),ini_.c_str());if(!n)return;try{std::wstring error;if(n>=value.size()-1||!apply_text(unpack_text_settings(value.data()),error,false))throw std::runtime_error("Invalid Port B text parser.");}catch(...){groups_[1].save_error=L"Invalid Port B text settings; review Tools > Text charts B.";}}
void UartWorkbench::text_dialog(){const auto f=text_fields_.empty()?defaults():text_fields_;
    UartToolForm form(L"Port B text charts",L"Independent Port B line filters and numeric extraction. PC receipt clock; fields appear as B.<name> and start unchecked. Binary/RTU/file-transfer-owned RX bypasses text charts. Raw log/recording is unchanged. Apply starts a fresh numeric capture.",
      {{L"Text charts",f[0],{L"Disabled",L"Enabled"}},{L"Include",f[1]},{L"Exclude",f[2]},{L"Line filter",f[3],{L"Text",L"Regex"}},{L"Extraction",f[4],{L"Key/value",L"CSV",L"Tab",L"JSON",L"Regex"}},{L"Columns / JSON ptr:name=/path",f[5]},{L"Regex pattern",f[6]},{L"Name capture group",f[7]},{L"Value capture group",f[8]}},{},[](const auto& v,std::wstring& e){TextTelemetryConfig c;return config(v,c,e);},this);
    if(form.DoModal()==IDOK){std::wstring error;if(!apply_text(form.values(),error))warn(error);}
}
void UartWorkbench::decode_text(){if(!text_b_.config().enabled)return;const auto batch=secondary_.traffic.after(text_sequence_,32);
    if(text_revision_!=batch.revision){text_b_.interrupt();text_sequence_=0;text_revision_=batch.revision;}
    const auto actual=text_sequence_==0?secondary_.traffic.after(0,32):batch;
    if(text_sequence_&&!actual.records.empty()&&actual.records.front().sequence>text_sequence_+1){text_b_.interrupt();++text_lost_;}
    for(const auto& record:actual.records){text_sequence_=record.sequence;if(binary_enabled(1)||numeric_suspended(1)){text_b_.interrupt();continue;}text_b_.receive(record);}
    if(!binary_enabled(1)&&!numeric_suspended(1))text_b_.drain(traffic_monotonic_seconds());
}
