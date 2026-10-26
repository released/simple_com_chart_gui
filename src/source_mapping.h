#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
struct TextFraming {
    enum Mode {LF,CR,CRLF,Delimiter,Idle} mode=LF;
    std::string delimiter;
    unsigned idle_ms=20;
};
std::string encode_text_framing(const TextFraming&);
TextFraming decode_text_framing(const std::string&);
struct SourceLine {std::string text;uint64_t offset=UINT64_MAX,count=0;double seconds=0;};
class SourceLineTracker {
public:
    void configure(const TextFraming&);
    const TextFraming& config()const{return framing_;}
    std::vector<SourceLine> feed(std::string_view bytes,double seconds=0);
    std::vector<SourceLine> drain(double seconds);
    std::vector<SourceLine> finish();
    uint64_t end() const {return end_;}
    void clear_partial() {line_.clear();discard_=false;start_=end_;match_=0;last_=0;skip_lf_=false;}
    void skip_bytes(size_t count){end_+=count;clear_partial();}
    uint64_t skipped() const {return skipped_;}
private:
    uint64_t end_=0,start_=0,skipped_=0;
    std::string line_;bool discard_=false;
    TextFraming framing_;
    std::string delimiter_="\n";
    std::vector<size_t> prefix_{0};
    size_t match_=0;double last_=0;
    bool skip_lf_=false;
};
