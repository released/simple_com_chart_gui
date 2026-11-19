#include "numeric_watch.h"
#include <sstream>
#include <cmath>
#include <locale>
#include <set>
bool parse_numeric_watches(const std::string& text,std::vector<NumericWatchRule>& out,std::string& error) {
    if(text.size()>16384){error="Watch definitions exceed 16 KiB.";return false;}
    std::istringstream input(text);std::string line;std::vector<NumericWatchRule> next;std::set<std::pair<unsigned,std::string>> keys;
    while(std::getline(input,line)) {
        if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;
        std::vector<std::string> parts;size_t at=0;for(;;){const auto end=line.find(';',at);parts.push_back(line.substr(at,end-at));if(end==line.npos)break;at=end+1;}
        if(parts.size()!=7||next.size()==16||(parts[0]!="A"&&parts[0]!="B")||parts[1].empty()||parts[1].size()>160||parts[1].find('\0')!=line.npos){error="Use up to 16 rows: A/B;exact key;low;high;hysteresis;dwell seconds;stale seconds.";return false;}
        NumericWatchRule r;r.source=parts[0]=="B";r.key=parts[1];double* fields[]={&r.low,&r.high,&r.hysteresis,&r.dwell,&r.stale};
        for(size_t i=0;i<5;++i){std::istringstream number(parts[i+2]);number.imbue(std::locale::classic());if(!(number>>*fields[i])||!(number>>std::ws).eof()||!std::isfinite(*fields[i])){error="Watch values must be complete finite numbers.";return false;}}
        if(r.low>=r.high||r.hysteresis<0||r.hysteresis>(r.high-r.low)/2||!std::isfinite(r.high-r.low)||r.dwell<0||r.dwell>60||r.stale<=0||r.stale>300||r.stale<r.dwell||!keys.emplace(r.source,r.key).second){error="Watch: low < high; hysteresis 0..half range; dwell 0..60 s; stale >0..300 s and >= dwell; keys unique.";return false;}
        next.push_back(std::move(r));
    }out=std::move(next);error.clear();return true;
}
void NumericWatches::arm(std::vector<NumericWatchRule> rules,uint64_t tick){states_.clear();for(auto& r:rules){State s;s.last=tick;s.rule=std::move(r);states_.push_back(std::move(s));}}
std::vector<NumericWatchEvent> NumericWatches::gap(unsigned source){std::vector<NumericWatchEvent> events;for(auto& s:states_)if(!s.disabled&&s.rule.source==source){s.disabled=true;events.push_back({source,s.rule.key,"Disarmed: input gap",0});}return events;}
std::vector<NumericWatchEvent> NumericWatches::disconnect(unsigned source){std::vector<NumericWatchEvent> events;for(auto& s:states_)if(!s.disabled&&s.rule.source==source){s.disabled=true;events.push_back({source,s.rule.key,"Disarmed: source disconnected",0});}return events;}
std::vector<NumericWatchEvent> NumericWatches::feed(unsigned source,const std::unordered_map<std::string,double>& values,uint64_t generation,uint64_t tick) {
    std::vector<NumericWatchEvent> events;
    for(auto& s:states_) {
        if(s.rule.source!=source||s.disabled)continue;
        if(s.generation!=UINT64_MAX&&s.generation!=generation){s.disabled=true;events.push_back({source,s.rule.key,"Disarmed: connection generation changed",0});continue;}
        s.generation=generation;const auto value=values.find(s.rule.key);if(value==values.end()||!std::isfinite(value->second))continue;
        if(s.seen&&(tick<s.last||tick-s.last>static_cast<uint64_t>(s.rule.stale*1000))){s.active=s.pending=0;s.since=tick;s.stale=true;}
        const double v=value->second;s.seen=true;s.last=tick;
        if(s.stale){events.push_back({source,s.rule.key,"Data resumed",v});s.stale=false;}
        const int target=s.active<0&&v<s.rule.low+s.rule.hysteresis?-1:s.active>0&&v>s.rule.high-s.rule.hysteresis?1:v<s.rule.low?-1:v>s.rule.high?1:0;
        if(target==s.active){s.pending=target;s.since=tick;continue;}
        if(target!=s.pending){s.pending=target;s.since=tick;}
        if(tick-s.since>=static_cast<uint64_t>(s.rule.dwell*1000)){s.active=target;events.push_back({source,s.rule.key,target<0?"Below minimum":target>0?"Above maximum":"Returned to range",v});}
    }return events;
}
std::vector<NumericWatchEvent> NumericWatches::poll(uint64_t tick) {
    std::vector<NumericWatchEvent> events;for(auto& s:states_)if(!s.disabled&&!s.stale&&tick>=s.last&&tick-s.last>static_cast<uint64_t>(s.rule.stale*1000)){s.stale=true;s.active=s.pending=0;events.push_back({s.rule.source,s.rule.key,"Data unavailable",0});}return events;
}
