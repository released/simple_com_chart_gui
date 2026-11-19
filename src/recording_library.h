#pragma once
#include "monitor_features.h"
#include <filesystem>
#include <atomic>
struct RecordingReference {RecentResource resource;std::wstring tags,version,source,modified,status;uint64_t bytes=0;};
struct RecordingLibrary {std::vector<RecordingReference> entries;std::string original;bool existed=false;};
std::string encode_recording_library(const std::vector<RecordingReference>&);
std::vector<RecordingReference> decode_recording_library(std::string_view);
RecordingLibrary read_recording_library(const std::filesystem::path&,const std::vector<RecentResource>&,const std::atomic<bool>&);
void save_recording_reference(const std::filesystem::path&,const RecordingLibrary&,RecordingReference,const std::atomic<bool>&);
