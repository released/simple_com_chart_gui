#pragma once
#include "field_extractor.h"
#include "line_filter.h"
#include "source_mapping.h"
#include "traffic_store.h"
#include <deque>
struct TextTelemetryConfig {
    bool enabled=false,regex=false;
    std::string include,exclude;
    ExtractionConfig extraction;
};
struct TextTelemetryFrame {std::string line;std::unordered_map<std::string,double> values;double seconds=0;};
class TextTelemetry {
public:
    bool configure(const TextTelemetryConfig&,std::string&);
    void receive(const TrafficRecord&);
    std::vector<TextTelemetryFrame> take(size_t limit=64);
    void interrupt();
    const TextTelemetryConfig& config()const{return config_;}
    uint64_t skipped()const{return skipped_+lines_.skipped();}
private:
    TextTelemetryConfig config_;
    FieldExtractor extractor_;LineFilter filter_;SourceLineTracker lines_;
    std::deque<std::pair<SourceLine,double>> pending_;size_t bytes_=0;
    uint64_t generation_=UINT64_MAX,skipped_=0;
};
