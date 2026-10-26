#include "recording_limits.h"
#include <charconv>
#include <stdexcept>
#include <vector>
std::string recording_limit_reason(const RecordingLimits& l,uint64_t written,uint64_t next,uint64_t elapsed,uint64_t available) {
    if(l.seconds&&elapsed/1000>=l.seconds)return "elapsed-time limit";
    if(l.bytes&&(written>l.bytes||next>l.bytes-written))return "total output byte limit";
    if(l.free_bytes&&(available<l.free_bytes||next>available-l.free_bytes))return "minimum available disk space";
    return {};
}
std::string encode_recording_limits(const RecordingLimits& l) {
    if(l.seconds>604800||l.bytes>1099511627776ULL||l.free_bytes>1099511627776ULL)throw std::runtime_error("Recording limits: max 1 TiB / 7 days; 0 disables a limit.");
    return "v1:"+std::to_string(l.bytes)+":"+std::to_string(l.seconds)+":"+std::to_string(l.free_bytes);
}
RecordingLimits decode_recording_limits(std::string_view s) {
    RecordingLimits l;if(s.empty())return l;
    if(s.substr(0,3)!="v1:")throw std::runtime_error("Unsupported recording limits.");s.remove_prefix(3);
    uint64_t* out[]={&l.bytes,&l.seconds,&l.free_bytes};
    for(size_t n=0;n<3;++n){const auto at=s.find(':');if((n<2)==(at==s.npos))throw std::runtime_error("Invalid recording limits.");const auto token=s.substr(0,at);const auto r=std::from_chars(token.data(),token.data()+token.size(),*out[n]);if(r.ec!=std::errc{}||r.ptr!=token.data()+token.size()||token.empty())throw std::runtime_error("Invalid recording limit number.");if(at!=s.npos)s.remove_prefix(at+1);}
    encode_recording_limits(l);return l;
}
