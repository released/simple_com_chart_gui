#include "lab_metrics.h"
#include "command_panel.h"
#include "log_parser.h"
#include "uart_files.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

bool SequenceTracker::configure(uint64_t modulus,uint64_t step,std::string& error) {
    if(modulus<2||modulus>4294967296ULL||!step||step>=modulus){error="Sequence modulus 2..4294967296; step 1..modulus-1.";return false;}
    modulus_=modulus;step_=step;reset();return true;
}
void SequenceTracker::receive(double value,uint64_t generation) {
    if(!std::isfinite(value)||value<0||value>=static_cast<double>(modulus_)||std::floor(value)!=value){++stats_.invalid;return;}
    if(generation_!=generation){if(previous_)++stats_.resets;previous_.reset();generation_=generation;}
    const auto current=static_cast<uint64_t>(value);++stats_.samples;
    if(previous_) {
        const auto delta=(current+modulus_-*previous_)%modulus_;
        if(!delta)++stats_.duplicates;
        else if(delta==step_){if(current<*previous_)++stats_.wraps;}
        else if(delta<modulus_/2)++stats_.jumps;
        else ++stats_.backwards;
    }
    previous_=current;
}
LatencyStats latency_stats(std::vector<uint64_t> values) {
    if(values.size()>4096)throw std::runtime_error("Latency sample limit: 4096.");
    LatencyStats s;s.count=values.size();if(values.empty())return s;
    std::sort(values.begin(),values.end());s.minimum=static_cast<double>(values.front());s.maximum=static_cast<double>(values.back());
    long double sum=0;for(const auto v:values)sum+=v;s.mean=static_cast<double>(sum/values.size());
    const auto quantile=[&](double p){return static_cast<double>(values[static_cast<size_t>(std::ceil(p*values.size()))-1]);};
    s.p50=quantile(.50);s.p95=quantile(.95);s.p99=quantile(.99);return s;
}
ParserCaseReport check_parser_cases(const ExtractionConfig& config,std::string_view input,double tolerance) {
    if(!std::isfinite(tolerance)||tolerance<0||tolerance>1e12)throw std::runtime_error("Invalid absolute tolerance.");
    FieldExtractor parser;std::string error;if(!parser.configure(config,error))throw std::runtime_error(error);
    std::vector<std::vector<std::string>> rows;
    if(!uart_csv_table(input,rows,error,65536,65,3))throw std::runtime_error(error);
    if(rows.empty()||rows.front()!=std::vector<std::string>{"Input","Expected","Seconds"})throw std::runtime_error("First CSV row must be Input,Expected,Seconds (max 64 cases).");
    ParserCaseReport report;std::ostringstream out;
    for(size_t i=1;i<rows.size();++i) {
        const auto& r=rows[i];if(r.size()!=3||r[0].size()>4096)throw std::runtime_error("Cases require three columns; input <=4096 bytes.");
        const auto actual=parser.extract(r[0]);std::unordered_map<std::string,double> expected;
        if(r[1]!="ERROR"&&!r[1].empty()) {
            std::istringstream pairs(r[1]);std::string pair;
            while(std::getline(pairs,pair,',')){const auto at=pair.find_first_of(":=");if(at==pair.npos)throw std::runtime_error("Every expected field needs key:value.");
                auto key=pair.substr(0,at),value=pair.substr(at+1);const auto first=key.find_first_not_of(" \t"),last=key.find_last_not_of(" \t");if(first==key.npos)throw std::runtime_error("Expected key cannot be empty.");key=key.substr(first,last-first+1);double n=0;
                if(!log_parser::parse_numeric_value(value,n)||!expected.emplace(key,n).second)throw std::runtime_error("Expected numeric fields must be valid and unique; no assertion is silently skipped.");}
            if(r[1].back()==',')throw std::runtime_error("Expected fields cannot end with an empty item.");
        }
        bool pass=false;
        if(r[1]=="ERROR")pass=actual.limited||!actual.error.empty();
        else {
            if(!r[1].empty()&&expected.empty())throw std::runtime_error("Expected must contain numeric key:value pairs, be empty, or ERROR.");
            pass=actual.error.empty()&&!actual.limited&&expected.size()==actual.values.size();
            for(const auto& item:expected){const auto found=actual.values.find(item.first);if(found==actual.values.end()||std::abs(found->second-item.second)>tolerance)pass=false;}
            if(!r[2].empty()){double seconds=0;const auto parsed=std::from_chars(r[2].data(),r[2].data()+r[2].size(),seconds);if(parsed.ec!=std::errc{}||parsed.ptr!=r[2].data()+r[2].size()||!std::isfinite(seconds))throw std::runtime_error("Invalid expected seconds.");if(!actual.seconds||std::abs(*actual.seconds-seconds)>tolerance)pass=false;}
        }
        if(pass)++report.passed;else++report.failed;
        out<<"Case "<<i<<": "<<(pass?"PASS":"FAIL")<<"; fields="<<actual.values.size();if(!actual.error.empty())out<<"; "<<actual.error;out<<"\r\n";
        if(!pass)for(const auto& value:actual.values)out<<"  "<<value.first<<"="<<std::setprecision(12)<<value.second<<"\r\n";
    }
    if(rows.size()<2)throw std::runtime_error("Add at least one case.");
    report.text="Passed "+std::to_string(report.passed)+", failed "+std::to_string(report.failed)+"\r\n"+out.str();return report;
}
bool parse_digital_lanes(std::string_view text,std::vector<DigitalLane>& result,std::string& error) {
    std::vector<std::vector<std::string>> rows;if(!uart_csv_table(text,rows,error,4096,8,4))return false;
    std::vector<DigitalLane> lanes;
    for(const auto& row:rows){DigitalLane lane;if(row.size()!=4||row[0].empty()||row[0].size()>64||row[1].empty()||row[1].size()>160){error="Lanes: name,key,first-bit,width; max 8 (width 0 = whole enum).";return false;}
        unsigned first=0,width=0;const auto a=std::from_chars(row[2].data(),row[2].data()+row[2].size(),first),b=std::from_chars(row[3].data(),row[3].data()+row[3].size(),width);
        if(a.ec!=std::errc{}||a.ptr!=row[2].data()+row[2].size()||b.ec!=std::errc{}||b.ptr!=row[3].data()+row[3].size()||first>31||width>32||first+width>32||(width==0&&first!=0)){error="Bits must fit unsigned 32-bit values; whole enum requires 0,0.";return false;}
        try{uart_wide(row[0]);uart_wide(row[1]);}catch(...){error="Digital lane names/keys must be valid UTF-8.";return false;}
        lane.name=row[0];lane.key=row[1];lane.bit=first;lane.width=width;lane.whole=width==0;lanes.push_back(std::move(lane));}
    result=std::move(lanes);return true;
}
std::optional<uint32_t> digital_value(double value,const DigitalLane& lane) {
    if(lane.bit>31||(!lane.whole&&(!lane.width||lane.width>32||lane.bit+lane.width>32)))return {};
    if(!std::isfinite(value)||value<0||value>4294967295.0||std::floor(value)!=value)return {};
    const auto bits=static_cast<uint32_t>(value);if(lane.whole)return bits;
    return (bits>>lane.bit)&(lane.width==32?UINT32_MAX:((uint32_t{1}<<lane.width)-1));
}
