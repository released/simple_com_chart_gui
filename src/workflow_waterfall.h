#pragma once
#include "signal_analysis.h"
#include <deque>
#include <cstdint>
struct WaterfallFrame {Spectrum spectrum;double seconds=0;uint64_t generation=0;};
class WaterfallHistory {
public:
    bool push(const WaterfallFrame&,size_t depth);
    void clear(){rows_.clear();rate_=0;last_=0;generation_=UINT64_MAX;}
    const std::deque<std::vector<float>>& rows()const{return rows_;}
    double rate()const{return rate_;}
    double newest()const{return last_;}
    uint64_t generation()const{return generation_;}
private:std::deque<std::vector<float>> rows_;double rate_=0,last_=0;uint64_t generation_=UINT64_MAX;
};
