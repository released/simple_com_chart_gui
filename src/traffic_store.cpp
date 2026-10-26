#include "traffic_store.h"
#include <algorithm>
#include <windows.h>

double traffic_monotonic_seconds(){static LARGE_INTEGER frequency=[]{LARGE_INTEGER value{};QueryPerformanceFrequency(&value);return value;}();LARGE_INTEGER value{};QueryPerformanceCounter(&value);return static_cast<double>(value.QuadPart)/frequency.QuadPart;}

TrafficStore::TrafficStore(size_t capacity,size_t max_records)
    :capacity_(std::max<size_t>(1,capacity)),max_records_(std::max<size_t>(1,max_records)){}
void TrafficStore::append(std::string_view bytes,int64_t receipt_ms,bool tx,double sample_seconds,uint64_t generation){
    if(bytes.empty())return;
    if(sample_seconds<=0)sample_seconds=traffic_monotonic_seconds();
    std::lock_guard<std::mutex> lock(mutex_);
    (tx?tx_:rx_)+=bytes.size();
    size_t cropped=0;
    if(bytes.size()>capacity_){cropped=bytes.size()-capacity_;expired_+=bytes.size()-capacity_;bytes.remove_prefix(bytes.size()-capacity_);}
    records_.push_back({++sequence_,receipt_ms,tx,std::string(bytes),sample_seconds,generation,sequence_,cropped,cropped+bytes.size()});bytes_+=bytes.size();
    while(bytes_>capacity_ || records_.size()>max_records_){
        bytes_-=records_.front().bytes.size();expired_+=records_.front().bytes.size();records_.pop_front();
    }
}
void TrafficStore::clear(){std::lock_guard<std::mutex> lock(mutex_);records_.clear();bytes_=0;expired_=0;++revision_;}
TrafficBatch TrafficStore::read_locked(uint64_t sequence,size_t limit)const{
    TrafficBatch out;out.revision=revision_;out.rx=rx_;out.tx=tx_;out.expired=expired_;out.newest=sequence_;
    for(const auto& record:records_)if(record.sequence>sequence){out.records.push_back(record);if(out.records.size()>=limit)break;}
    return out;
}
TrafficBatch TrafficStore::after(uint64_t sequence,size_t limit)const{
    std::lock_guard<std::mutex> lock(mutex_);return read_locked(sequence,std::max<size_t>(1,limit));
}
TrafficBatch TrafficStore::snapshot()const{std::lock_guard<std::mutex> lock(mutex_);return read_locked(0,max_records_);}
