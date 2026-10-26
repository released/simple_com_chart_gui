#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "uart_files.h"
#include "binary_options.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace {
std::vector<std::wstring> defaults(){return {L"Disabled",L"",L"4",L"0",L"1",L"Little",L"0",L"None",L"0",L"Little",L"value,0,u16le",L""};}
const wchar_t* section(size_t i){return i==0?L"Workbench.Binary.A":L"Workbench.Binary.B";}
bool hex_input(const std::wstring& text,std::string& bytes){std::string compact;for(const wchar_t c:text){if(c==L' '||c==L'\t'||c==L'\r'||c==L'\n')continue;if(c>127)return false;compact.push_back(static_cast<char>(towupper(c)));}return compact.size()<=131072&&uart_unhex(compact,bytes);}
bool config_from(const std::vector<std::wstring>& f,BinaryFrameConfig& out,std::wstring& error){
    if(f.size()==12&&((f[0]!=L"Enabled"&&f[0]!=L"Disabled")||(f[5]!=L"Little"&&f[5]!=L"Big")||(f[9]!=L"Little"&&f[9]!=L"Big"))){error=L"Unsupported parser mode or byte order.";return false;}
    if(f.size()!=12){error=L"Invalid configuration field count.";return false;}if(!hex_input(f[1],out.header)){error=L"Header must contain hexadecimal bytes.";return false;}
    const int indexes[]={2,3,4,6,8};size_t* outputs[]={&out.fixed_bytes,&out.length_offset,&out.length_width,&out.length_add,&out.checksum_from};
    for(size_t i=0;i<5;++i){double number=0;if(!uart_number(f[indexes[i]],0,65536,number)||number!=std::floor(number)){error=L"Lengths and offsets must be whole byte counts 0..65536.";return false;}*outputs[i]=static_cast<size_t>(number);}
    out.length_big_endian=f[5]==L"Big";out.checksum_big_endian=f[9]==L"Big";
    const std::vector<std::wstring> names={L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"};const auto found=std::find(names.begin(),names.end(),f[7]);if(found==names.end()){error=L"Unsupported checksum.";return false;}out.checksum=static_cast<FrameChecksum>(found-names.begin());
    std::string detail;if(!parse_binary_layout(message_utf8(f[10]),out,detail)){error=uart_wide(detail);return false;}return true;
}
std::wstring encode(const std::vector<std::wstring>& f){std::string data;for(size_t i=0;i<11;++i){data+=message_utf8(f[i]);data.push_back('\0');}if(data.size()>14000)throw std::runtime_error("Binary settings exceed 14 KiB.");return (f[0]==L"Enabled"?L"v1:1:":L"v1:0:")+uart_wide(uart_hex(data));}
std::vector<std::wstring> decode(const std::wstring& text){if(text.rfind(L"v1:1:",0)!=0&&text.rfind(L"v1:0:",0)!=0)throw std::runtime_error("Unsupported binary settings version.");std::string data;if(!uart_unhex(message_utf8(text.substr(5)),data)||data.size()>14000)throw std::runtime_error("Invalid binary settings.");std::vector<std::wstring> f;size_t start=0;while(start<data.size()){const auto end=data.find('\0',start);if(end==std::string::npos)throw std::runtime_error("Truncated binary settings.");f.push_back(uart_wide(std::string_view(data).substr(start,end-start)));start=end+1;}if(f.size()!=11||(f[0]!=L"Enabled"&&f[0]!=L"Disabled")||(text[3]==L'1')!=(f[0]==L"Enabled"))throw std::runtime_error("Invalid binary field schema.");f.push_back(L"");return f;}
}
bool workbench_binary_config(const std::vector<std::wstring>& f,BinaryFrameConfig& out,std::wstring& error){return parse_binary_options(f,out,error);}
void UartWorkbench::initialize(const std::wstring& ini,std::shared_ptr<TrafficStore> primary){if(!ini_.empty())return;ini_=ini;primary_=std::move(primary);load_binary();load_text();load_framing();for(size_t i=0;i<2;++i){const auto section=i?L"Workbench.TX.B":L"Workbench.TX.A";wchar_t value[128]{};GetPrivateProfileStringW(section,L"Configuration",L"",value,128,ini_.c_str());if(!*value)continue;std::wstring saved(value);const auto split=saved.find(L':',3);double a=0,b=0;const bool ok=saved.rfind(L"v1:",0)==0&&split!=std::wstring::npos&&uart_number(saved.substr(3,split-3),0,1000,a)&&a==std::floor(a)&&uart_number(saved.substr(split+1),0,60000,b)&&b==std::floor(b);if(ok)pacing_[i]={static_cast<unsigned>(a),static_cast<unsigned>(b)};else{pacing_[i]={1001,0};groups_[i].save_error=L"Invalid saved TX pacing; review Tools > TX pacing before transmitting.";}}secondary_.pacing(pacing_[1]);}
void UartWorkbench::load_binary(){for(size_t i=0;i<2;++i){binary_[i]=BinarySlot{};binary_[i].fields=defaults();std::vector<wchar_t> raw(32768);const auto n=GetPrivateProfileStringW(section(i),L"Configuration",L"",raw.data(),static_cast<DWORD>(raw.size()),ini_.c_str());if(!n)continue;std::wstring error;try{if(n>=raw.size()-1)throw std::runtime_error("Binary settings truncated.");const auto f=decode(raw.data());if(!apply_binary(i,f,error,false))throw std::runtime_error("Saved binary schema is invalid.");}catch(...){binary_[i].enabled=false;groups_[i].save_error=L"Invalid binary settings; review Tools > Binary parser.";}}}
bool UartWorkbench::apply_binary(size_t i,const std::vector<std::wstring>& f,std::wstring& error,bool persist){
    if(shutdown_||f.size()!=12){error=L"Workbench is closing or settings are invalid.";return false;}const bool enabled=f[0]==L"Enabled";BinaryFrameConfig config;
    if(enabled&&!config_from(f,config,error))return false;
    if(persist){Request check;check.action=Request::CheckBinary;check.flag=enabled;if(!request(check)){error=L"Finish offline/profile work and clear text-parser Time field/groups before using shared PC-receipt binary charts.";return false;}
        try{const auto value=encode(f);if(!WritePrivateProfileStringW(section(i),L"Configuration",value.c_str(),ini_.c_str())){error=L"Cannot save binary settings; previous configuration retained.";return false;}}catch(const std::exception& e){error=uart_wide(e.what());return false;}}
    auto& slot=binary_[i];if(enabled){std::string detail;if(!slot.decoder.configure(std::move(config),detail)){error=uart_wide(detail);return false;}}
    slot.enabled=enabled;slot.fields=f;slot.fields[11].clear();const auto info=(i==0?*primary_:secondary_.traffic).after(UINT64_MAX,1);slot.sequence=info.newest;slot.revision=info.revision;slot.frames=slot.lost=0;decoded_.clear();decoded_bytes_=0;
    if(persist){Request reset;reset.action=Request::ResetNumeric;request(reset);}return true;
}
void UartWorkbench::binary_dialog(size_t i){auto values=binary_[i].fields.empty()?defaults():binary_[i].fields;
    std::vector<UartToolForm::Field> fields={
        {L"Use for charts",values[0],{L"Disabled",L"Enabled"}},{L"Header HEX",values[1]},
        {L"Frame bytes (0=length)",values[2]},{L"Length field offset",values[3]},{L"Length field bytes",values[4],{L"1",L"2",L"4"}},
        {L"Length byte order",values[5],{L"Little",L"Big"}},{L"Add to length field",values[6]},
        {L"Checksum",values[7],{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"}},
        {L"Checksum start offset",values[8]},{L"Checksum byte order",values[9],{L"Little",L"Big"}},
        {L"Fields: name,offset,type",values[10],{},6},{L"Preview HEX",L"",{},3}};
    UartToolForm form(i==0?L"Binary parser - Port A":L"Binary parser - Port B",
        L"Byte offsets are zero-based decimal. Types: u8/i8, u16/i16/u32/i32/f32/f64 with le/be suffix. Optional CSV scale,add. Checksum occupies final bytes. Apply starts a new numeric capture; raw recording stays unchanged.",fields,
        [](const auto& f){BinaryFrameConfig config;std::wstring error;if(!config_from(f,config,error))return error;std::string input;if(!hex_input(f[11],input))return std::wstring(L"Invalid preview HEX.");BinaryFrameDecoder decoder;std::string detail;decoder.configure(config,detail);const auto frames=decoder.feed(input,0);std::wostringstream out;out<<frames.size()<<L" complete frames; CRC errors "<<decoder.checksum_errors();for(const auto& frame:frames)for(const auto& value:frame.values)out<<L"\r\n"<<uart_wide(value.first)<<L" = "<<value.second;return out.str();},
        [](const auto& f,std::wstring& error){if(f[0]==L"Disabled")return true;BinaryFrameConfig config;return config_from(f,config,error);},this);
    if(form.DoModal()==IDOK&&::IsWindow(m_hWnd)){std::wstring error;if(!apply_binary(i,form.values(),error))warn(error);}
}
void UartWorkbench::decode_binary(){
    for(size_t i=0;i<2;++i){auto& slot=binary_[i];if(!slot.enabled)continue;auto& store=i==0?*primary_:secondary_.traffic;if(numeric_suspended(i)){const auto info=store.after(UINT64_MAX,1);slot.sequence=info.newest;slot.revision=info.revision;slot.decoder.reset();continue;}const auto data=store.after(slot.sequence,32);
        const auto start=GetTickCount64();
        if(slot.revision!=data.revision){slot.decoder.reset();slot.revision=data.revision;slot.sequence=0;}
        const auto batch=slot.sequence==0?store.after(0,32):data;
        if(slot.sequence&&!batch.records.empty()&&batch.records.front().sequence>slot.sequence+1){slot.decoder.reset();++slot.lost;}
        const auto accept=[&](std::vector<BinaryFrame> frames){for(auto& frame:frames){if(frame.generation==UINT64_MAX)frame.generation=slot.generation;++slot.frames;decoded_bytes_+=frame.bytes.size();decoded_.push_back({i,std::move(frame)});while(decoded_.size()>256||decoded_bytes_>2*1024*1024){decoded_bytes_-=decoded_.front().frame.bytes.size();++binary_[decoded_.front().port].lost;decoded_.pop_front();}}};
        for(const auto& record:batch.records){if(GetTickCount64()-start>=4)break;slot.sequence=record.sequence;if(!record.tx){if(slot.generation!=record.generation){if(slot.decoder.pending_bytes())++slot.lost;slot.decoder.interrupt();slot.generation=record.generation;}auto frames=slot.decoder.feed(record.bytes,record.receipt_ms,record.sample_seconds);for(auto& frame:frames)frame.generation=record.generation;accept(std::move(frames));}}
        if(GetTickCount64()-start<4)accept(slot.decoder.feed({},0));
    }
}
std::vector<UartWorkbench::Decoded> UartWorkbench::take_binary(){std::vector<Decoded> result;while(!decoded_.empty()&&result.size()<64){decoded_bytes_-=decoded_.front().frame.bytes.size();result.push_back(std::move(decoded_.front()));decoded_.pop_front();}return result;}
