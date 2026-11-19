#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
struct MaskedPattern {std::vector<uint8_t> value,mask;};
bool parse_masked_pattern(std::wstring_view,MaskedPattern&,std::wstring&);
size_t find_masked(std::string_view,const MaskedPattern&);
