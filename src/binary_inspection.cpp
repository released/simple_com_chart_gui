#include "binary_inspection.h"
#include "uart_files.h"
#include "command_panel.h"
#include <charconv>
#include <sstream>
#include <iomanip>
#include <algorithm>
namespace {
bool integer(const std::string& s,unsigned& value){const auto p=std::from_chars(s.data(),s.data()+s.size(),value);return p.ec==std::errc{}&&p.ptr==s.data()+s.size();}
std::string number(double n){std::ostringstream out;out<<std::setprecision(12)<<n;return out.str();}
}
bool parse_bit_fields(std::string_view text,std::vector<BitField>& fields,std::string& error){
    if(text.size()>8192){error="Bit definitions exceed 8 KiB.";return false;}if(text.empty()){fields.clear();return true;}
    std::vector<std::vector<std::string>> rows;if(!uart_csv_rows(text,rows,error))return false;std::vector<BitField> next;
    for(const auto& row:rows){if(row.size()!=6||row[0].empty()||row[0].size()>128||next.size()>=32){error="Use name,offset,unsigned_type,first_bit,bit_count,enum; max 32 fields.";return false;}BitField bit;bit.field.name=row[0];bit.field.type=row[2];unsigned offset=0;
        if((row[2]!="u8"&&row[2]!="u16le"&&row[2]!="u16be"&&row[2]!="u32le"&&row[2]!="u32be")||!integer(row[1],offset)||offset>65535||!integer(row[3],bit.first)||!integer(row[4],bit.width)||!bit.width||bit.width>32||bit.first>31||bit.first+bit.width>binary_field_width(row[2])*8){error="Invalid bit offset, type or width.";return false;}bit.field.offset=offset;
        std::istringstream enums(row[5]);std::string pair;while(std::getline(enums,pair,'|')){const auto at=pair.find('=');unsigned key=0;if(at==pair.npos||!integer(pair.substr(0,at),key)||pair.size()-at-1>64||!bit.names.emplace(key,pair.substr(at+1)).second||(bit.width<32&&key>=(uint32_t{1}<<bit.width))){error="Enums use unique value=label pairs separated by |.";return false;}}
        next.push_back(std::move(bit));
    }fields=std::move(next);return true;
}
std::vector<InspectionRow> inspect_binary(std::string_view bytes,const BinaryFrameConfig& config,const std::vector<BitField>& bits){
    std::vector<InspectionRow> rows;const auto add=[&](const std::vector<BinaryField>& fields){for(const auto& field:fields){double value=0;const bool ok=read_binary_field(bytes,field,value);rows.push_back({field.name,field.type,ok?number(value):"n/a: field outside frame or non-finite",field.offset,binary_field_width(field.type),ok});}};add(config.fields);
    if(!config.variants.empty()){BinaryField id;id.name="@id";id.offset=config.id_offset;id.type=config.id_width==1?"u8":std::string(config.id_width==2?"u16":"u32")+(config.id_big_endian?"be":"le");double value=0;const bool valid=read_binary_field(bytes,id,value);add({id});if(valid){const auto found=std::find_if(config.variants.begin(),config.variants.end(),[&](const auto& v){return v.id==value;});if(found!=config.variants.end())add(found->fields);else rows.back().value+=" (unknown type)";}}
    for(const auto& bit:bits){double raw=0;const bool ok=read_binary_field(bytes,bit.field,raw);std::string value="n/a: field outside frame";if(ok){const auto mask=bit.width==32?UINT32_MAX:(uint32_t{1}<<bit.width)-1;const auto v=(static_cast<uint32_t>(raw)>>bit.first)&mask;value=std::to_string(v);const auto name=bit.names.find(v);if(name!=bit.names.end())value+=" = "+name->second;}rows.push_back({bit.field.name,bit.field.type+" bits "+std::to_string(bit.first)+":"+std::to_string(bit.width),value,bit.field.offset,binary_field_width(bit.field.type),ok});}return rows;
}
std::string inspect_checksum(std::string_view bytes,const BinaryFrameConfig& config){
    const auto width=binary_checksum_width(config);if(!width)return "No checksum configured.";
    if(bytes.size()<width||config.checksum_from>bytes.size()-width||(config.checksum_count&&config.checksum_count>bytes.size()-width-config.checksum_from))return "Incomplete frame/checksum range.";
    const auto calculated=binary_checksum(bytes.substr(config.checksum_from,config.checksum_count?config.checksum_count:bytes.size()-width-config.checksum_from),config);uint32_t received=0;
    for(size_t n=0;n<width;++n){const auto shift=config.checksum_big_endian?(width-1-n)*8:n*8;received|=static_cast<uint32_t>(static_cast<unsigned char>(bytes[bytes.size()-width+n]))<<shift;}
    return std::string(calculated==received?"CRC/checksum PASS":"CRC/checksum FAIL")+" | calculated "+std::to_string(calculated)+" | received "+std::to_string(received)+" | range ["+std::to_string(config.checksum_from)+","+std::to_string(config.checksum_count?config.checksum_from+config.checksum_count:bytes.size()-width)+")";
}
