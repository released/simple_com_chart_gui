#include "field_extractor.h"
#include "log_parser.h"
#include "uart_files.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cctype>

namespace {
std::string trim(std::string s) {const auto a=s.find_first_not_of(" \t\r\n");return a==s.npos?"":s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
bool key_ok(const std::string& key) {return !key.empty() && key.size()<=128 && std::none_of(key.begin(),key.end(),[](unsigned char c){return c<32;});}
std::vector<std::string> split(const std::string& line,char delimiter) {
    std::vector<std::string> result;size_t p=0;
    while(p<=line.size()) {
        std::string field;
        if(p<line.size() && line[p]=='"') {
            ++p;bool closed=false;
            while(p<line.size()) {const auto c=line[p++];if(c=='"') {if(p<line.size() && line[p]=='"') {++p;field+='"';} else {closed=true;break;}}else field+=c;}
            if(!closed) throw std::runtime_error("Unclosed quoted field.");
            while(p<line.size() && (line[p]==' '||line[p]=='\r')) ++p;
            if(p<line.size() && line[p]!=delimiter) throw std::runtime_error("Unexpected data after quoted field.");
        } else {const auto end=line.find(delimiter,p);field=trim(line.substr(p,end==line.npos?line.npos:end-p));p=end==line.npos?line.size():end;}
        result.push_back(std::move(field)); if(result.size()>1024) throw std::runtime_error("More than 1024 fields.");
        if(p==line.size()) break;++p;
    }
    return result;
}
// Flat JSON numeric fields; nested values are validated and skipped, not flattened.
class JsonReader {
public:
    explicit JsonReader(const std::string& s):s_(s){}
    std::unordered_map<std::string,double> read() {
        space();expect('{');std::unordered_map<std::string,double> values;
        space();if(eat('}')) return finish(values);
        size_t fields=0;std::unordered_map<std::string,bool> seen;
        do {space();const auto key=string();if(!seen.emplace(key,true).second) throw std::runtime_error("Duplicate JSON key.");space();expect(':');space();double number=0;const bool numeric=value(0,number);
            if(numeric && key_ok(key)) {if(!values.emplace(key,number).second) throw std::runtime_error("Duplicate numeric JSON key.");}
            if(++fields>1024) throw std::runtime_error("Too many JSON fields.");space();if(eat('}')) return finish(values);expect(',');
        } while(true);
    }
private:
    std::unordered_map<std::string,double> finish(std::unordered_map<std::string,double>& v) {space();if(p_!=s_.size()) throw std::runtime_error("Trailing JSON content.");return v;}
    void space(){while(p_<s_.size() && (s_[p_]==' '||s_[p_]=='\t'||s_[p_]=='\r'||s_[p_]=='\n')) ++p_;}
    bool eat(char c){if(p_<s_.size()&&s_[p_]==c){++p_;return true;}return false;}
    void expect(char c){if(!eat(c)) throw std::runtime_error("Malformed JSON.");}
    unsigned hex4(){unsigned n=0;for(int i=0;i<4;++i){if(p_==s_.size()) throw std::runtime_error("Incomplete JSON Unicode.");const char c=s_[p_++];const int d=c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;if(d<0) throw std::runtime_error("Invalid JSON Unicode.");n=n*16+d;}return n;}
    std::string string(){expect('"');std::string out;
        while(p_<s_.size()){unsigned char c=static_cast<unsigned char>(s_[p_++]);if(c=='"') return out;if(c<32) throw std::runtime_error("JSON control character.");
            if(c!='\\'){out+=static_cast<char>(c);continue;}if(p_==s_.size()) break;c=static_cast<unsigned char>(s_[p_++]);
            if(c=='u'){unsigned cp=hex4();if(cp>=0xd800 && cp<=0xdbff){expect('\\');expect('u');unsigned lo=hex4();if(lo<0xdc00||lo>0xdfff) throw std::runtime_error("Invalid surrogate.");cp=0x10000+((cp-0xd800)<<10)+lo-0xdc00;}else if(cp>=0xdc00&&cp<=0xdfff) throw std::runtime_error("Invalid surrogate.");
                if(cp<0x80) out+=static_cast<char>(cp);else if(cp<0x800){out+=static_cast<char>(0xc0|(cp>>6));out+=static_cast<char>(0x80|(cp&63));}else if(cp<0x10000){out+=static_cast<char>(0xe0|(cp>>12));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}else{out+=static_cast<char>(0xf0|(cp>>18));out+=static_cast<char>(0x80|((cp>>12)&63));out+=static_cast<char>(0x80|((cp>>6)&63));out+=static_cast<char>(0x80|(cp&63));}}
            else if(c=='"'||c=='\\'||c=='/') out+=static_cast<char>(c);else if(c=='b') out+='\b';else if(c=='f') out+='\f';else if(c=='n') out+='\n';else if(c=='r') out+='\r';else if(c=='t') out+='\t';else throw std::runtime_error("Invalid JSON escape.");
        }throw std::runtime_error("Unclosed JSON string.");}
    bool value(int depth,double& result){space();if(depth>16 || p_==s_.size()) throw std::runtime_error("Invalid/deep JSON.");
        if(s_[p_]=='"'){string();return false;}
        if(eat('{')){space();if(eat('}')) return false;do{space();string();space();expect(':');double ignored=0;value(depth+1,ignored);space();if(eat('}')) return false;expect(',');}while(true);}
        if(eat('[')){space();if(eat(']')) return false;do{double ignored=0;value(depth+1,ignored);space();if(eat(']')) return false;expect(',');}while(true);}
        for(const auto literal:{"true","false","null"}) {const size_t n=strlen(literal);if(s_.compare(p_,n,literal)==0){p_+=n;return false;}}
        const size_t start=p_;eat('-');if(!eat('0')) {const auto first=p_;while(p_<s_.size() && s_[p_]>='0'&&s_[p_]<='9') ++p_;if(first==p_) throw std::runtime_error("Invalid JSON number.");}
        if(eat('.')) {const auto first=p_;while(p_<s_.size()&&isdigit(static_cast<unsigned char>(s_[p_]))) ++p_;if(first==p_) throw std::runtime_error("Invalid JSON fraction.");}
        if(eat('e')||eat('E')){if(!eat('+')) eat('-');const auto first=p_;while(p_<s_.size()&&isdigit(static_cast<unsigned char>(s_[p_]))) ++p_;if(first==p_) throw std::runtime_error("Invalid JSON exponent.");}
        const auto token=s_.substr(start,p_-start);char* end=nullptr;result=strtod(token.c_str(),&end);if(!std::isfinite(result)||!end||*end) throw std::runtime_error("Invalid JSON finite number.");return true;}
    const std::string& s_;size_t p_=0;
};
}
bool FieldExtractor::configure(const ExtractionConfig& config,std::string& error) {
    error.clear();FieldExtractor next;next.config_=config;
    if(config.mode<0 || config.mode>ExtractionConfig::Regex || config.columns.size()>8192 || config.pattern.size()>1024 || config.time_field.size()>128) {error="Invalid extraction size/mode.";return false;}
    try {
        if(config.mode==ExtractionConfig::Csv || config.mode==ExtractionConfig::Tab) {
            next.columns_=split(config.columns,',');std::unordered_map<std::string,bool> seen;
            for(const auto& key:next.columns_) if(!key_ok(key) || !seen.emplace(key,true).second) throw std::runtime_error("Enter unique comma-separated column names (CSV and Tab).");
        }
        if(config.mode==ExtractionConfig::Regex) {
            if(!next.regex_.compile(config.pattern,error)) return false;
            const auto group_ok=[&](int n){return n>0 && static_cast<size_t>(n)<=next.regex_.groups();};
            if(!group_ok(config.name_group)||!group_ok(config.value_group)||(config.time_group && !group_ok(config.time_group))) throw std::runtime_error("Choose existing capture groups 1..8; time 0 means receipt time.");
        }
        *this=std::move(next);return true;
    } catch(const std::exception& e){error=e.what();return false;}
}
ExtractedFields FieldExtractor::extract(const std::string& line) const {
    ExtractedFields result;if(line.size()>65536){result.limited=true;result.error="Line exceeds 64 KiB.";return result;}
    try {
        if(config_.mode==ExtractionConfig::KeyValue) result.values=log_parser::parse_kv_log(line);
        else if(config_.mode==ExtractionConfig::Json) result.values=JsonReader(line).read();
        else if(config_.mode==ExtractionConfig::Csv || config_.mode==ExtractionConfig::Tab) {
            const auto values=split(line,config_.mode==ExtractionConfig::Csv?',':'\t');
            if(values.size()!=columns_.size()) throw std::runtime_error("Column count does not match configured names.");
            for(size_t i=0;i<values.size();++i){double number=0;if(log_parser::parse_numeric_value(values[i],number)) result.values[columns_[i]]=number;}
        } else {
            const auto match=regex_.search(line,65536);result.limited=match.limited;if(!match.found) return result;
            const auto group=[&](int n){const auto a=match.bounds[n*2],b=match.bounds[n*2+1];return a<0||b<a?std::string():line.substr(static_cast<size_t>(a),static_cast<size_t>(b-a));};
            const auto key=group(config_.name_group);double number=0;
            if(key_ok(key) && log_parser::parse_numeric_value(group(config_.value_group),number)) result.values[key]=number;
            if(config_.time_group) {double time=0;if(log_parser::parse_numeric_value(group(config_.time_group),time) && time>=0 && time<=1e12) result.seconds=time;else throw std::runtime_error("Captured time must be 0..1e12 seconds.");}
        }
        if(!config_.time_field.empty()) {const auto i=result.values.find(config_.time_field);if(i==result.values.end()||i->second<0||i->second>1e12) throw std::runtime_error("Time field must exist and be 0..1e12 seconds.");result.seconds=i->second;}
    } catch(const std::exception& e){result.values.clear();result.seconds.reset();result.error=e.what();}
    return result;
}
std::string encode_extraction(const ExtractionConfig& c) {
    FieldExtractor check;std::string error;if(!check.configure(c,error)) throw std::runtime_error(error);
    return "v1:"+std::to_string(c.mode)+":"+std::to_string(c.name_group)+":"+std::to_string(c.value_group)+":"+std::to_string(c.time_group)+":"+uart_hex(c.columns)+":"+uart_hex(c.pattern)+":"+uart_hex(c.time_field);
}
ExtractionConfig decode_extraction(const std::string& s) {
    ExtractionConfig c;if(s.empty()) return c;std::vector<std::string> v;size_t p=0;
    do {const auto end=s.find(':',p);v.push_back(s.substr(p,end==s.npos?s.npos:end-p));if(end==s.npos) break;p=end+1;}while(true);
    if(v.size()!=8||v[0]!="v1") throw std::runtime_error("Invalid extraction schema.");
    int n[4]{};for(int i=0;i<4;++i){const auto parsed=std::from_chars(v[i+1].data(),v[i+1].data()+v[i+1].size(),n[i]);if(parsed.ec!=std::errc{}||parsed.ptr!=v[i+1].data()+v[i+1].size()) throw std::runtime_error("Invalid extraction number.");}
    c.mode=static_cast<ExtractionConfig::Mode>(n[0]);c.name_group=n[1];c.value_group=n[2];c.time_group=n[3];
    if(!uart_unhex(v[5],c.columns)||!uart_unhex(v[6],c.pattern)||!uart_unhex(v[7],c.time_field)) throw std::runtime_error("Invalid extraction encoding.");
    uart_wide(c.columns);uart_wide(c.pattern);uart_wide(c.time_field);
    FieldExtractor check;std::string error;if(!check.configure(c,error)) throw std::runtime_error(error);return c;
}
