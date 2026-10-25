#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
struct SourceLine {std::string text;uint64_t offset=UINT64_MAX,count=0;};
class SourceLineTracker {
public:
    std::vector<SourceLine> feed(std::string_view bytes);
    std::vector<SourceLine> finish();
    uint64_t end() const {return end_;}
    void clear_partial() {line_.clear();discard_=false;start_=end_;}
    void skip_bytes(size_t count){end_+=count;clear_partial();}
    uint64_t skipped() const {return skipped_;}
private:
    uint64_t end_=0,start_=0,skipped_=0;
    std::string line_;bool discard_=false;
};
