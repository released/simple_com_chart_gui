#include "binary_frame.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <set>

namespace {
constexpr size_t max_frame=65536,max_fields=64;
struct Type {size_t bytes=0;bool big=false,signed_value=false,floating=false;};
Type field_type(const std::string& name){
    if(name=="u8")return {1,false,false,false};if(name=="i8")return {1,false,true,false};
    if(name.size()<4)return {};
    const bool big=name.substr(name.size()-2)=="be";if(!big&&name.substr(name.size()-2)!="le")return {};
    const auto prefix=name.substr(0,name.size()-2);
    if(prefix=="u16"||prefix=="i16")return {2,big,prefix[0]=='i',false};
    if(prefix=="u32"||prefix=="i32")return {4,big,prefix[0]=='i',false};
    if(prefix=="f32")return {4,big,false,true};if(prefix=="f64")return {8,big,false,true};return {};
}
std::string trim(std::string_view text){const auto first=text.find_first_not_of(" \t\r\n"),last=text.find_last_not_of(" \t\r\n");return first==std::string_view::npos?"":std::string(text.substr(first,last-first+1));}
bool integer(std::string_view text,size_t& value){uint64_t n=0;const auto result=std::from_chars(text.data(),text.data()+text.size(),n);if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||n>max_frame)return false;value=static_cast<size_t>(n);return true;}
bool scalar(std::string_view text,double& value){const auto result=std::from_chars(text.data(),text.data()+text.size(),value);return result.ec==std::errc{}&&result.ptr==text.data()+text.size()&&std::isfinite(value);}
uint64_t unsigned_value(std::string_view bytes,bool big){uint64_t out=0;for(size_t i=0;i<bytes.size();++i){const auto p=big?i:bytes.size()-1-i;out=(out<<8)|static_cast<unsigned char>(bytes[p]);}return out;}
double field_value(std::string_view bytes,const Type& type){
    const auto bits=unsigned_value(bytes,type.big);
    if(type.floating){if(type.bytes==4){const auto raw=static_cast<uint32_t>(bits);float value;std::memcpy(&value,&raw,4);return value;}double value;std::memcpy(&value,&bits,8);return value;}
    if(type.signed_value&&(bits&(uint64_t{1}<<(type.bytes*8-1))))return static_cast<double>(bits)-std::ldexp(1.0,static_cast<int>(type.bytes*8));return static_cast<double>(bits);
}
}
size_t binary_field_width(const std::string& type){return field_type(type).bytes;}
bool read_binary_field(std::string_view bytes,const BinaryField& field,double& value){const auto type=field_type(field.type);if(!type.bytes||field.offset>bytes.size()||type.bytes>bytes.size()-field.offset)return false;value=field_value(bytes.substr(field.offset,type.bytes),type)*field.scale+field.add;return std::isfinite(value);}
size_t frame_checksum_width(FrameChecksum kind){return kind==FrameChecksum::None?0:kind==FrameChecksum::Crc32?4:kind==FrameChecksum::Modbus16||kind==FrameChecksum::Ccitt16?2:1;}
uint32_t frame_checksum(std::string_view bytes,FrameChecksum kind){
    uint32_t value=kind==FrameChecksum::Modbus16||kind==FrameChecksum::Ccitt16?0xFFFF:kind==FrameChecksum::Crc32?0xFFFFFFFF:0;
    for(const unsigned char byte:bytes){
        if(kind==FrameChecksum::Sum8)value=(value+byte)&255;
        else if(kind==FrameChecksum::Xor8)value^=byte;
        else if(kind==FrameChecksum::Modbus16||kind==FrameChecksum::Crc32){value^=byte;const uint32_t poly=kind==FrameChecksum::Modbus16?0xA001:0xEDB88320;for(int bit=0;bit<8;++bit)value=(value>>1)^((value&1)?poly:0);}
        else if(kind==FrameChecksum::Ccitt16){value^=static_cast<uint32_t>(byte)<<8;for(int bit=0;bit<8;++bit)value=((value<<1)^((value&0x8000)?0x1021:0))&0xFFFF;}
    }return kind==FrameChecksum::Crc32?value^0xFFFFFFFF:value;
}
bool parse_binary_fields(std::string_view csv,std::vector<BinaryField>& fields,std::string& error){
    if(csv.size()>16384){error="Field schema exceeds 16 KiB.";return false;}std::vector<BinaryField> result;size_t at=0;
    while(at<csv.size()){auto end=csv.find('\n',at);if(end==std::string_view::npos)end=csv.size();const auto line=trim(csv.substr(at,end-at));at=end+1;if(line.empty())continue;
        std::vector<std::string> parts;size_t begin=0;do{auto comma=line.find(',',begin);if(comma==std::string::npos)comma=line.size();parts.push_back(trim(std::string_view(line).substr(begin,comma-begin)));begin=comma+1;}while(begin<=line.size());
        if(parts.size()<3||parts.size()>5){error="Use name,offset,type[,scale[,add]] on each line.";return false;}BinaryField field;field.name=parts[0];field.type=parts[2];
        if(!integer(parts[1],field.offset)||(parts.size()>3&&!scalar(parts[3],field.scale))||(parts.size()>4&&!scalar(parts[4],field.add))){error="Invalid field offset/scale/add.";return false;}result.push_back(std::move(field));if(result.size()>max_fields){error="At most 64 fields are allowed.";return false;}
    }
    BinaryFrameConfig probe;probe.fixed_bytes=max_frame;probe.fields=result;if(!validate_binary_config(probe,error))return false;fields=std::move(result);return true;
}
bool validate_binary_config(const BinaryFrameConfig& config,std::string& error){
    if(static_cast<int>(config.checksum)<0||static_cast<int>(config.checksum)>5){error="Unsupported checksum.";return false;}
    if(config.header.size()>32||config.fixed_bytes>max_frame||config.length_offset>max_frame-4||config.length_add>max_frame||config.checksum_from>max_frame){error="Frame/header/range exceeds supported limits.";return false;}
    if(!config.fixed_bytes&&(config.header.empty()||(config.length_width!=1&&config.length_width!=2&&config.length_width!=4))){error="Variable frames require a header and a 1/2/4-byte length field.";return false;}
    const size_t minimum=std::max(config.header.size(),config.fixed_bytes?size_t{0}:config.length_offset+config.length_width);
    if(config.fixed_bytes&&(config.fixed_bytes<minimum+frame_checksum_width(config.checksum)||config.checksum_from>config.fixed_bytes-frame_checksum_width(config.checksum))){error="Frame is too short for its header/checksum.";return false;}
    if(config.fields.empty()||config.fields.size()>max_fields){error="Configure 1..64 fields.";return false;}std::set<std::string> names;
    for(const auto& field:config.fields){const auto type=field_type(field.type);if(field.name.empty()||field.name.size()>64||field.name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-[]")!=std::string::npos||!names.insert(field.name).second||!type.bytes||field.offset>max_frame-type.bytes||!std::isfinite(field.scale)||!std::isfinite(field.add)){error="Invalid or duplicate field name/type/range.";return false;}
        if(config.fixed_bytes&&field.offset+type.bytes>config.fixed_bytes-frame_checksum_width(config.checksum)){error="A field overlaps the checksum or extends beyond the frame.";return false;}}
    return true;
}
bool BinaryFrameDecoder::configure(BinaryFrameConfig config,std::string& error){if(!validate_binary_config(config,error))return false;config_=std::move(config);configured_=true;reset();return true;}
void BinaryFrameDecoder::reset(){pending_.clear();receipts_.clear();discarded_=checksum_errors_=layout_errors_=0;}
std::vector<BinaryFrame> BinaryFrameDecoder::feed(std::string_view bytes,int64_t receipt_ms,double sample_seconds){
    std::vector<BinaryFrame> out;if(!configured_)return out;
    if(bytes.size()>max_frame*2){discarded_+=pending_.size()+bytes.size()-max_frame*2;pending_.clear();receipts_.clear();bytes.remove_prefix(bytes.size()-max_frame*2);}
    pending_.append(bytes);if(!bytes.empty()){if(!receipts_.empty()&&receipts_.back().ms==receipt_ms&&receipts_.back().seconds==sample_seconds)receipts_.back().count+=bytes.size();else receipts_.push_back({bytes.size(),receipt_ms,sample_seconds});}
    size_t head=0;
    const auto consume=[&](size_t count,bool discard){head+=count;if(discard)discarded_+=count;while(count&&!receipts_.empty()){const auto n=std::min(count,receipts_.front().count);count-=n;receipts_.front().count-=n;if(!receipts_.front().count)receipts_.pop_front();}};
    const auto completion_time=[&](size_t count){for(const auto& r:receipts_){if(count<=r.count)return r;count-=r.count;}return Receipt{0,receipt_ms,sample_seconds};};
    if(pending_.size()>max_frame*2)consume(pending_.size()-max_frame*2,true);
    size_t iterations=0,checked_bytes=0;
    while(head<pending_.size()&&out.size()<256&&++iterations<=4096){
        auto available=std::string_view(pending_).substr(head);
        if(!config_.header.empty()){const auto at=available.find(config_.header);if(at==std::string::npos){const auto keep=std::min(available.size(),config_.header.size()-1);consume(available.size()-keep,true);break;}if(at){consume(at,true);available=std::string_view(pending_).substr(head);}}
        size_t total=config_.fixed_bytes;
        if(!total){if(available.size()<config_.length_offset+config_.length_width)break;const auto length=unsigned_value(available.substr(config_.length_offset,config_.length_width),config_.length_big_endian);
            if(length>max_frame||length+config_.length_add>max_frame){++layout_errors_;consume(1,true);continue;}total=static_cast<size_t>(length)+config_.length_add;}
        const auto check_width=frame_checksum_width(config_.checksum);const auto minimum=std::max(config_.header.size(),config_.fixed_bytes?size_t{0}:config_.length_offset+config_.length_width);
        if(total<minimum+check_width||total<=config_.checksum_from||!total){++layout_errors_;consume(1,true);continue;}
        if(available.size()<total)break;
        if(checked_bytes&&total>max_frame-checked_bytes)break;checked_bytes+=total;
        const auto frame=available.substr(0,total);
        if(check_width&&(config_.checksum_from>total-check_width||frame_checksum(frame.substr(config_.checksum_from,total-check_width-config_.checksum_from),config_.checksum)!=unsigned_value(frame.substr(total-check_width),config_.checksum_big_endian))){++checksum_errors_;consume(1,true);continue;}
        BinaryFrame decoded;decoded.bytes.assign(frame);const auto time=completion_time(total);decoded.receipt_ms=time.ms;decoded.sample_seconds=time.seconds;bool valid=true;
        for(const auto& field:config_.fields){const auto type=field_type(field.type);if(field.offset+type.bytes>total-check_width){valid=false;break;}const double value=field_value(frame.substr(field.offset,type.bytes),type)*field.scale+field.add;if(!std::isfinite(value)){valid=false;break;}decoded.values.emplace_back(field.name,value);}
        if(!valid){decoded.values.clear();++layout_errors_;}out.push_back(std::move(decoded));consume(total,false);
    }if(head)pending_.erase(0,head);return out;
}
