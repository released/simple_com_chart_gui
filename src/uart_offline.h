#pragma once
#include "uart_files.h"
#include <memory>
#include <cstdint>
#include <vector>
#include "replay_storage.h"
struct ReplayData {
    ReplayRecords records;
    std::vector<std::shared_ptr<ReplayFile>> files;
    std::vector<std::filesystem::path> paths;
    std::string read_record(size_t record,const std::atomic<bool>& cancel) const;
    uint64_t total_bytes=0;
    bool receipt_times = false;
    std::wstring provenance;
};
size_t replay_seek_time(const ReplayData&,double seconds);
size_t replay_find(const ReplayData&,size_t first,std::string_view pattern,const std::atomic<bool>& cancel);
std::shared_ptr<ReplayData> load_replay(const std::filesystem::path& path, unsigned interval_ms, const std::atomic<bool>& cancel, uint64_t input_limit=64ULL*1024*1024*1024);
class ReplayClock {
public:
    void load(std::shared_ptr<const ReplayData> data);
    void play(uint64_t now, double speed);
    void pause(uint64_t now);
    void seek(size_t record, uint64_t now);
    size_t due(uint64_t now, size_t budget = 128);
    bool playing() const { return playing_; }
    size_t position() const { return position_; }
    const std::shared_ptr<const ReplayData>& data() const { return data_; }
private:
    double time(uint64_t now) const;
    std::shared_ptr<const ReplayData> data_;
    size_t position_ = 0;
    double anchor_seconds_ = 0, speed_ = 1;
    uint64_t anchor_ms_ = 0;
    bool playing_ = false;
};
