#include "linked_cursor.h"
#include <cmath>
#include <algorithm>
std::optional<double> linked_cursor_value(const CaptureSnapshot& data,const std::string& key,double time,double age,const CaptureRow* context){
    if(!std::isfinite(time)||!std::isfinite(age)||age<=0||data.rows.empty())return {};
    if(context&&std::none_of(data.rows.begin(),data.rows.end(),[&](const auto& row){return row.get()==context;}))return {};
    const auto column=std::find(data.keys.begin(),data.keys.end(),key);if(column==data.keys.end())return {};
    const CaptureRow* anchor=context;double distance=age;
    if(context&&std::abs(context->source_seconds-time)>age)return {};
    if(!context)for(const auto& row:data.rows){const auto delta=std::abs(row->source_seconds-time);if(delta<=distance){distance=delta;anchor=row.get();}}
    if(!anchor)return {};
    std::optional<double> result;distance=age;
    for(const auto& row:data.rows){if(row->numeric_source!=anchor->numeric_source||row->generation!=anchor->generation)continue;double value=0;
        const auto delta=std::abs(row->source_seconds-time);if(delta<=distance&&CaptureStore::value(*row,static_cast<size_t>(column-data.keys.begin()),value)&&std::isfinite(value)){distance=delta;result=value;}}
    return result;
}
