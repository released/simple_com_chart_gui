#pragma once
#include "uart_recorder.h"
#include <memory>

bool recording_same_path(const std::filesystem::path& a,const std::filesystem::path& b);
RecordingOverwrite recording_file_identity(const std::filesystem::path& path);
std::vector<RecordingOverwrite> existing_recording_files(const RecordingOptions& options);
std::filesystem::path recording_log_path(const RecordingOptions& options,uint64_t segment);

// A confirmed session replacement is staged before publishing any new stream.
class RecordingReplacement {
public:
    explicit RecordingReplacement(const RecordingOptions& options);
    ~RecordingReplacement();
    std::filesystem::path output(const std::filesystem::path& destination);
    void created(const std::filesystem::path& destination);
    void commit();
    void validate_output(const std::filesystem::path& destination) const;
    void finish();
    std::string rollback();
private:
    struct State;
    std::unique_ptr<State> state_;
};
