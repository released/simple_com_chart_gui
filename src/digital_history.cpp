#include "lab_metrics.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
DigitalHistory digital_history(const CaptureSnapshot& data,const DigitalLane& lane,double window,double hold) {
    if(!std::isfinite(window)||window<=0||!std::isfinite(hold)||hold<=0)throw std::runtime_error("Invalid digital history range.");
    DigitalHistory result;const auto key=std::find(data.keys.begin(),data.keys.end(),lane.key);if(key==data.keys.end()||data.rows.empty())return result;
    const auto column=static_cast<size_t>(key-data.keys.begin());std::optional<unsigned> source;
    for(auto it=data.rows.rbegin();it!=data.rows.rend();++it){double v=0;if(CaptureStore::value(**it,column,v)){source=(*it)->numeric_source;break;}}
    if(!source)return result;
    double end=0;for(const auto& row:data.rows)end=std::max(end,row->seconds);const double start=std::max(0.,end-window);bool have=false;double previous_time=0;uint32_t previous=0;uint64_t generation=0;
    for(const auto& row:data.rows) {
        if(row->numeric_source!=*source||row->seconds<start-hold||row->seconds>end)continue;
        double raw=0;const auto value=CaptureStore::value(*row,column,raw)?digital_value(raw,lane):std::optional<uint32_t>{};
        if(have&&row->generation==generation&&row->seconds>=previous_time&&row->seconds-previous_time<=hold&&row->seconds>=start)result.segments.push_back({std::max(start,previous_time),row->seconds,previous,value});
        have=value.has_value();if(value){previous=*value;previous_time=row->seconds;generation=row->generation;if(row->seconds>=start)result.latest=std::make_pair(row->seconds,*value);}
    }
    return result;
}
