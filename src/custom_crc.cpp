#include "custom_crc.h"
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace {
uint32_t reflect(uint32_t n,unsigned width){uint32_t r=0;for(unsigned i=0;i<width;++i){r=(r<<1)|(n&1);n>>=1;}return r;}
uint32_t mask(unsigned w){return w==32?UINT32_MAX:(uint32_t{1}<<w)-1;}
void validate(const CrcSpec& s){if(s.width!=8&&s.width!=16&&s.width!=32)throw std::runtime_error("CRC width must be 8, 16 or 32.");const auto m=mask(s.width);if(!s.polynomial||!(s.polynomial&1)||s.polynomial>m||s.initial>m||s.xor_out>m)throw std::runtime_error("CRC polynomial must be nonzero/odd; HEX parameters must fit the width.");}
}
CrcSpec parse_crc_spec(std::string_view text){
    std::vector<std::string> parts;size_t p=0;do{const auto end=text.find(':',p);parts.emplace_back(text.substr(p,end==text.npos?text.npos:end-p));if(end==text.npos)break;p=end+1;}while(true);
    if(parts.size()!=6||text.size()>96)throw std::runtime_error("CRC spec: width:polyHEX:initHEX:xorHEX:reflectIn0or1:reflectOut0or1.");
    CrcSpec s;uint32_t numbers[6]{};for(size_t i=0;i<6;++i){const auto& v=parts[i];const auto r=std::from_chars(v.data(),v.data()+v.size(),numbers[i],i>=1&&i<=3?16:10);if(v.empty()||r.ec!=std::errc{}||r.ptr!=v.data()+v.size())throw std::runtime_error("Invalid CRC parameter.");}
    if(numbers[4]>1||numbers[5]>1)throw std::runtime_error("CRC reflection flags must be 0 or 1.");s.width=numbers[0];s.polynomial=numbers[1];s.initial=numbers[2];s.xor_out=numbers[3];s.reflect_in=numbers[4]!=0;s.reflect_out=numbers[5]!=0;validate(s);return s;
}
std::string encode_crc_spec(const CrcSpec& s){validate(s);std::ostringstream out;out<<s.width<<':'<<std::hex<<std::uppercase<<s.polynomial<<':'<<s.initial<<':'<<s.xor_out<<std::dec<<':'<<s.reflect_in<<':'<<s.reflect_out;return out.str();}
CustomCrc::CustomCrc(CrcSpec s):spec_(s){validate(s);mask_=mask(s.width);
    for(unsigned i=0;i<256;++i){uint32_t v=static_cast<uint32_t>(i)<<(s.width-8);for(int bit=0;bit<8;++bit)v=((v<<(1))^((v&(uint32_t{1}<<(s.width-1)))?s.polynomial:0))&mask_;table_[i]=v;}}
uint32_t CustomCrc::calculate(std::string_view bytes)const{uint32_t v=spec_.initial;for(unsigned char b:bytes){if(spec_.reflect_in)b=static_cast<unsigned char>(reflect(b,8));v=((v<<8)^table_[((v>>(spec_.width-8))^b)&255])&mask_;}if(spec_.reflect_out)v=reflect(v,spec_.width);return (v^spec_.xor_out)&mask_;}
