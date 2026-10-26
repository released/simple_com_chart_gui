#pragma once
#include "session_manifest.h"
struct RecordingNote {uint64_t offset=0,count=0;std::string text;};
struct RecordingNotes {SessionEntry source;std::vector<RecordingNote> notes;};
RecordingNotes bind_notes(const std::filesystem::path&,const std::atomic<bool>&);
void verify_notes(const RecordingNotes&,const std::filesystem::path&,const std::atomic<bool>&);
void validate_notes(const RecordingNotes&);
std::string encode_notes(const RecordingNotes&);
RecordingNotes decode_notes(std::string_view);
