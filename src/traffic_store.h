#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

struct TrafficRecord {
    uint64_t sequence=0;
    int64_t receipt_ms=0;
    bool tx=false;
    std::string bytes;
    double sample_seconds=0;
    uint64_t generation=0;
};
double traffic_monotonic_seconds();
struct TrafficBatch {
    uint64_t revision=0, rx=0, tx=0, expired=0, newest=0;
    std::vector<TrafficRecord> records;
};
// One instance per port. It never calls a window, driver or file operation.
class TrafficStore {
public:
    explicit TrafficStore(size_t capacity=256*1024,size_t max_records=2048);
    void append(std::string_view bytes,int64_t receipt_ms,bool tx=false,double sample_seconds=0,uint64_t generation=0);
    void clear();
    TrafficBatch after(uint64_t sequence,size_t limit=64) const;
    TrafficBatch snapshot() const;
private:
    TrafficBatch read_locked(uint64_t sequence,size_t limit) const;
    mutable std::mutex mutex_;
    std::deque<TrafficRecord> records_;
    size_t capacity_, max_records_, bytes_=0;
    uint64_t sequence_=0, revision_=0, rx_=0, tx_=0, expired_=0;
};
