#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <array>
struct ByteDifference {size_t offset=0;int a=-1,b=-1;};
struct ByteComparisonRequest {size_t frame_a=0,frame_b=0,offset_a=0,offset_b=0;};
bool parse_byte_comparison_request(const std::array<std::wstring,4>&,size_t frame_count,ByteComparisonRequest&);
std::vector<ByteDifference> compare_bytes(std::string_view,std::string_view,size_t offset_a=0,size_t offset_b=0);
