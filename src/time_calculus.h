#pragma once
#include "capture_store.h"
#include <atomic>
struct CalculusPoint {double seconds=0,value=0;uint64_t row=0;size_t segment=0;bool valid=false;};
struct CalculusResult {std::vector<CalculusPoint> points;std::string report,csv;};
CalculusResult time_calculus(const CaptureSnapshot&,const std::string& key,unsigned source,
    bool integral,double maximum_gap,const std::atomic<bool>& cancel);
