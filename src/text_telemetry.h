#pragma once
#include "field_extractor.h"
#include "line_filter.h"
#include "source_mapping.h"
#include "traffic_store.h"
#include <deque>
#include "pipeline_diagnostics.h"
struct TextTelemetryConfig {
    bool enabled=false,regex=false;
    std::string include,exclude;
    ExtractionConfig extraction;
};
struct TextTelemetryFrame {std::string line;std::unordered_map<std::string,double> values;double seconds=0;uint64_t generation=UINT64_MAX;};
std::wstring pack_text_settings(const std::vector<std::wstring>&);
std::vector<std::wstring> unpack_text_settings(const std::wstring&);
class TextTelemetry {
public:
    bool configure(const TextTelemetryConfig&,std::string&);
    void receive(const TrafficRecord&);
    void framing(const TextFraming& c){lines_.configure(c);interrupt();}
    void drain(double now);
    std::vector<TextTelemetryFrame> take(size_t limit=64);
    void interrupt();
    const TextTelemetryConfig& config()const{return config_;}
    uint64_t skipped()const{return skipped_+lines_.skipped();}
    const PipelineDiagnostics& diagnostics()const{return diagnostics_;}
    void reset_diagnostics(){diagnostics_.reset();}
private:
    PipelineDiagnostics diagnostics_;
    TextTelemetryConfig config_;
    FieldExtractor extractor_;LineFilter filter_;SourceLineTracker lines_;
    std::deque<std::pair<SourceLine,double>> pending_;size_t bytes_=0;
    uint64_t generation_=UINT64_MAX,skipped_=0;
};
