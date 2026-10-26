#pragma once
#include "uart_offline.h"
struct RecordingHealth {
    uint64_t raw_bytes=0,valid_bytes=0;
    bool complete=false,input_gap=false;
    std::string detail;
    std::shared_ptr<ReplayData> prefix;
    std::string report()const;
};
RecordingHealth inspect_recording(const std::filesystem::path&,const std::atomic<bool>&);
std::string export_recording_range(const ReplayData&,size_t first,size_t past_last,
    const std::filesystem::path& output,const std::atomic<bool>&);
