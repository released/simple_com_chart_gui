#pragma once
#include "session_metadata.h"
#include "replay_storage.h"
struct SessionEntry {std::wstring leaf;uint64_t bytes=0;std::string sha256;};
struct SessionManifest {CaptureMetadata metadata;std::wstring source;unsigned interval_ms=100;std::vector<SessionEntry> files;};
std::string file_sha256(const ReplayFile&,const std::atomic<bool>&);
std::string encode_session_manifest(const SessionManifest&);
SessionManifest decode_session_manifest(std::string_view);
SessionManifest create_session_manifest(const std::filesystem::path& source,const std::filesystem::path& output,
    CaptureMetadata current,bool capture_start,bool include_log,unsigned interval_ms,const std::atomic<bool>&);
SessionManifest verify_session_manifest(const std::filesystem::path&,const std::atomic<bool>&);
std::string session_manifest_report(const SessionManifest&);
