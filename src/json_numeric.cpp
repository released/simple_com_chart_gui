#include "json_numeric.h"
#include "uart_files.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>
#include <algorithm>
#include <stdexcept>
namespace {
std::string pointer_key(const std::string& key){std::string s;for(char c:key){if(c=='~')s+="~0";else if(c=='/')s+="~1";else s+=c;}return s;}
class Reader {
public:
    Reader(const std::string& text,const JsonSelections& selected,std::vector<JsonNumericNode>* tree=nullptr):s(text),selected(selected),tree(tree){}
    std::unordered_map<std::string,double> run(){space();if(!tree&&selected.empty()&&(pos==s.size()||s[pos]!='{'))fail("Flat JSON requires an object.");value(0,"","");space();if(pos!=s.size())fail("Trailing JSON content.");return values_;}
private:
    [[noreturn]] void fail(const char* why){throw std::runtime_error(why);}
    void space(){while(pos<s.size()&&(s[pos]==' '||s[pos]=='\r'||s[pos]=='\n'||s[pos]=='\t'))++pos;}
    bool eat(char c){if(pos<s.size()&&s[pos]==c){++pos;return true;}return false;}
    void expect(char c){if(!eat(c))fail("Malformed JSON.");}
    unsigned hex4(){unsigned v=0;for(int i=0;i<4;++i){if(pos==s.size())fail("Incomplete JSON Unicode.");const char c=s[pos++];const int d=c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;if(d<0)fail("Invalid JSON Unicode.");v=v*16+d;}return v;}
    std::string string(){expect('"');std::string out;
        while(pos<s.size()){unsigned char c=static_cast<unsigned char>(s[pos++]);if(c=='"')return out;if(c<32)fail("JSON control character.");if(c!='\\'){out+=c;continue;}if(pos==s.size())break;c=static_cast<unsigned char>(s[pos++]);
            if(c=='u'){unsigned cp=hex4();if(cp>=0xd800&&cp<=0xdbff){expect('\\');expect('u');const auto lo=hex4();if(lo<0xdc00||lo>0xdfff)fail("Invalid JSON surrogate.");cp=0x10000+((cp-0xd800)<<10)+lo-0xdc00;}else if(cp>=0xdc00&&cp<=0xdfff)fail("Invalid JSON surrogate.");
                if(cp<0x80)out+=static_cast<char>(cp);else if(cp<0x800){out+=static_cast<char>(0xc0|(cp>>6));out+=static_cast<char>(0x80|(cp&63));}else if(cp<0x10000){out+=static_cast<char>(0xe0|(cp>>12));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}else{out+=static_cast<char>(0xf0|(cp>>18));out+=static_cast<char>(0x80|((cp>>12)&63));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}}
            else if(c=='"'||c=='\\'||c=='/')out+=c;else if(c=='b')out+='\b';else if(c=='f')out+='\f';else if(c=='n')out+='\n';else if(c=='r')out+='\r';else if(c=='t')out+='\t';else fail("Invalid JSON escape.");}
        fail("Unclosed JSON string.");
    }
    void value(unsigned depth,const std::string& path,const std::string& flat_key,size_t parent=SIZE_MAX){
        space();if(depth>16||pos==s.size()||++nodes>((selected.empty()&&!tree)?65536:4096))fail("JSON depth/work limit.");
        const auto node=tree?tree->size():SIZE_MAX;
        if(tree){if(path.size()>512||tree_bytes>1024*1024-path.size())fail("JSON tree paths exceed 512 bytes each or 1 MiB total.");tree_bytes+=path.size();}
        if(tree)tree->push_back({path,s[pos]=='{'?"Object":s[pos]=='['?"Array":s[pos]=='\"'?"String":s[pos]=='t'||s[pos]=='f'?"Boolean":s[pos]=='n'?"Null":"Number",parent});
        if(s[pos]=='"'){string();return;}
        if(eat('{')){space();if(eat('}'))return;std::set<std::string> keys;do{space();auto key=string();if((depth==0||!selected.empty()||tree)&&!keys.insert(key).second)fail("Duplicate JSON key.");if(keys.size()>1024)fail("Too many JSON keys.");space();expect(':');value(depth+1,path+"/"+pointer_key(key),depth==0?key:"",node);space();if(eat('}'))return;expect(',');}while(true);}
        if(eat('[')){space();if(eat(']'))return;size_t i=0;do{value(depth+1,path+"/"+std::to_string(i++),"",node);space();if(eat(']'))return;expect(',');}while(true);}
        for(const auto lit:{"true","false","null"}){const auto len=std::strlen(lit);if(s.compare(pos,len,lit)==0){pos+=len;return;}}
        const auto first=pos;eat('-');if(!eat('0')){const auto before=pos;while(pos<s.size()&&s[pos]>='0'&&s[pos]<='9')++pos;if(before==pos)fail("Invalid JSON number.");}
        if(eat('.')){const auto before=pos;while(pos<s.size()&&std::isdigit(static_cast<unsigned char>(s[pos])))++pos;if(before==pos)fail("Invalid JSON fraction.");}
        if(eat('e')||eat('E')){if(!eat('+'))eat('-');const auto before=pos;while(pos<s.size()&&std::isdigit(static_cast<unsigned char>(s[pos])))++pos;if(before==pos)fail("Invalid JSON exponent.");}
        const auto token=s.substr(first,pos-first);char* end=nullptr;const double n=std::strtod(token.c_str(),&end);if(!end||*end||!std::isfinite(n))fail("Invalid JSON finite number.");
        if(tree){(*tree)[node].numeric=true;(*tree)[node].value=n;}
        if(selected.empty()){if(depth==1&&!flat_key.empty()&&flat_key.size()<=128&&std::none_of(flat_key.begin(),flat_key.end(),[](unsigned char c){return c<32;}))values_[flat_key]=n;}
        else for(const auto& choice:selected)if(choice.second==path)values_[choice.first]=n;
    }
    const std::string& s;const JsonSelections& selected;size_t pos=0,nodes=0;std::unordered_map<std::string,double> values_;
    std::vector<JsonNumericNode>* tree=nullptr;
    size_t tree_bytes=0;
};
}
JsonSelections json_selections(const std::string& definition){
    if(definition.empty())return {};if(definition.size()>8192)throw std::runtime_error("JSON selections exceed 8 KiB.");uart_wide(definition);
    JsonSelections out;std::set<std::string> names;size_t at=0;
    while(at<=definition.size()){std::string token;
        if(at<definition.size()&&definition[at]=='"'){++at;bool closed=false;while(at<definition.size()){const char c=definition[at++];if(c=='"'){if(at<definition.size()&&definition[at]=='"'){token+='"';++at;}else{closed=true;break;}}else token+=c;}if(!closed||(at<definition.size()&&definition[at]!=','))throw std::runtime_error("Invalid quoted JSON selection.");}
        else {const auto comma=definition.find(',',at);token=definition.substr(at,comma==definition.npos?definition.npos:comma-at);at=comma==definition.npos?definition.size():comma;}
        const auto eq=token.find('=');
        if(eq==token.npos||!eq)throw std::runtime_error("JSON selections: name=/pointer,name2=/array/0; empty selects flat fields.");
        const auto name=token.substr(0,eq),path=token.substr(eq+1);
        if(name.size()>128||std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32;})||std::any_of(path.begin(),path.end(),[](unsigned char c){return c<32;})||!names.insert(name).second||path.size()>512||(!path.empty()&&path[0]!='/'))throw std::runtime_error("Invalid/duplicate JSON selection.");
        for(size_t i=0;i<path.size();++i)if(path[i]=='~'&&(++i==path.size()||(path[i]!='0'&&path[i]!='1')))throw std::runtime_error("JSON Pointer escapes are ~0 and ~1.");
        out.emplace_back(name,path);if(out.size()>64)throw std::runtime_error("At most 64 JSON selections.");if(at==definition.size())break;++at;
    }return out;
}
std::unordered_map<std::string,double> json_numbers(const std::string& text,const JsonSelections& selected){return Reader(text,selected).run();}
std::vector<JsonNumericNode> json_numeric_tree(const std::string& text){
    if(text.size()>65536)throw std::runtime_error("JSON sample exceeds 64 KiB.");uart_wide(text);
    std::vector<JsonNumericNode> tree;const JsonSelections selected;Reader(text,selected,&tree).run();return tree;
}
std::string json_tree_selection(const std::vector<JsonNumericNode>& tree,const std::vector<size_t>& selected){
    if(selected.empty()||selected.size()>64)throw std::runtime_error("Select 1..64 numeric leaves.");
    std::string definition;size_t n=0;std::set<size_t> unique;
    for(const auto index:selected){if(index>=tree.size()||!tree[index].numeric||!unique.insert(index).second)throw std::runtime_error("Select unique numeric leaves.");
        if(n)definition+=',';definition+='"';const auto token="value"+std::to_string(++n)+"="+tree[index].pointer;
        for(char c:token){definition+=c;if(c=='"')definition+='"';}definition+='"';
    }json_selections(definition);return "ptr:"+definition;
}
