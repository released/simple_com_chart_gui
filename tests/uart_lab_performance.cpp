#include "line_filter.h"
#include "field_extractor.h"
#include "pipeline_diagnostics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
volatile size_t consumed=0;
double measure(bool diagnostics,const std::string& input,ExtractionConfig::Mode mode,size_t count) {
    LineFilter filter;FieldExtractor parser;ExtractionConfig config;config.mode=mode;std::string error;
    if(!filter.configure("fan","error",false,error)||!parser.configure(config,error))throw std::runtime_error("Benchmark configuration failed.");
    PipelineDiagnostics observations;size_t total=0;const auto begin=std::chrono::steady_clock::now();
    for(size_t i=0;i<count;++i) {
        const auto decision=filter.evaluate(input);
        if(decision!=LineFilter::Accept){if(diagnostics)observations.observe(decision==LineFilter::Excluded?PipelineDiagnostics::Excluded:PipelineDiagnostics::IncludeMiss,input);continue;}
        const auto fields=parser.extract(input);total+=fields.values.size();
        if(diagnostics)observations.observe(fields.limited||!fields.error.empty()?PipelineDiagnostics::ParserError:fields.values.empty()?PipelineDiagnostics::NoValues:PipelineDiagnostics::Accepted,input);
    }
    consumed=total;return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
}
}
int main() {
    try {
        // Acceptance chosen before measurement: median overhead <=15%, or <=5 ms
        // per 100000 lines for very fast paths. This isolates diagnostic overhead;
        // end-to-end responsiveness/loss is checked separately by the RX soak.
        struct Case {const char* name;std::string line;ExtractionConfig::Mode mode;};
        const std::vector<Case> cases{{"numeric","fan,duty:920,target:2340,pri_ntc:76,fg_hz:69.1,fg_rpm:2074,err:11%,cap:105/76/29,cmd:0x3202",ExtractionConfig::KeyValue},
            {"include-miss","ac_adc_mv:1169,calibrated:1169,ac_v:116,ocp_mv:377,ntc_mv:2439",ExtractionConfig::KeyValue},
            {"excluded","fan,error,rpm:2000",ExtractionConfig::KeyValue},{"no-values","fan status ready",ExtractionConfig::KeyValue},
            {"parser-error","fan invalid JSON",ExtractionConfig::Json}};
        bool passed=true;constexpr size_t count=100000;
        for(const auto& c:cases) {
            measure(false,c.line,c.mode,2000);measure(true,c.line,c.mode,2000);std::vector<double> before,after;
            for(int turn=0;turn<5;++turn){if(turn%2){after.push_back(measure(true,c.line,c.mode,count));before.push_back(measure(false,c.line,c.mode,count));}else{before.push_back(measure(false,c.line,c.mode,count));after.push_back(measure(true,c.line,c.mode,count));}}
            std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());const double limit=before[2]+std::max(5.,before[2]*.15);const bool ok=after[2]<=limit;passed=passed&&ok;
            std::cout<<(ok?"PASS":"FAIL")<<": "<<c.name<<" lines="<<count<<" baseline_ms="<<before[2]<<" diagnostics_ms="<<after[2]<<" acceptance_ms="<<limit<<" repeats=5\n";
        }
        return passed?0:1;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
