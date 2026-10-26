#include "packet_builder.h"
#include "uart_files.h"
#include <charconv>
#include <cmath>
#include <set>
#include <stdexcept>

namespace {
bool scalar(std::string_view s,double& v) {
    const auto r=std::from_chars(s.data(),s.data()+s.size(),v);
    return r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(v);
}
bool name_ok(const std::string& s) {
    return !s.empty()&&s.size()<=48&&s.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789")==s.npos;
}
}
std::vector<PacketParameterRow> packet_parameters(std::string_view input) {
    std::vector<std::vector<std::string>> rows;std::string error;
    if(!uart_csv_table(input,rows,error,16384,32,4))throw std::runtime_error(error);
    std::set<std::string> names;std::vector<PacketParameterRow> result;
    for(const auto& row:rows){double value=0,low=0,high=0;
        if(row.size()!=4||!name_ok(row[0])||row[0]=="length"||row[0]=="sequence"||!names.insert(row[0]).second||!scalar(row[1],value)||!scalar(row[2],low)||!scalar(row[3],high)||low>high||value<low||value>high)
            throw std::runtime_error("Parameters require unique name,value,min,max rows (max 32); length/sequence are reserved.");
        result.push_back({row[0],row[1],row[2],row[3]});}
    return result;
}
std::string encode_packet_parameters(const std::vector<PacketParameterRow>& rows){if(rows.size()>32)throw std::runtime_error("At most 32 parameters.");std::string out;for(const auto& r:rows){for(size_t c=0;c<4;++c){if(r[c].find_first_of(",\r\n\"\0",0,5)!=r[c].npos)throw std::runtime_error("Parameter cells cannot contain delimiters or controls.");if(c)out+=',';out+=r[c];}out+='\n';}packet_parameters(out);return out;}
bool packet_payload(const PacketDefinition& p,std::string& bytes,std::string& error) {
    if(p.command.name.size()>128||p.command.name.find_first_of("\r\n\0",0,3)!=p.command.name.npos){error="Packet name must be <=128 UTF-8 bytes without controls.";return false;}
    try{uart_wide(p.command.name);}catch(...){error="Packet name must be valid UTF-8.";return false;}
    if(p.command.body.size()>16384||static_cast<int>(p.codec)<0||static_cast<int>(p.codec)>2||
        static_cast<int>(p.command.ending)<0||static_cast<int>(p.command.ending)>3||
        static_cast<int>(p.command.checksum)<0||static_cast<int>(p.command.checksum)>5) {error="Packet template/codec is invalid.";return false;}
    std::vector<PacketParameterRow> rows;
    try{rows=packet_parameters(p.parameters);}catch(const std::exception& e){error=e.what();return false;}
    std::map<std::string,PanelCommand> parameters;
    for(const auto& row:rows) {
        PanelCommand value;value.minimum=value.maximum=value.value=0;
        scalar(row[1],value.value);scalar(row[2],value.minimum);scalar(row[3],value.maximum);
        parameters.emplace(row[0],value);
    }
    const auto expand=[&](size_t length,std::string& out)->bool {
        out.clear();
        for(size_t at=0;at<p.command.body.size();) {
            if(p.command.body[at]!='{'){out+=p.command.body[at++];continue;}
            const auto end=p.command.body.find('}',at+1);
            if(end==std::string::npos){error="Unclosed parameter placeholder.";return false;}
            const auto token=p.command.body.substr(at+1,end-at-1);const auto colon=token.find(':');
            const auto name=token.substr(0,colon);PanelCommand c;
            if(name=="length"||name=="sequence"){c.value=static_cast<double>(name=="length"?length:p.sequence);c.minimum=0;c.maximum=4294967295.0;}
            else {const auto it=parameters.find(name);if(it==parameters.end()){error="Unknown parameter: "+name;return false;}c=it->second;}
            c.hex=p.command.hex;
            c.body="{value"+(colon==token.npos?std::string{}:token.substr(colon))+"}";
            std::string value;
            if(!panel_payload(c,c.value,value,error))return false;
            out+=c.hex?uart_hex(value):value;at=end+1;
        }
        return true;
    };
    if(p.custom_crc&&p.command.checksum!=FrameChecksum::None){error="Set standard Checksum to None when using a custom CRC.";return false;}
    const auto emit=[&](const PanelCommand& input,std::string& payload)->bool {
        if(!p.custom_crc)return panel_payload(input,0,payload,error);
        try {
            PanelCommand bare=input;bare.checksum=FrameChecksum::None;bare.ending=TxEnding::None;
            if(bare.body.empty())payload.clear();else if(!panel_payload(bare,0,payload,error))return false;
            if(p.checksum_from>payload.size()||(p.checksum_count&&p.checksum_count>payload.size()-p.checksum_from))throw std::runtime_error("Custom CRC range exceeds expanded payload.");
            const auto checksum=CustomCrc(*p.custom_crc).calculate(std::string_view(payload).substr(p.checksum_from,p.checksum_count?p.checksum_count:payload.size()-p.checksum_from));
            const size_t width=p.custom_crc->width/8;for(size_t i=0;i<width;++i)payload+=static_cast<char>(checksum>>(8*(input.checksum_big?width-1-i:i)));
            if(input.ending==TxEnding::CR||input.ending==TxEnding::CRLF)payload+='\r';if(input.ending==TxEnding::LF||input.ending==TxEnding::CRLF)payload+='\n';
            if(payload.size()>4096)throw std::runtime_error("Custom CRC payload exceeds 4096 bytes.");return true;
        }catch(const std::exception& e){error=e.what();return false;}
    };
    PanelCommand c=p.command;c.minimum=c.maximum=c.value=0;
    if(c.name.empty())c.name="Packet";
    if(!expand(0,c.body))return false;
    std::string first;
    if(!emit(c,first))return false;
    // Length includes final checksum/ending, excludes optional wire encoding.
    size_t length=first.size();bool stable=false;
    for(int pass=0;pass<8;++pass) {
        if(!expand(length,c.body)||!emit(c,first))return false;
        if(first.size()==length){stable=true;break;}length=first.size();
    }
    if(!stable){error="Length substitution did not converge.";return false;}
    try{auto encoded=wire_encode(first,p.codec);if(encoded.empty()||encoded.size()>4096){error="Encoded packet exceeds the 4096-byte TX limit.";return false;}bytes=std::move(encoded);return true;}
    catch(const std::exception& e){error=e.what();return false;}
}
std::string encode_packet(const PacketDefinition& p) {
    std::string bytes,error;if(!packet_payload(p,bytes,error))throw std::runtime_error(error);
    // Keep legacy command/step formats untouched; laboratory definitions are separate.
    std::string out=p.custom_crc?"SCC_PACKET_V2\r\n":"SCC_PACKET_V1\r\n";
    std::vector<std::string> values{p.command.name,p.command.hex?"HEX":"TEXT",p.command.body,p.parameters,
        std::to_string(static_cast<int>(p.command.ending)),std::to_string(static_cast<int>(p.command.checksum)),p.command.checksum_big?"1":"0",std::to_string(p.sequence),std::to_string(static_cast<int>(p.codec))};
    if(p.custom_crc){values.push_back(encode_crc_spec(*p.custom_crc));values.push_back(std::to_string(p.checksum_from));values.push_back(std::to_string(p.checksum_count));}
    for(const auto& value:values)out+=uart_csv_cell(value)+',';
    out.pop_back();return out+"\r\n";
}
bool decode_packet(std::string_view input,PacketDefinition& result,std::string& error) {
    constexpr std::string_view header="SCC_PACKET_V1\r\n";
    const bool extended=input.substr(0,header.size())=="SCC_PACKET_V2\r\n";
    if(!extended&&input.substr(0,header.size())!=header){error="Unsupported packet definition.";return false;}
    std::vector<std::vector<std::string>> rows;if(!uart_csv_table(input.substr(header.size()),rows,error,65536,1,12))return false;
    if(rows.size()!=1||rows[0].size()!=(extended?12:9)){error="Invalid packet columns.";return false;}
    const auto& r=rows[0];PacketDefinition p;double ending=0,checksum=0,sequence=0,codec=0;
    if((r[1]!="HEX"&&r[1]!="TEXT")||(r[6]!="0"&&r[6]!="1")||!scalar(r[4],ending)||ending<0||ending>3||ending!=std::floor(ending)||
        !scalar(r[5],checksum)||checksum<0||checksum>5||checksum!=std::floor(checksum)||!scalar(r[7],sequence)||sequence<0||sequence>4294967295.0||sequence!=std::floor(sequence)||
        !scalar(r[8],codec)||codec<0||codec>2||codec!=std::floor(codec)){error="Invalid packet options.";return false;}
    p.command.name=r[0];p.command.hex=r[1]=="HEX";p.command.body=r[2];p.parameters=r[3];p.command.ending=static_cast<TxEnding>(static_cast<int>(ending));
    p.command.checksum=static_cast<FrameChecksum>(static_cast<int>(checksum));p.command.checksum_big=r[6]=="1";p.sequence=static_cast<uint32_t>(sequence);p.codec=static_cast<WireCodec>(static_cast<int>(codec));
    if(extended){try{p.custom_crc=parse_crc_spec(r[9]);double from=0,count=0;if(!scalar(r[10],from)||!scalar(r[11],count)||from<0||count<0||from>65536||count>65536||from!=std::floor(from)||count!=std::floor(count))throw std::runtime_error("Invalid custom CRC byte range.");p.checksum_from=static_cast<size_t>(from);p.checksum_count=static_cast<size_t>(count);}catch(const std::exception& e){error=e.what();return false;}}
    std::string bytes;if(!packet_payload(p,bytes,error))return false;result=std::move(p);return true;
}
