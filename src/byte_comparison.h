#pragma once
#include <string>
#include <string_view>
#include <vector>
struct ByteDifference {size_t offset=0;int a=-1,b=-1;};
std::vector<ByteDifference> compare_bytes(std::string_view,std::string_view,size_t offset_a=0,size_t offset_b=0);
