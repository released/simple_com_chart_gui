#include "log_parser.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <charconv>
#include <cstdint>
#include <vector>
#include <string_view>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

bool parse_number(const std::string& text, double& value) {
    const auto clean=trim(text);if(clean.empty()) return false;
    size_t p=0;if(clean[p]=='+'||clean[p]=='-') ++p;
    if(p==clean.size()) return false;
    if(p+2<=clean.size() && clean[p]=='0' && (clean[p+1]=='x'||clean[p+1]=='X')) {
        const auto first=p+2;uint64_t number=0;const auto parsed=std::from_chars(clean.data()+first,clean.data()+clean.size(),number,16);
        if(parsed.ec!=std::errc{} || parsed.ptr==clean.data()+first || parsed.ptr!=clean.data()+clean.size()) return false;
        value=static_cast<double>(number);if(clean[0]=='-') value=-value;return true;
    }
    const auto digits=[&] {const auto start=p;while(p<clean.size() && clean[p]>='0' && clean[p]<='9') ++p;return p-start;};
    size_t count=digits();if(p<clean.size() && clean[p]=='.') {++p;count+=digits();}if(!count) return false;
    if(p+1<clean.size() && (clean[p]=='e'||clean[p]=='E') &&
       ((clean[p+1]>='0'&&clean[p+1]<='9') || clean[p+1]=='+' || clean[p+1]=='-')) {
        ++p;if(clean[p]=='+'||clean[p]=='-') ++p;if(!digits()) return false;
    }
    const auto number_end=p;while(p<clean.size() && (clean[p]==' '||clean[p]=='\t'||clean[p]=='\r'||clean[p]=='\n'||clean[p]=='\f'||clean[p]=='\v')) ++p;
    while(p<clean.size() && ((clean[p]>='a'&&clean[p]<='z')||(clean[p]>='A'&&clean[p]<='Z')||clean[p]=='%')) ++p;
    if(p!=clean.size()) return false;
    const auto token=clean.substr(0,number_end);char* after=nullptr;value=std::strtod(token.c_str(),&after);
    return after && after==token.c_str()+token.size() && std::isfinite(value);
}

bool digit(char ch) {return ch>='0'&&ch<='9';}
bool clock_field(const std::string& field) {
    if(field.size()!=12||field[2]!=':'||field[5]!=':'||field[8]!='.')return false;
    for(const auto index:{0u,1u,3u,4u,6u,7u,9u,10u,11u})if(!digit(field[index]))return false;
    return true;
}
std::string capture_payload(const std::string& line) {
    if(line.size()<20||line[0]!='['||line[3]!=':'||line[6]!=':'||line[9]!='.'||line[13]!=']'||
       line.compare(14,6," [RX] ")!=0)return line;
    for(const auto index:{1u,2u,4u,5u,7u,8u,10u,11u,12u})if(!digit(line[index]))return line;
    return line.substr(20);
}

std::vector<std::string> fields(std::string_view text,bool containers=false) {
    std::vector<std::string> result;size_t start=0;unsigned square=0,round=0;
    for(size_t i=0;i<=text.size();++i){
        if(i<text.size()&&containers){
            if(text[i]=='[')++square;else if(text[i]==']'&&square)--square;
            else if(text[i]=='(')++round;else if(text[i]==')'&&round)--round;
        }
        if(i==text.size()||(text[i]==','&&!square&&!round)){
            result.emplace_back(text.substr(start,i-start));start=i+1;
        }
    }
    return result;
}
struct Parser {
    std::unordered_map<std::string,double> result;
    size_t key_limits=0;
    bool key_ok(const std::string& key) {
        if(key.size()>128){++key_limits;return false;}
        return !key.empty()&&std::none_of(key.begin(),key.end(),[](unsigned char ch){return ch<32;});
    }
    void read(const std::vector<std::string>& tokens,size_t& index,const std::string& group,bool last=false) {
        const auto& token=tokens[index];
        if(last&&clock_field(trim(token)))return;
        const auto separator=last?token.find_last_of(":="):token.find_first_of(":=");
        if(separator==token.npos)return;
        auto key=trim(token.substr(0,separator));
        if(!group.empty()&&!(key.size()>=2&&key.front()=='['&&key.back()==']'))key=group+"."+key;
        if(!key_ok(key))return;
        auto raw=trim(token.substr(separator+1));double number=0;
        if(raw.empty()&&index+1<tokens.size()&&parse_number(tokens[index+1],number))raw=tokens[++index];
        std::vector<double> values;size_t at=0;
        do{
            const auto end=raw.find('/',at);const auto count=end==raw.npos?raw.size()-at:end-at;
            if(!parse_number(raw.substr(at,count),number)||values.size()==1024)return;
            values.push_back(number);if(end==raw.npos)break;at=end+1;
        }while(at<=raw.size());
        for(size_t i=0;i<values.size();++i)
            if(!key_ok(values.size()==1?key:key+"["+std::to_string(i)+"]"))return;
        for(size_t i=0;i<values.size();++i){
            const auto name=values.size()==1?key:key+"["+std::to_string(i)+"]";
            result[name]=values[i];
        }
    }
    void bracket_fields(const std::string& token) {
        size_t start=0;
        while((start=token.find('[',start))!=token.npos){
            const auto end=token.find(']',start+1);if(end==token.npos)break;
            start=token.rfind('[',end);
            const auto body=token.substr(start+1,end-start-1);
            if(!clock_field(body)){
                const auto tokens=fields(body);
                for(size_t i=0;i<tokens.size();++i)read(tokens,i,"",true);
            }
            start=end+1;
        }
    }
    void parse(const std::string& payload) {
        double scalar=0;if(parse_number(payload,scalar)){result["value"]=scalar;return;}
        const auto tokens=fields(payload,true);std::string group;
        for(size_t i=0;i<tokens.size();++i){
            const auto token=trim(tokens[i]);
            if(token.size()>=3&&token.front()=='['&&token.back()==']'&&token.find_first_of(":=,[]",1)==token.size()-1){
                // An invalid group remains invalid; do not silently unqualify its fields.
                group=key_ok(token)?token:std::string(129,'?');continue;
            }
            if(token.size()>=2&&token.front()=='('&&token.back()==')'){
                const auto inner=fields(std::string_view(token).substr(1,token.size()-2));
                for(size_t j=0;j<inner.size();++j)read(inner,j,group);
            }else{bracket_fields(token);read(tokens,i,group);}
        }
    }
};
}

namespace log_parser {
bool parse_numeric_value(const std::string& text,double& value) {return parse_number(text,value);}
std::unordered_map<std::string,double> parse_kv_log(const std::string& line,size_t* key_limits) {
    Parser parser;if(line.size()<=65536)parser.parse(capture_payload(line));
    if(key_limits)*key_limits=parser.key_limits;
    return std::move(parser.result);
}
}
