#include "command_panel.h"
#include "uart_files.h"
#include "message_format.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
std::string decimal(double value,int precision=12){std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(precision)<<value;return out.str();}
bool scalar(std::string_view text,double& value){const auto result=std::from_chars(text.data(),text.data()+text.size(),value);return result.ec==std::errc{}&&result.ptr==text.data()+text.size()&&std::isfinite(value);}
bool expanded(const PanelCommand& command,double value,std::string& body,std::string& error){
    body.clear();for(size_t at=0;at<command.body.size();){if(command.body.compare(at,6,"{value")!=0){body.push_back(command.body[at++]);continue;}
        const auto end=command.body.find('}',at);if(end==std::string::npos){error="Unclosed value placeholder.";return false;}const auto token=command.body.substr(at+1,end-at-1);at=end+1;
        if(token=="value"&&!command.hex){body+=decimal(value);continue;}
        const auto type=token=="value"?"u8":token.rfind("value:",0)==0?token.substr(6):"";
        size_t width=type=="u8"||type=="i8"?1:type=="u16le"||type=="u16be"||type=="i16le"||type=="i16be"?2:type=="u32le"||type=="u32be"||type=="i32le"||type=="i32be"?4:0;
        if(!command.hex||!width||value!=std::floor(value)){error="HEX placeholders require an integer u8/i8/u16/i16/u32/i32 value.";return false;}
        const bool signed_value=type[0]=='i',big=type.size()>2&&type.substr(type.size()-2)=="be";const double low=signed_value?-std::ldexp(1.0,static_cast<int>(width*8-1)):0,high=signed_value?std::ldexp(1.0,static_cast<int>(width*8-1))-1:std::ldexp(1.0,static_cast<int>(width*8))-1;
        if(value<low||value>high){error="Value does not fit the selected binary type.";return false;}const auto bits=static_cast<uint64_t>(static_cast<int64_t>(value));std::string raw;
        for(size_t i=0;i<width;++i)raw.push_back(static_cast<char>(bits>>(8*(big?width-1-i:i))));body+=uart_hex(raw);
    }return true;
}
}
std::string uart_csv_cell(std::string_view value){std::string out="\"";for(const char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';}
bool uart_csv_rows(std::string_view text,std::vector<std::vector<std::string>>& result,std::string& error){
    return uart_csv_table(text,result,error,128*1024,256,20);
}
bool uart_csv_table(std::string_view text,std::vector<std::vector<std::string>>& result,std::string& error,size_t max_bytes,size_t max_rows,size_t max_columns){
    if(text.size()>max_bytes||text.find('\0')!=std::string_view::npos){error="CSV exceeds its byte limit or contains NUL.";return false;}
    std::vector<std::vector<std::string>> rows;std::vector<std::string> row;std::string field;bool quoted=false,closed=false,started=false;
    const auto finish_field=[&]{row.push_back(std::move(field));field.clear();closed=started=false;};
    const auto finish_row=[&]{finish_field();if(!(row.size()==1&&row[0].empty()))rows.push_back(std::move(row));row.clear();};
    for(size_t at=0;at<text.size();++at){const char c=text[at];if(quoted){if(c=='\"'){if(at+1<text.size()&&text[at+1]=='\"'){field+='\"';++at;}else{quoted=false;closed=true;}}else field+=c;}
        else if(c==',')finish_field();else if(c=='\r'||c=='\n'){if(c=='\r'&&at+1<text.size()&&text[at+1]=='\n')++at;finish_row();}
        else if(c=='\"'&&!started){quoted=true;started=true;}else if(closed){error="Unexpected characters after a quoted field.";return false;}else{if(c=='\"'){error="Quote inside an unquoted field.";return false;}field+=c;started=true;}
        if(field.size()>32768||row.size()>max_columns||rows.size()>max_rows){error="CSV row/field limit exceeded.";return false;}}
    if(quoted){error="Unclosed CSV quote.";return false;}if(started||closed||!field.empty()||!row.empty())finish_row();
    if(rows.size()>max_rows||std::any_of(rows.begin(),rows.end(),[&](const auto& row){return row.size()>max_columns;})){error="CSV row/column limit exceeded.";return false;}result=std::move(rows);return true;
}
bool panel_payload(const PanelCommand& command,double value,std::string& bytes,std::string& error){
    if(!std::isfinite(value)||value<command.minimum||value>command.maximum){error="Value is outside the command range.";return false;}
    std::string body;if(!expanded(command,value,body,error))return false;TxEntry tx;
    try{tx.text=uart_wide(body);}catch(...){error="Command must be valid UTF-8.";return false;}tx.hex=command.hex;
    std::wstring detail;std::string payload;if(!body.empty()&&!make_tx_payload(tx,payload,detail)){error=message_utf8(detail);return false;}
    const auto checksum=frame_checksum(payload,command.checksum);const auto width=frame_checksum_width(command.checksum);
    for(size_t i=0;i<width;++i)payload.push_back(static_cast<char>(checksum>>(8*(command.checksum_big?width-1-i:i))));
    if(command.ending==TxEnding::CR||command.ending==TxEnding::CRLF)payload+='\r';if(command.ending==TxEnding::LF||command.ending==TxEnding::CRLF)payload+='\n';
    if(payload.empty()||payload.size()>4096){error="Payload including checksum/ending must be 1..4096 bytes.";return false;}bytes=std::move(payload);return true;
}
bool validate_panel(const std::vector<PanelCommand>& commands,std::string& error){
    if(commands.size()>128){error="At most 128 commands are supported.";return false;}std::set<std::pair<std::string,std::string>> names;size_t serialized=std::string_view("SCC_COMMANDS_V1\r\n").size();
    for(const auto& c:commands){if(c.group.size()>128||c.name.empty()||c.name.size()>128||c.body.size()>16384||c.group.find_first_of("\r\n\t")!=std::string::npos||c.name.find_first_of("\r\n\t")!=std::string::npos||!names.insert({c.group,c.name}).second||!std::isfinite(c.minimum)||!std::isfinite(c.maximum)||!std::isfinite(c.step)||c.minimum>c.maximum||c.minimum<-1e12||c.maximum>1e12||c.step<=0||c.step>1e12||static_cast<int>(c.ending)<0||static_cast<int>(c.ending)>3||static_cast<int>(c.checksum)<0||static_cast<int>(c.checksum)>5){error="Invalid command name/range/step/checksum or duplicate group/name.";return false;}
        if(std::any_of(c.group.begin(),c.group.end(),[](unsigned char b){return b<32||b==127;})||std::any_of(c.name.begin(),c.name.end(),[](unsigned char b){return b<32||b==127;})){error="Names cannot contain control characters.";return false;}
        try{uart_wide(c.group);uart_wide(c.name);}catch(...){error="Names must be valid UTF-8.";return false;}std::string payload;if(!panel_payload(c,c.value,payload,error))return false;
        for(const auto& field:std::vector<std::string>{c.group,c.name,c.hex?"HEX":"TEXT",c.body,std::to_string(static_cast<int>(c.ending)),std::to_string(static_cast<int>(c.checksum)),c.checksum_big?"1":"0",decimal(c.minimum,17),decimal(c.maximum,17),decimal(c.step,17),decimal(c.value,17)})serialized+=uart_csv_cell(field).size()+1;
        ++serialized;if(serialized>128*1024){error="Command library exceeds 128 KiB; previous definitions retained.";return false;}}
    return true;
}
std::string encode_panel(const std::vector<PanelCommand>& commands){std::string error;if(!validate_panel(commands,error))throw std::runtime_error(error);std::string out="SCC_COMMANDS_V1\r\n";
    for(const auto& c:commands){const std::vector<std::string> values={c.group,c.name,c.hex?"HEX":"TEXT",c.body,std::to_string(static_cast<int>(c.ending)),std::to_string(static_cast<int>(c.checksum)),c.checksum_big?"1":"0",decimal(c.minimum,17),decimal(c.maximum,17),decimal(c.step,17),decimal(c.value,17)};for(size_t i=0;i<values.size();++i){if(i)out+=',';out+=uart_csv_cell(values[i]);}out+="\r\n";}if(out.size()>128*1024)throw std::runtime_error("Command definition exceeds 128 KiB.");return out;
}
bool decode_panel(std::string_view text,std::vector<PanelCommand>& commands,std::string& error){
    constexpr std::string_view header="SCC_COMMANDS_V1\r\n";if(text.substr(0,header.size())!=header){error="Unsupported command definition version.";return false;}std::vector<std::vector<std::string>> rows;if(!uart_csv_rows(text.substr(header.size()),rows,error))return false;std::vector<PanelCommand> result;
    for(const auto& f:rows){if(f.size()!=11||(f[2]!="HEX"&&f[2]!="TEXT")||(f[6]!="0"&&f[6]!="1")){error="Invalid command definition columns.";return false;}PanelCommand c;c.group=f[0];c.name=f[1];c.hex=f[2]=="HEX";c.body=f[3];double ending=0,checksum=0;
        if(!scalar(f[4],ending)||ending!=std::floor(ending)||ending<0||ending>3||!scalar(f[5],checksum)||checksum!=std::floor(checksum)||checksum<0||checksum>5||!scalar(f[7],c.minimum)||!scalar(f[8],c.maximum)||!scalar(f[9],c.step)||!scalar(f[10],c.value)){error="Invalid numeric command settings.";return false;}c.ending=static_cast<TxEnding>(static_cast<int>(ending));c.checksum=static_cast<FrameChecksum>(static_cast<int>(checksum));c.checksum_big=f[6]=="1";result.push_back(std::move(c));}
    if(!validate_panel(result,error))return false;commands=std::move(result);return true;
}
