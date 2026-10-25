#include "source_mapping.h"
std::vector<SourceLine> SourceLineTracker::feed(std::string_view bytes) {
    std::vector<SourceLine> result;
    for(const char c:bytes) {const auto offset=end_++;
        if(c=='\n') {if(!discard_) {while(!line_.empty() && line_.back()=='\r') line_.pop_back();if(!line_.empty()) result.push_back({std::move(line_),start_,end_-start_});}line_.clear();discard_=false;start_=end_;}
        else if(!discard_) {if(line_.empty()) start_=offset;line_+=c;if(line_.size()>65536) {line_.clear();discard_=true;++skipped_;}}
    }return result;
}
std::vector<SourceLine> SourceLineTracker::finish() {std::vector<SourceLine> result;if(!discard_&&!line_.empty()) {while(!line_.empty()&&line_.back()=='\r') line_.pop_back();if(!line_.empty()) result.push_back({std::move(line_),start_,end_-start_});}clear_partial();return result;}
