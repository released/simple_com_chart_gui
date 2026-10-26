#include "workflow_waterfall.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
bool WaterfallHistory::push(const WaterfallFrame& f,size_t depth){if(depth<1||depth>256||f.spectrum.bins.size()<2||f.spectrum.bins.size()>2049||!std::isfinite(f.seconds)||!std::isfinite(f.spectrum.sample_rate)||f.spectrum.sample_rate<=0)throw std::runtime_error("Invalid bounded waterfall frame.");std::vector<float> row;row.reserve(f.spectrum.bins.size());double previous=-1;
    for(const auto& bin:f.spectrum.bins){if(!std::isfinite(bin.first)||!std::isfinite(bin.second)||bin.first<0||bin.first<=previous||bin.second<0)throw std::runtime_error("Invalid FFT bin.");previous=bin.first;row.push_back(static_cast<float>(std::clamp(20*std::log10(std::max(bin.second,1e-12)),-200.0,200.0)));}
    if(!rows_.empty()&&(generation_!=f.generation||rows_.back().size()!=row.size()||std::abs(rate_-f.spectrum.sample_rate)>rate_*0.000001||f.seconds<last_||f.seconds-last_>std::max(2.0,4*(row.size()-1)/f.spectrum.sample_rate)))clear();
    if(!rows_.empty()&&f.seconds==last_)return false;generation_=f.generation;rate_=f.spectrum.sample_rate;last_=f.seconds;rows_.push_back(std::move(row));while(rows_.size()>depth)rows_.pop_front();return true;
}
