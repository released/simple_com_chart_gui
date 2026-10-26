#include "wire_codec.h"
#include <stdexcept>

std::string wire_encode(std::string_view input,WireCodec mode) {
    if(input.size()>65536) throw std::runtime_error("Codec input exceeds 65536 bytes.");
    if(mode==WireCodec::None) return std::string(input);
    if(mode!=WireCodec::Cobs&&mode!=WireCodec::Slip) throw std::runtime_error("Unknown codec.");
    std::string out;
    if(mode==WireCodec::Slip) {
        out+='\xC0';
        for(unsigned char c:input) {
            if(c==0xC0) out+="\xDB\xDC";
            else if(c==0xDB) out+="\xDB\xDD";
            else out+=static_cast<char>(c);
        }
        return out+'\xC0';
    }
    size_t code_at=0;unsigned code=1;out+='\0';
    for(char c:input) {
        if(!c) {out[code_at]=static_cast<char>(code);code_at=out.size();out+='\0';code=1;}
        else {out+=c;if(++code==255){out[code_at]=static_cast<char>(code);code_at=out.size();out+='\0';code=1;}}
    }
    out[code_at]=static_cast<char>(code);return out+'\0';
}
std::vector<std::string> WireDecoder::feed(std::string_view input,int64_t receipt_ms,double seconds) {
    if(input.size()>131074) throw std::runtime_error("Codec dispatch exceeds 131074 bytes.");
    stamps_.clear();
    if(mode_==WireCodec::None){if(!input.empty())stamps_.push_back({receipt_ms,seconds});return input.empty()?std::vector<std::string>{}:std::vector<std::string>{std::string(input)};}
    if(queued_.size()+input.size()>262148||(!input.empty()&&arrivals_.size()>=4096)){queued_.clear();arrivals_.clear();pending_.clear();dropping_=true;++errors_;}
    if(!input.empty()){queued_.append(input);if(!arrivals_.empty()&&arrivals_.back().stamp.ms==receipt_ms&&arrivals_.back().stamp.seconds==seconds)arrivals_.back().count+=input.size();else arrivals_.push_back({input.size(),{receipt_ms,seconds}});}
    std::vector<std::string> result;
    size_t consumed=0;
    while(consumed<queued_.size()&&result.size()<256) {
        const unsigned char c=static_cast<unsigned char>(queued_[consumed++]);const auto stamp=arrivals_.front().stamp;
        if(!--arrivals_.front().count)arrivals_.pop_front();
        const bool end=c==(mode_==WireCodec::Cobs?0:0xC0);
        if(end) {
            if(dropping_){dropping_=false;pending_.clear();continue;}
            if(pending_.empty())continue;
            std::string decoded;bool valid=true;
            for(size_t at=0;at<pending_.size()&&valid;) {
                unsigned char b=static_cast<unsigned char>(pending_[at++]);
                if(mode_==WireCodec::Slip) {
                    if(b==0xDB) {
                        if(at==pending_.size()){valid=false;break;}
                        b=static_cast<unsigned char>(pending_[at++]);
                        if(b==0xDC)b=0xC0;else if(b==0xDD)b=0xDB;else{valid=false;break;}
                    }
                    decoded+=static_cast<char>(b);
                } else {
                    const size_t count=b-1;
                    if(count>pending_.size()-at){valid=false;break;}
                    decoded.append(pending_,at,count);at+=count;
                    if(b<255&&at<pending_.size())decoded+='\0';
                }
                if(decoded.size()>65536)valid=false;
            }
            if(!valid)++errors_;else{result.push_back(std::move(decoded));stamps_.push_back(stamp);}
            pending_.clear();
        } else if(!dropping_) {
            if(pending_.size()>=131073){pending_.clear();dropping_=true;++errors_;}
            else pending_+=static_cast<char>(c);
        }
    }
    if(consumed)queued_.erase(0,consumed);
    return result;
}
