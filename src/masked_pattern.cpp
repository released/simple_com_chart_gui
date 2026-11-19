#include "masked_pattern.h"
#include <cwctype>
bool parse_masked_pattern(std::wstring_view text,MaskedPattern& out,std::wstring& error) {
    MaskedPattern next;size_t p=0;
    const auto hex=[](wchar_t c){return c>=L'0'&&c<=L'9'?c-L'0':c>=L'a'&&c<=L'f'?c-L'a'+10:c>=L'A'&&c<=L'F'?c-L'A'+10:-1;};
    while(p<text.size()) {
        while(p<text.size()&&iswspace(text[p]))++p;if(p==text.size())break;
        const auto first=p;while(p<text.size()&&!iswspace(text[p]))++p;
        if(p-first!=2||next.value.size()==256){error=L"Masked HEX: enter 1..256 space-separated byte pairs or ??.";return false;}
        if(text.substr(first,2)==L"??"){next.value.push_back(0);next.mask.push_back(0);}
        else {const int a=hex(text[first]),b=hex(text[first+1]);if(a<0||b<0){error=L"Masked HEX accepts complete byte pairs or ??, not nibble wildcards or 0x.";return false;}next.value.push_back(static_cast<uint8_t>(a*16+b));next.mask.push_back(255);}
    }
    if(next.value.empty()){error=L"Masked HEX pattern is empty.";return false;}out=std::move(next);error.clear();return true;
}
size_t find_masked(std::string_view data,const MaskedPattern& p) {
    if(p.value.empty()||p.value.size()!=p.mask.size()||p.value.size()>data.size())return std::string::npos;
    size_t anchor=0;while(anchor<p.mask.size()&&!p.mask[anchor])++anchor;
    for(size_t i=0;i<=data.size()-p.value.size();++i){
        if(anchor<p.mask.size()&&static_cast<uint8_t>(data[i+anchor])!=p.value[anchor])continue;
        size_t j=0;while(j<p.value.size()&&(static_cast<uint8_t>(data[i+j])&p.mask[j])==p.value[j])++j;
        if(j==p.value.size())return i;
    }return std::string::npos;
}
