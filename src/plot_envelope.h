#pragma once
#include "channel_model.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

struct PlotPixelSpan {int x=0;ChannelSample first,last,low,high;};
inline std::vector<PlotPixelSpan> plot_pixel_spans(const std::vector<ChannelSample>& data,double start,double end,int width){
    std::vector<PlotPixelSpan> out;if(width<=0||!(start<end))return out;out.reserve(std::min(data.size(),static_cast<size_t>(width+1)));
    for(const auto& p:data){if(!std::isfinite(p.t)||!std::isfinite(p.v)||p.t<start||p.t>end)continue;
        const auto x=static_cast<int>(std::llround((p.t-start)/(end-start)*width));
        if(out.empty()||out.back().x!=x)out.push_back({x,p,p,p,p});
        else {auto& span=out.back();span.last=p;if(p.v<span.low.v)span.low=p;if(p.v>span.high.v)span.high=p;}
    }return out;
}

// Rendering only: preserve extrema in their original temporal order.
inline std::vector<ChannelSample> plot_envelope(const std::vector<ChannelSample>& data,
                                               double start,double end,int width) {
    std::vector<ChannelSample> out;
    if(width<=0||!(start<end))return out;
    out.reserve(std::min(data.size(),static_cast<size_t>(width+1)*4));
    size_t first=0,last=0,low=0,high=0;int bucket=-1;
    const auto flush=[&]{
        std::array<size_t,4> indices{{first,low,high,last}};
        std::sort(indices.begin(),indices.end());
        size_t previous=SIZE_MAX;
        for(const auto i:indices)if(i!=previous){out.push_back(data[i]);previous=i;}
    };
    for(size_t i=0;i<data.size();++i){const auto& p=data[i];
        if(!std::isfinite(p.t)||!std::isfinite(p.v)||p.t<start||p.t>end)continue;
        const auto x=static_cast<int>(std::llround((static_cast<long double>(p.t)-start)/(static_cast<long double>(end)-start)*width));
        if(x!=bucket){if(bucket>=0)flush();bucket=x;first=last=low=high=i;}
        else {last=i;if(p.v<data[low].v)low=i;if(p.v>data[high].v)high=i;}
    }
    if(bucket>=0)flush();
    return out;
}
