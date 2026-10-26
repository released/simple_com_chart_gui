#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
class PipelineDiagnostics {
public:
    enum Reason { Accepted,IncludeMiss,Excluded,FilterLimit,ParserError,NoValues,TimeError,ReasonCount };
    void observe(Reason,std::string_view);
    void reset(){counts_={};examples_={};}
    std::string report()const;
private:
    std::array<uint64_t,ReasonCount> counts_{};
    std::array<std::deque<std::string>,ReasonCount> examples_;
};
