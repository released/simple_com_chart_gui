#include "binary_frame.h"
#include <algorithm>
#include <charconv>
#include <map>
#include <sstream>

bool parse_binary_layout(std::string_view text,BinaryFrameConfig& out,std::string& error) {
    if(text.size()>16384){error="Layout exceeds 16 KiB.";return false;}
    BinaryFrameConfig next=out;next.fields.clear();next.variants.clear();next.codec=WireCodec::None;next.custom_crc.reset();next.checksum_count=0;
    bool id_seen=false,codec_seen=false,crc_seen=false;std::map<uint32_t,std::vector<BinaryField>> variants;
    std::istringstream input{std::string(text)};std::string line,common;
    const auto integer=[](const std::string& s,uint32_t& v){const auto r=std::from_chars(s.data(),s.data()+s.size(),v);return r.ec==std::errc{}&&r.ptr==s.data()+s.size();};
    while(std::getline(input,line)) {
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        if(line.front()!='@'){common+=line+'\n';continue;}
        std::vector<std::string> fields;std::istringstream row(line);std::string f;while(std::getline(row,f,','))fields.push_back(f);
        if(fields[0]=="@crc"&&(fields.size()==7||fields.size()==8)&&!crc_seen) {
            crc_seen=true;std::string spec;for(size_t i=1;i<7;++i){if(i>1)spec+=':';spec+=fields[i];}
            try{next.custom_crc=parse_crc_spec(spec);}catch(const std::exception& e){error=e.what();return false;}
            uint32_t count=0;if(fields.size()==8&&(!integer(fields[7],count)||count>65536)){error="Invalid CRC covered byte count.";return false;}next.checksum_count=count;
        } else if(fields[0]=="@codec"&&fields.size()==2&&!codec_seen) {
            codec_seen=true;
            if(fields[1]=="None")next.codec=WireCodec::None;
            else if(fields[1]=="COBS")next.codec=WireCodec::Cobs;
            else if(fields[1]=="SLIP")next.codec=WireCodec::Slip;
            else{error="Codec must be None, COBS or SLIP.";return false;}
        } else if(fields[0]=="@id"&&fields.size()==4&&!id_seen) {
            uint32_t offset=0,width=0;
            if(!integer(fields[1],offset)||offset>65532||!integer(fields[2],width)||(width!=1&&width!=2&&width!=4)||(fields[3]!="Little"&&fields[3]!="Big")){error="Use @id,offset,1|2|4,Little|Big.";return false;}
            next.id_offset=offset;next.id_width=width;next.id_big_endian=fields[3]=="Big";id_seen=true;
        } else if(fields[0]=="@case"&&fields.size()>=5&&fields.size()<=7) {
            uint32_t id=0;if(!integer(fields[1],id)){error="Message IDs must be decimal unsigned integers.";return false;}
            std::string field;for(size_t i=2;i<fields.size();++i){if(i>2)field+=',';field+=fields[i];}
            std::vector<BinaryField> parsed;if(!parse_binary_fields(field,parsed,error)||parsed.size()!=1)return false;
            auto& list=variants[id];list.push_back(parsed.front());
            if(variants.size()>32||list.size()>64){error="At most 32 message types / 64 fields each.";return false;}
        } else {error="Unknown/duplicate layout directive.";return false;}
    }
    if(!common.empty()&&!parse_binary_fields(common,next.fields,error))return false;
    if(!variants.empty()&&!id_seen){error="Message cases require an @id directive.";return false;}
    for(auto& item:variants)next.variants.push_back({item.first,std::move(item.second)});
    if(!validate_binary_config(next,error))return false;out=std::move(next);return true;
}
