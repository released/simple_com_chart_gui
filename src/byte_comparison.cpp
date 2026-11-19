#include "byte_comparison.h"
#include <algorithm>
#include <stdexcept>
std::vector<ByteDifference> compare_bytes(std::string_view a,std::string_view b,size_t x,size_t y){
    if(a.size()>4096||b.size()>4096||x>a.size()||y>b.size())throw std::runtime_error("Compare two snapshots up to 4096 bytes each; offsets must be inside the snapshots.");
    std::vector<ByteDifference> result;for(size_t i=0;i<std::max(a.size()-x,b.size()-y);++i){const int p=i<a.size()-x?static_cast<unsigned char>(a[x+i]):-1,q=i<b.size()-y?static_cast<unsigned char>(b[y+i]):-1;if(p!=q)result.push_back({i,p,q});}return result;
}
