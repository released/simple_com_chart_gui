#include "pipeline_diagnostics.h"
#include <sstream>
void PipelineDiagnostics::observe(Reason reason,std::string_view line) {
    ++counts_[reason];if(reason==Accepted||(counts_[reason]>2&&(counts_[reason]&255)!=0))return;
    std::string sample;sample.reserve(160);
    for(unsigned char c:line.substr(0,160))sample+=c>=32&&c<127?static_cast<char>(c):'.';
    auto& examples=examples_[reason];if(!examples.empty()&&examples.back()==sample)return;
    if(examples.size()==2)examples.pop_front();examples.push_back(std::move(sample));
}
std::string PipelineDiagnostics::report()const {
    const char* names[]={"Numeric accepted","Include did not match","Excluded","Filter work limit","Extraction error/work limit","No numeric fields","Invalid/backwards extracted time","Channel name exceeds 128 bytes after grouping/indexing"};
    std::ostringstream out;
    for(size_t i=0;i<ReasonCount;++i){out<<names[i]<<": "<<counts_[i]<<"\r\n";for(const auto& sample:examples_[i])out<<"  "<<sample<<"\r\n";}
    return out.str();
}
