#pragma once
#include "traffic_store.h"
#include <array>
struct TrafficFraming {enum Mode{Chunks,LF,CRLF,Fixed,Idle,HeaderFixed} mode=Chunks;size_t bytes=16;unsigned idle_ms=20;std::string header;};
class TrafficFramer {
public:
    bool configure(TrafficFraming,std::string& error);
    std::vector<TrafficRecord> feed(const TrafficRecord&);
    std::vector<TrafficRecord> drain(double now_seconds);
    void reset();
    uint64_t discarded()const{return discarded_;}
    const TrafficFraming& config()const{return config_;}
private:
    struct Pending {std::string bytes;struct Span{size_t count;TrafficRecord receipt;};std::deque<Span> spans;uint64_t generation=0;};
    std::vector<TrafficRecord> extract(size_t,double now_seconds);
    TrafficRecord consume(Pending&,size_t,bool keep=true);
    TrafficFraming config_;
    std::array<Pending,2> pending_;
    uint64_t discarded_=0;
};
