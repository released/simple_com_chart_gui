#pragma once
#include "capture_store.h"
#include "traffic_store.h"
#include <array>
#include <atomic>
#include <algorithm>

struct Distribution {
    size_t count=0;
    double low=0,high=0,p50=0,p95=0,p99=0;
    std::array<size_t,32> bins{};
};
Distribution monitor_distribution(std::vector<double>,const std::atomic<bool>&);
struct TimelineRow {unsigned source=0;TrafficRecord record;};
std::vector<TimelineRow> monitor_timeline(const TrafficBatch&,const TrafficBatch&,const std::atomic<bool>&);
std::vector<double> monitor_values(const CaptureSnapshot&,const std::string& key,unsigned source,double first,double last,const std::atomic<bool>&);
std::vector<double> monitor_intervals(const TrafficBatch&,const std::atomic<bool>&);
struct ByteVariation {size_t offset=0,distinct=0;std::array<size_t,8> transitions{};};
std::vector<ByteVariation> monitor_variation(const std::vector<TrafficRecord>&,size_t first,size_t last);

// Presentation scheduling never controls acquisition or numeric consumption.
struct PresentationCadence {
    unsigned interval=50;
    uint64_t last_begin=0,last_end=0,pending=0;
    double cost=0,maximum=0;
    bool due(uint64_t now)const{return interval==50||!last_begin||now<last_begin||now-last_begin>=interval;}
    void finish(uint64_t begin,uint64_t end,double elapsed=-1){last_begin=begin;last_end=end;cost=elapsed>=0?elapsed:static_cast<double>(end>=begin?end-begin:0);maximum=std::max(maximum,cost);pending=0;}
};

struct MonitorAnalysisResult {
    std::wstring title,summary;
    std::wstring first_column_sample;
    std::vector<std::wstring> columns;
    std::vector<std::vector<std::wstring>> rows;
};
