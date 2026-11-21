#include "monitor_analysis.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
namespace {
void check_cancel(const std::atomic<bool>& cancel){if(cancel)throw std::runtime_error("Cancelled.");}
}
Distribution monitor_distribution(std::vector<double> values,const std::atomic<bool>& cancel){
    if(values.size()>10000)throw std::runtime_error("Distribution exceeds 10000 samples.");
    check_cancel(cancel);
    for(double value:values)if(!std::isfinite(value))throw std::runtime_error("Distribution requires finite values.");
    Distribution result;result.count=values.size();if(values.empty())return result;
    std::sort(values.begin(),values.end());check_cancel(cancel);
    result.low=values.front();result.high=values.back();
    const auto rank=[&](double p){return values[static_cast<size_t>(std::ceil(p*values.size()))-1];};
    result.p50=rank(.5);result.p95=rank(.95);result.p99=rank(.99);
    const auto span=result.high-result.low;
    for(double value:values){double fraction=0;if(result.high>result.low)fraction=std::isfinite(span)?(value-result.low)/span:(value/2-result.low/2)/(result.high/2-result.low/2);const auto index=static_cast<size_t>(std::clamp(fraction*32,0.0,31.0));++result.bins[index];}
    return result;
}
std::vector<TimelineRow> monitor_timeline(const TrafficBatch& a,const TrafficBatch& b,const std::atomic<bool>& cancel){
    if(a.records.size()>2048||b.records.size()>2048)throw std::runtime_error("Timeline exceeds retained record limits.");
    std::vector<TimelineRow> rows;size_t bytes=0;
    for(unsigned source=0;source<2;++source)for(const auto& row:(source?b:a).records){
        check_cancel(cancel);if(row.bytes.size()>512*1024-bytes)throw std::runtime_error("Timeline exceeds 512 KiB.");bytes+=row.bytes.size();rows.push_back({source,row});
    }
    std::stable_sort(rows.begin(),rows.end(),[](const auto& x,const auto& y){if(x.record.receipt_ms!=y.record.receipt_ms)return x.record.receipt_ms<y.record.receipt_ms;if(x.source!=y.source)return x.source<y.source;return x.record.sequence<y.record.sequence;});
    return rows;
}
std::vector<double> monitor_values(const CaptureSnapshot& snapshot,const std::string& key,unsigned source,double first,double last,const std::atomic<bool>& cancel){
    if(source>1||!std::isfinite(first)||!std::isfinite(last)||first>last||snapshot.rows.size()>10000)throw std::runtime_error("Invalid distribution range/source.");
    const auto found=std::find(snapshot.keys.begin(),snapshot.keys.end(),key);if(found==snapshot.keys.end())throw std::runtime_error("Selected channel is no longer retained.");
    const auto column=static_cast<size_t>(found-snapshot.keys.begin());std::vector<double> values;
    for(const auto& row:snapshot.rows){check_cancel(cancel);double value=0;if(row&&row->numeric_source==source&&row->seconds>=first&&row->seconds<=last&&CaptureStore::value(*row,column,value)&&std::isfinite(value))values.push_back(value);}
    return values;
}
std::vector<double> monitor_intervals(const TrafficBatch& batch,const std::atomic<bool>& cancel){
    if(batch.records.size()>2048)throw std::runtime_error("Receipt analysis exceeds 2048 records.");
    std::vector<double> values;const TrafficRecord* previous=nullptr;
    for(const auto& row:batch.records){check_cancel(cancel);if(row.tx||row.receipt_ms<=0)continue;
        if(previous&&row.generation==previous->generation&&row.sequence==previous->sequence+1&&row.receipt_ms>=previous->receipt_ms)values.push_back(static_cast<double>(row.receipt_ms-previous->receipt_ms));previous=&row;
    }return values;
}
std::vector<ByteVariation> monitor_variation(const std::vector<TrafficRecord>& frames,size_t first,size_t last){
    if(first>=frames.size()||last>=frames.size()||first>=last||last-first>=256)throw std::runtime_error("Select 2..256 consecutive snapshot frames.");
    const auto length=frames[first].bytes.size();if(!length||length>4096||(last-first+1)>1024*1024/length)throw std::runtime_error("Variation is limited to 4096 bytes/frame and 1 MiB total.");
    for(size_t i=first;i<=last;++i)if(frames[i].bytes.size()!=length||frames[i].tx!=frames[first].tx||frames[i].generation!=frames[first].generation)throw std::runtime_error("Select equal-length frames from one direction and connection generation.");
    std::vector<ByteVariation> rows(length);
    for(size_t offset=0;offset<length;++offset){auto& row=rows[offset];row.offset=offset;std::array<bool,256> seen{};
        for(size_t i=first;i<=last;++i){const auto value=static_cast<unsigned char>(frames[i].bytes[offset]);seen[value]=true;if(i>first){const auto changed=value^static_cast<unsigned char>(frames[i-1].bytes[offset]);for(unsigned bit=0;bit<8;++bit)if(changed&(1u<<bit))++row.transitions[bit];}}
        row.distinct=static_cast<size_t>(std::count(seen.begin(),seen.end(),true));
    }return rows;
}
