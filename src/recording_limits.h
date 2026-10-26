#pragma once
#include <cstdint>
#include <string>
#include <string_view>
struct RecordingLimits {
    uint64_t bytes=0, seconds=0, free_bytes=0;
};
std::string recording_limit_reason(const RecordingLimits&,uint64_t written,uint64_t next,
    uint64_t elapsed_ms,uint64_t available);
std::string encode_recording_limits(const RecordingLimits&);
RecordingLimits decode_recording_limits(std::string_view);
