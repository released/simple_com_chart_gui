#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <deque>

enum class WireCodec { None, Cobs, Slip };
std::string wire_encode(std::string_view, WireCodec);
class WireDecoder {
public:
    struct Stamp {int64_t ms=0;double seconds=0;};
    void configure(WireCodec mode) { mode_=mode; reset(); }
    void reset() { pending_.clear();queued_.clear();arrivals_.clear();stamps_.clear();dropping_=false; }
    std::vector<std::string> feed(std::string_view,int64_t receipt_ms=0,double seconds=0);
    uint64_t errors() const { return errors_; }
    size_t pending_bytes()const{return pending_.size()+queued_.size();}
    const std::vector<Stamp>& stamps()const{return stamps_;}
private:
    struct Arrival {size_t count=0;Stamp stamp;};
    std::deque<Arrival> arrivals_;
    std::vector<Stamp> stamps_;
    WireCodec mode_=WireCodec::None;
    std::string pending_;
    std::string queued_;
    bool dropping_=false;
    uint64_t errors_=0;
};
