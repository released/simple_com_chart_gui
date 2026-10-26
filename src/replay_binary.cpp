#include "replay_binary.h"
#include <stdexcept>
std::vector<BinaryFrame> ReplayBinary::feed(const ReplayRecord& record,std::string_view bytes){
    if(generation_!=record.generation){decoder.interrupt();generation_=record.generation;}
    auto frames=decoder.feed(bytes,record.unix_ms,record.seconds+1);
    for(unsigned turn=0;turn<4096&&decoder.pending_bytes();++turn){const auto before=decoder.pending_bytes();auto next=decoder.feed({},0);
        for(auto& frame:next)frames.push_back(std::move(frame));
        if(frames.size()>8192)throw std::runtime_error("Too many replay frames per record.");
        if(decoder.pending_bytes()==before)break;
    }for(auto& frame:frames)frame.generation=record.generation;return frames;
}
