#pragma once
#include "bounded_regex.h"
#include <optional>
#include <unordered_map>

struct ExtractionConfig {
    enum Mode { KeyValue, Csv, Tab, Json, Regex } mode=KeyValue;
    std::string columns, pattern, time_field;
    int name_group=1,value_group=2,time_group=0;
};
struct ExtractedFields {
    std::unordered_map<std::string,double> values;
    std::optional<double> seconds;
    bool limited=false;
    std::string error;
};
class FieldExtractor {
public:
    bool configure(const ExtractionConfig&,std::string& error);
    ExtractedFields extract(const std::string&) const;
    const ExtractionConfig& config() const {return config_;}
private:
    ExtractionConfig config_;
    BoundedRegex regex_;
    std::vector<std::string> columns_;
};
std::string encode_extraction(const ExtractionConfig&);
ExtractionConfig decode_extraction(const std::string&);
