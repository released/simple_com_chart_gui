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

}
bool FieldExtractor::configure(const ExtractionConfig& config,std::string& error) {
    error.clear();FieldExtractor next;next.config_=config;
    if(config.mode<0 || config.mode>ExtractionConfig::Regex || config.columns.size()>8192 || config.pattern.size()>1024 || config.time_field.size()>128) {error="Invalid extraction size/mode.";return false;}
    try {
        if(config.mode==ExtractionConfig::Csv || config.mode==ExtractionConfig::Tab) {
            next.columns_=split(config.columns,',');std::unordered_map<std::string,bool> seen;
            for(const auto& key:next.columns_) if(!key_ok(key) || !seen.emplace(key,true).second) throw std::runtime_error("Enter unique comma-separated column names (CSV and Tab).");
        }
        if(config.mode==ExtractionConfig::Json&&config.columns.rfind("ptr:",0)==0){next.json_paths_=json_selections(config.columns.substr(4));if(next.json_paths_.empty())throw std::runtime_error("Add name=/pointer after ptr:.");}
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
        if(config_.mode==ExtractionConfig::KeyValue) result.values=log_parser::parse_kv_log(line,&result.key_limits);
        else if(config_.mode==ExtractionConfig::Json) result.values=json_numbers(line,json_paths_);
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
    const std::string version=c.mode==ExtractionConfig::Json&&c.columns.rfind("ptr:",0)==0?"v2:":"v1:";
    return version+std::to_string(c.mode)+":"+std::to_string(c.name_group)+":"+std::to_string(c.value_group)+":"+std::to_string(c.time_group)+":"+uart_hex(c.columns)+":"+uart_hex(c.pattern)+":"+uart_hex(c.time_field);
}
ExtractionConfig decode_extraction(const std::string& s) {
    ExtractionConfig c;if(s.empty()) return c;std::vector<std::string> v;size_t p=0;
    do {const auto end=s.find(':',p);v.push_back(s.substr(p,end==s.npos?s.npos:end-p));if(end==s.npos) break;p=end+1;}while(true);
    if(v.size()!=8||(v[0]!="v1"&&v[0]!="v2")) throw std::runtime_error("Invalid extraction schema.");
    int n[4]{};for(int i=0;i<4;++i){const auto parsed=std::from_chars(v[i+1].data(),v[i+1].data()+v[i+1].size(),n[i]);if(parsed.ec!=std::errc{}||parsed.ptr!=v[i+1].data()+v[i+1].size()) throw std::runtime_error("Invalid extraction number.");}
    c.mode=static_cast<ExtractionConfig::Mode>(n[0]);c.name_group=n[1];c.value_group=n[2];c.time_group=n[3];
    if(!uart_unhex(v[5],c.columns)||!uart_unhex(v[6],c.pattern)||!uart_unhex(v[7],c.time_field)) throw std::runtime_error("Invalid extraction encoding.");
    uart_wide(c.columns);uart_wide(c.pattern);uart_wide(c.time_field);
    if(v[0]=="v2"&&(c.mode!=ExtractionConfig::Json||c.columns.rfind("ptr:",0)!=0))throw std::runtime_error("Extraction v2 requires explicit JSON Pointer selections.");
    FieldExtractor check;std::string error;if(!check.configure(c,error)) throw std::runtime_error(error);return c;
}
