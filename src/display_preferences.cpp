#include "display_preferences.h"
#include "uart_files.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

bool DisplayPreferences::set(const std::string& key,const ChannelDisplay& d,std::string& error) {
    const auto safe=[](const std::string& text,size_t limit){return text.size()<=limit && std::none_of(text.begin(),text.end(),[](unsigned char c){return c<32;});};
    error.clear();
    if(key.empty() || !safe(key,160) || !safe(d.alias,128) || !safe(d.unit,32) || d.decimals<-1 || d.decimals>12 || !std::isfinite(d.scale) || !std::isfinite(d.offset) || std::abs(d.scale)>1e12 || std::abs(d.offset)>1e100 || (!entries_.count(key) && entries_.size()>=256)) {error="Invalid display settings or 256-entry limit.";return false;}
    try {uart_wide(key);uart_wide(d.alias);uart_wide(d.unit);} catch(...) {error="Invalid display UTF-8.";return false;}
    entries_[key]=d;return true;
}
ChannelDisplay DisplayPreferences::get(const std::string& key) const {const auto i=entries_.find(key);return i==entries_.end()?ChannelDisplay{}:i->second;}
double DisplayPreferences::project(const std::string& key,double raw) const {const auto d=get(key);const double result=raw*d.scale+d.offset;return std::isfinite(result)?result:std::numeric_limits<double>::quiet_NaN();}
std::string DisplayPreferences::name(const std::string& key) const {const auto d=get(key);return d.alias.empty()?key:d.alias;}
std::string DisplayPreferences::format(const std::string& key,double displayed) const {
    if(!std::isfinite(displayed)) return "n/a";const auto d=get(key);char value[512]{};
    if(d.decimals<0) snprintf(value,sizeof(value),"%.10g",displayed);
    else if(std::abs(displayed)>=1e18) snprintf(value,sizeof(value),"%.*e",d.decimals,displayed);
    else snprintf(value,sizeof(value),"%.*f",d.decimals,displayed);
    return value+(d.unit.empty()?std::string():" "+d.unit);
}
std::string DisplayPreferences::encode() const {
    std::vector<std::string> keys;for(const auto& entry:entries_) keys.push_back(entry.first);std::sort(keys.begin(),keys.end());
    std::ostringstream out;out.imbue(std::locale::classic());out<<"v1\n"<<std::setprecision(17);
    for(const auto& key:keys) {const auto& d=entries_.at(key);out<<uart_hex(key)<<':'<<uart_hex(d.alias)<<':'<<uart_hex(d.unit)<<':'<<d.decimals<<':'<<d.scale<<':'<<d.offset<<'\n';}
    if(out.str().size()>30000) throw std::runtime_error("Display preferences exceed 30 KiB.");return out.str();
}
DisplayPreferences DisplayPreferences::decode(const std::string& encoded) {
    DisplayPreferences result;if(encoded.empty()) return result;if(encoded.size()>30000) throw std::runtime_error("Oversized display preferences.");
    std::istringstream input(encoded);std::string line;if(!std::getline(input,line)||line!="v1") throw std::runtime_error("Invalid display schema.");
    while(std::getline(input,line)) {std::replace(line.begin(),line.end(),':',' ');std::istringstream row(line);row.imbue(std::locale::classic());
        // Empty hex fields are legal; split explicitly instead of formatted extraction.
        std::vector<std::string> parts;size_t p=0;do{const auto end=line.find(' ',p);parts.push_back(line.substr(p,end==line.npos?line.npos:end-p));if(end==line.npos) break;p=end+1;}while(true);
        if(parts.size()!=6) throw std::runtime_error("Malformed display record.");std::string key;ChannelDisplay d;
        if(!uart_unhex(parts[0],key)||!uart_unhex(parts[1],d.alias)||!uart_unhex(parts[2],d.unit)) throw std::runtime_error("Invalid display text.");
        std::istringstream numbers(parts[3]+" "+parts[4]+" "+parts[5]);numbers.imbue(std::locale::classic());char extra=0;
        if(!(numbers>>d.decimals>>d.scale>>d.offset)||(numbers>>extra)||result.entries_.count(key)) throw std::runtime_error("Invalid/duplicate display number.");
        std::string error;if(!result.set(key,d,error)) throw std::runtime_error(error);
    }return result;
}
