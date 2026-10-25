#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cstdint>

struct RecordingOptions {
    std::filesystem::path base;
    uint64_t rotate_bytes = 32 * 1024 * 1024;
    bool rotate_daily = true;
};
struct RecordingStatus {
    bool active = false, stopping = false;
    uint64_t bytes = 0, tx_bytes = 0, lost = 0, segments = 0;
    std::wstring detail = L"Recording off";
};

// Bounded producer queue; all filesystem I/O belongs to the single worker.
class UartRecorder {
public:
    UartRecorder();
    ~UartRecorder();
    bool start(RecordingOptions options);
    void stop();
    void append(std::string_view bytes, int64_t unix_ms, bool tx = false);
    void event(std::string text, int64_t unix_ms);
    RecordingStatus status() const;
    bool wait_idle(unsigned timeout_ms);
private:
    enum class Kind { Start, Data, Event, Stop };
    struct Work { Kind kind; RecordingOptions options; std::string bytes; int64_t unix_ms = 0; bool tx = false; };
    void run();
    mutable std::mutex mutex_;
    std::condition_variable wake_, idle_;
    std::deque<Work> queue_;
    size_t queued_bytes_ = 0;
    RecordingStatus status_;
    bool shutdown_ = false;
    std::thread worker_;
};
