#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
struct CrcSpec {unsigned width=16;uint32_t polynomial=0x1021,initial=0xFFFF,xor_out=0;bool reflect_in=false,reflect_out=false;};
CrcSpec parse_crc_spec(std::string_view);
std::string encode_crc_spec(const CrcSpec&);
class CustomCrc {
public:
    explicit CustomCrc(CrcSpec={});
    uint32_t calculate(std::string_view)const;
private:CrcSpec spec_;uint32_t mask_=0;std::array<uint32_t,256> table_{};
};
