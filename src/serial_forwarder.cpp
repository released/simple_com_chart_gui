#include "serial_forwarder.h"
#include <algorithm>
bool SerialForwarder::start(ForwardOptions options,std::array<uint64_t,2> generations,uint64_t now,std::string& error){
    if(active_||static_cast<unsigned>(options.direction)>2||options.seconds<1||options.seconds>3600||options.byte_limit<1||options.byte_limit>1024ULL*1024*1024){error="Stop forwarding first; direction/1..3600 s/1..1024 MiB limit required.";return false;}
    options_=options;generations_=generations;deadline_=now+options.seconds*1000ULL;pending_={};inflight_={};last_sent_={};received_=accepted_=0;queued_bytes_=queued_count_=0;active_=true;draining_=false;status_="Forwarding (driver acceptance only)";return true;
}
void SerialForwarder::receive(size_t source,const TrafficRecord& record,uint64_t now){
    tick(now);if(!active_||source>1||record.tx||record.bytes.empty())return;
    if(record.generation!=generations_[source]){stop("STOPPED: connection generation changed; no resume");return;}
    if(options_.direction!=ForwardOptions::Both&&source!=(options_.direction==ForwardOptions::AtoB?0:1))return;
    if(draining_){stop("STOPPED: additional RX exceeded the selected byte budget");return;}
    if(options_.direction==ForwardOptions::Both&&!last_sent_[source].empty()&&record.bytes==last_sent_[source]){stop("STOPPED: possible device echo loop; choose one-way forwarding or review wiring");return;}
    const auto count=(record.bytes.size()+4095)/4096;
    if(record.bytes.size()>options_.byte_limit-received_){stop("STOPPED: RX would exceed byte budget");return;}
    if(queued_count_+count>64||queued_bytes_+record.bytes.size()>256*1024){stop("STOPPED: forwarding queue full; no silent loss/retry");return;}
    received_+=record.bytes.size();for(size_t offset=0;offset<record.bytes.size();offset+=4096){auto bytes=record.bytes.substr(offset,4096);queued_bytes_+=bytes.size();++queued_count_;pending_[1-source].push_back(std::move(bytes));}
    draining_=received_==options_.byte_limit;
}
bool SerialForwarder::next(size_t destination,std::string& bytes){if(!active_||destination>1||!inflight_[destination].empty()||pending_[destination].empty())return false;inflight_[destination]=std::move(pending_[destination].front());pending_[destination].pop_front();queued_bytes_-=inflight_[destination].size();--queued_count_;bytes=inflight_[destination];last_sent_[destination]=bytes;return true;}
void SerialForwarder::complete(size_t destination,bool success,size_t accepted){if(!active_||destination>1||inflight_[destination].empty())return;accepted_+=std::min(accepted,inflight_[destination].size());if(!success||accepted!=inflight_[destination].size()){stop("STOPPED: partial/error/cancelled TX; accepted bytes may have affected target; no retry");return;}inflight_[destination].clear();finish();}
void SerialForwarder::finish(){if(active_&&draining_&&!queued_count_&&inflight_[0].empty()&&inflight_[1].empty()){active_=false;status_="COMPLETED: selected byte budget forwarded; no auto-resume";}}
void SerialForwarder::tick(uint64_t now){if(active_&&now>=deadline_)stop("STOPPED: forwarding time limit reached; no resume");}
void SerialForwarder::stop(std::string reason){active_=false;pending_={};inflight_={};queued_bytes_=queued_count_=0;status_=std::move(reason);}
