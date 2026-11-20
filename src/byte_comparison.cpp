#include "byte_comparison.h"
#include <algorithm>
#include <stdexcept>
namespace {
bool decimal(const std::wstring& text,size_t minimum,size_t maximum,size_t& result) {
    if(text.empty())return false;
    size_t value=0;
    for(const auto c:text){
        if(c<L'0'||c>L'9')return false;
        const auto digit=static_cast<size_t>(c-L'0');
        if(value>maximum/10||(value==maximum/10&&digit>maximum%10))return false;
        value=value*10+digit;
    }
    if(value<minimum)return false;
    result=value;return true;
}
}
bool parse_byte_comparison_request(const std::array<std::wstring,4>& text,size_t frames,ByteComparisonRequest& result){
    ByteComparisonRequest next;
    if(!frames||!decimal(text[0],1,frames,next.frame_a)||!decimal(text[1],1,frames,next.frame_b)||
       !decimal(text[2],0,4096,next.offset_a)||!decimal(text[3],0,4096,next.offset_b))return false;
    --next.frame_a;--next.frame_b;result=next;return true;
}
std::vector<ByteDifference> compare_bytes(std::string_view a,std::string_view b,size_t x,size_t y){
    if(a.size()>4096||b.size()>4096||x>a.size()||y>b.size())throw std::runtime_error("Compare two snapshots up to 4096 bytes each; offsets must be inside the snapshots.");
    std::vector<ByteDifference> result;for(size_t i=0;i<std::max(a.size()-x,b.size()-y);++i){const int p=i<a.size()-x?static_cast<unsigned char>(a[x+i]):-1,q=i<b.size()-y?static_cast<unsigned char>(b[y+i]):-1;if(p!=q)result.push_back({i,p,q});}return result;
}
