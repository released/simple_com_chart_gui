#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>
#include "field_extractor.h"
#include "pipeline_diagnostics.h"
#include "capture_store.h"
struct SequenceStats {
    uint64_t samples=0,duplicates=0,jumps=0,backwards=0,wraps=0,invalid=0,resets=0;
};
class SequenceTracker {
public:
    bool configure(uint64_t modulus,uint64_t step,std::string&);
    void receive(double,uint64_t generation);
    const SequenceStats& stats()const{return stats_;}
    void reset(){previous_.reset();generation_=UINT64_MAX;stats_={};}
private:
    uint64_t modulus_=65536,step_=1,generation_=UINT64_MAX;
    std::optional<uint64_t> previous_;
    SequenceStats stats_;
};
struct LatencyStats {size_t count=0;double minimum=0,maximum=0,mean=0,p50=0,p95=0,p99=0;};
LatencyStats latency_stats(std::vector<uint64_t>);
struct ParserCaseReport {size_t passed=0,failed=0;std::string text;};
ParserCaseReport check_parser_cases(const ExtractionConfig&,std::string_view,double tolerance);

struct DigitalLane {std::string name,key;unsigned bit=0,width=1;bool whole=false;};
bool parse_digital_lanes(std::string_view,std::vector<DigitalLane>&,std::string&);
std::optional<uint32_t> digital_value(double,const DigitalLane&);
struct DigitalSegment {double first=0,last=0;uint32_t value=0;std::optional<uint32_t> next;};
struct DigitalHistory {std::vector<DigitalSegment> segments;std::optional<std::pair<double,uint32_t>> latest;};
DigitalHistory digital_history(const CaptureSnapshot&,const DigitalLane&,double window,double maximum_hold);
