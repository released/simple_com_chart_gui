#pragma once
#include "traffic_store.h"
#include <array>
struct ForwardOptions {enum Direction{AtoB,BtoA,Both} direction=AtoB;unsigned seconds=300;uint64_t byte_limit=1024*1024;};
class SerialForwarder {
public:
    bool start(ForwardOptions,std::array<uint64_t,2> generations,uint64_t now,std::string& error);
    void receive(size_t,const TrafficRecord&,uint64_t now);
    bool next(size_t destination,std::string& bytes);
    void complete(size_t destination,bool success,size_t accepted);
    void tick(uint64_t now);
    void stop(std::string reason);
    bool active()const{return active_;}
    const std::string& status()const{return status_;}
    uint64_t accepted()const{return accepted_;}
    size_t queued_bytes()const{return queued_bytes_;}
private:
    void finish();
    ForwardOptions options_;std::array<uint64_t,2> generations_{};
    std::array<std::deque<std::string>,2> pending_;
    std::array<std::string,2> inflight_,last_sent_;
    uint64_t deadline_=0,received_=0,accepted_=0;size_t queued_bytes_=0,queued_count_=0;
    bool active_=false,draining_=false;std::string status_="Stopped";
};
