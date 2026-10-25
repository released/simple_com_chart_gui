#include "binary_frame.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>

namespace {
int count=0;
void check(bool value,const char* name){if(!value)throw std::runtime_error(name);++count;}
std::string bytes(std::initializer_list<unsigned> input){std::string out;for(auto b:input)out.push_back(static_cast<char>(b));return out;}
std::string seal(std::string value,FrameChecksum kind,bool big=false){const auto crc=frame_checksum(value,kind);const auto width=frame_checksum_width(kind);for(size_t i=0;i<width;++i){const auto shift=8*(big?width-1-i:i);value.push_back(static_cast<char>(crc>>shift));}return value;}
}
int main(){try{
    check(frame_checksum("123456789",FrameChecksum::Modbus16)==0x4B37,"Modbus CRC16 known vector");
    check(frame_checksum("123456789",FrameChecksum::Ccitt16)==0x29B1,"CCITT-FALSE CRC16 known vector");
    check(frame_checksum("123456789",FrameChecksum::Crc32)==0xCBF43926,"CRC32 known vector");
    check(frame_checksum("123456789",FrameChecksum::Sum8)==0xDD&&frame_checksum("123456789",FrameChecksum::Xor8)==0x31,"sum and xor known vectors");
    BinaryFrameConfig config;config.header=bytes({0xAA,0x55});config.fixed_bytes=12;config.checksum=FrameChecksum::Modbus16;std::string error;
    check(parse_binary_fields("rpm,2,u16le\ntemp,4,i16be,0.1\nvalue,6,f32le",config.fields,error),"typed CSV field schema");
    const auto frame=seal(bytes({0xAA,0x55,0xE8,0x03,0xFF,0x38,0x00,0x00,0xC0,0x3F}),config.checksum);
    for(size_t split=0;split<=frame.size();++split){BinaryFrameDecoder decoder;check(decoder.configure(config,error),"valid frame configuration");auto first=decoder.feed(std::string_view(frame).substr(0,split),100);const auto rest=decoder.feed(std::string_view(frame).substr(split),200);first.insert(first.end(),rest.begin(),rest.end());
        check(first.size()==1&&first[0].bytes==frame,"arbitrary driver read split preserves exact frame");check(first[0].values.size()==3&&first[0].values[0].second==1000&&std::abs(first[0].values[1].second+20)<1e-6&&first[0].values[2].second==1.5,"endian signed scaled float fields decoded correctly");check(first[0].receipt_ms==(split==frame.size()?100:200),"frame timestamp belongs to last received byte");}
    BinaryFrameDecoder decoder;check(decoder.configure(config,error),"configure before noise");
    const auto noisy=decoder.feed("noise"+frame+frame,300);check(noisy.size()==2&&decoder.discarded()==5,"header resynchronization and concatenated frames");
    auto bad=frame;bad[6]^=1;const auto recovered=decoder.feed(bad+frame,400);check(recovered.size()==1&&recovered[0].values[0].second==1000&&decoder.checksum_errors()>0,"bad CRC never creates numeric values and parser recovers");
    BinaryFrameConfig invalid=config;invalid.fields.push_back(invalid.fields[0]);check(!decoder.configure(invalid,error),"duplicate field rejected without changing active configuration");check(decoder.feed(frame,500).size()==1,"invalid Apply preserves prior decoder");
    invalid=config;invalid.fields[0].offset=11;check(!validate_binary_config(invalid,error),"field cannot overlap checksum");invalid=config;invalid.fields[0].scale=std::numeric_limits<double>::infinity();check(!validate_binary_config(invalid,error),"non-finite scale rejected");
    invalid=config;invalid.checksum=static_cast<FrameChecksum>(200);check(!validate_binary_config(invalid,error),"unknown checksum rejected");
    auto fields=config.fields;check(!parse_binary_fields("duplicate,0,u8\nduplicate,1,u8",fields,error),"duplicate schema names rejected");check(fields.size()==3,"failed schema parse preserves caller data");check(!parse_binary_fields("x,0,unknown",fields,error),"unknown scalar type rejected");
    BinaryFrameConfig variable;variable.header=bytes({0xA5});variable.length_offset=1;variable.length_width=1;variable.length_add=2;variable.fields={{"voltage","u16be",2,1,0}};check(decoder.configure(variable,error),"length-field configuration");
    check(decoder.feed(bytes({0xA5,2,1}),600).empty(),"partial variable frame waits");const auto both=decoder.feed(bytes({0xF4,0xA5,2,3,0xE8}),700);check(both.size()==2&&both[0].values[0].second==500&&both[1].values[0].second==1000,"variable lengths parse across read boundary");
    BinaryFrameConfig fixed;fixed.fixed_bytes=2;fixed.fields={{"sample","u16le",0,1,0}};check(decoder.configure(fixed,error),"headerless fixed-size stream supported");std::string many;for(unsigned i=0;i<1000;++i)many+=bytes({i&255,i>>8});
    auto items=decoder.feed(many,777,5.5);size_t total=items.size();check(total==256,"per-dispatch output budget bounded");for(const auto& item:items)check(item.receipt_ms==777&&item.sample_seconds==5.5,"initial receipt provenance");while(total<1000){items=decoder.feed({},999,9.5);check(!items.empty(),"backlog drain makes progress");for(const auto& item:items)check(item.receipt_ms==777&&item.sample_seconds==5.5,"backlog drain does not invent later receipt time");total+=items.size();}check(total==1000,"bounded dispatch drains complete source without sample loss");
    config.fields={{"value","f64le",2,1,0}};check(decoder.configure(config,error),"float64 schema");const auto nan=seal(bytes({0xAA,0x55,0,0,0,0,0,0,0xF8,0x7F}),config.checksum);const auto rejected=decoder.feed(nan,800);check(rejected.size()==1&&rejected[0].values.empty()&&decoder.layout_errors()==1,"NaN retains raw frame without fabricated chart samples");
    BinaryFrameConfig attack;attack.header=bytes({0xAA});attack.fixed_bytes=4;attack.checksum=FrameChecksum::Modbus16;attack.fields={{"x","u8",1,1,0}};check(decoder.configure(attack,error),"bounded malformed stream configuration");decoder.feed(std::string(131072,static_cast<char>(0xAA)),900);check(decoder.checksum_errors()<=4096&&decoder.discarded()<=4096,"malformed stream has bounded per-dispatch work");
    attack.fixed_bytes=65536;check(decoder.configure(attack,error),"large-frame work budget configuration");decoder.feed(std::string(131072,static_cast<char>(0xAA)),901);check(decoder.checksum_errors()<=1,"CRC work is bounded by bytes, not only frame count");
    std::cout<<"PASS: "<<count<<" binary framing/type/CRC/bounds/receipt checks.\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
