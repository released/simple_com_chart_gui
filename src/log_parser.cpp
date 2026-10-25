#include "log_parser.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <charconv>
#include <cstdint>
#include <vector>

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
}

namespace log_parser {
bool parse_numeric_value(const std::string& text,double& value) {return parse_number(text,value);}
std::unordered_map<std::string, double> parse_kv_log(const std::string& line) {
    std::unordered_map<std::string, double> result;
    size_t start = 0;
    while (start < line.size()) {
        auto end = line.find(',', start);
        if (end == std::string::npos) end = line.size();
        const auto token = line.substr(start, end - start);
        start = end + 1;
        const auto separator = token.find_first_of(":=");
        if (separator == std::string::npos) continue;
        const auto key = trim(token.substr(0, separator));
        if (key.empty() || key.size() > 128 || std::any_of(key.begin(), key.end(),
            [](unsigned char ch) { return ch < 32; })) continue;
        const auto raw = token.substr(separator + 1);
        std::vector<double> values;
        size_t part_start = 0;
        bool valid = true;
        do {
            auto part_end = raw.find('/', part_start);
            if (part_end == std::string::npos) part_end = raw.size();
            double value = 0;
            if (!parse_number(raw.substr(part_start, part_end - part_start), value)) {
                valid = false;
                break;
            }
            values.push_back(value);
            if(values.size()>1024) {valid=false;break;}
            part_start = part_end + 1;
        } while (part_start <= raw.size());
        if (!valid) continue;
        for (size_t i = 0; i < values.size(); ++i) {
            result[values.size() == 1 ? key : key + "[" + std::to_string(i) + "]"] = values[i];
        }
    }
    return result;
}
}
