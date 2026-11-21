#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
struct NumericWatchRule {unsigned source=0;std::string key;double low=0,high=0,hysteresis=0,dwell=0,stale=2;unsigned severity=1;};
bool parse_numeric_watches(const std::string&,std::vector<NumericWatchRule>&,std::string&);
struct NumericWatchEvent {unsigned source=0;std::string key,state;double value=0;unsigned severity=1;};
class NumericWatches {
public:
    void arm(std::vector<NumericWatchRule>,uint64_t tick=0);
    void stop(){states_.clear();}
    bool armed()const{for(const auto& state:states_)if(!state.disabled)return true;return false;}
    std::vector<NumericWatchEvent> feed(unsigned,const std::unordered_map<std::string,double>&,uint64_t generation,uint64_t tick);
    std::vector<NumericWatchEvent> poll(uint64_t tick);
    std::vector<NumericWatchEvent> disconnect(unsigned);
    std::vector<NumericWatchEvent> gap(unsigned);
private:
    struct State {NumericWatchRule rule;uint64_t generation=UINT64_MAX,last=0,since=0;int active=0,pending=0;bool seen=false,stale=false,disabled=false;};
    std::vector<State> states_;
};
