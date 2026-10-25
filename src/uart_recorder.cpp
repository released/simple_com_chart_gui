#include "uart_recorder.h"
#include "message_format.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <cstring>

namespace {
class RecordingFile {
public:
    ~RecordingFile() { close(); }
    void open(const std::filesystem::path& path) {
        close(); path_ = path;
        handle_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create " + message_utf8(path.wstring()) + " (existing name, permissions or disk error), Windows error " + std::to_string(GetLastError()));
    }
    void write(std::string_view bytes) {
        if (bytes.empty()) return;
        DWORD written = 0;
        const bool ok = WriteFile(handle_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) != FALSE;
        if (!ok || written != bytes.size())
            throw std::runtime_error("Recording write failed: " + message_utf8(path_.wstring()) + ", Windows error " + std::to_string(ok ? ERROR_WRITE_FAULT : GetLastError()));
    }
    void flush() {
        if (handle_ != INVALID_HANDLE_VALUE && !FlushFileBuffers(handle_))
            throw std::runtime_error("Recording flush failed: " + message_utf8(path_.wstring()) + ", Windows error " + std::to_string(GetLastError()));
    }
    void close() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); handle_ = INVALID_HANDLE_VALUE; }
private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::filesystem::path path_;
};
std::wstring failure_text(const std::exception& ex) {
    const int length = static_cast<int>(strlen(ex.what()));
    const int n = MultiByteToWideChar(CP_UTF8, 0, ex.what(), length, nullptr, 0);
    std::wstring value(n, 0); MultiByteToWideChar(CP_UTF8, 0, ex.what(), length, value.data(), n);
    return L"Recording ERROR: " + value;
}
int64_t day_at(int64_t ms) {
    ULARGE_INTEGER ticks{}; ticks.QuadPart = static_cast<uint64_t>(ms + 11644473600000LL) * 10000;
    FILETIME ft{ticks.LowPart, ticks.HighPart}, local{}; SYSTEMTIME st{};
    FileTimeToLocalFileTime(&ft, &local); FileTimeToSystemTime(&local, &st);
    return st.wYear * 10000 + st.wMonth * 100 + st.wDay;
}
}

UartRecorder::UartRecorder() : worker_([this] { run(); }) {}
UartRecorder::~UartRecorder() {
    { std::lock_guard<std::mutex> lock(mutex_); shutdown_ = true; }
    wake_.notify_one(); if (worker_.joinable()) worker_.join();
}
bool UartRecorder::start(RecordingOptions options) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_.active || shutdown_ || options.base.empty()) return false;
    options.rotate_bytes = std::clamp<uint64_t>(options.rotate_bytes, 1024, 1024ULL * 1024 * 1024);
    status_ = {}; status_.active = true; status_.detail = L"Starting recording...";
    queue_.push_back({Kind::Start, std::move(options), {}, 0, false}); wake_.notify_one(); return true;
}
void UartRecorder::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping) return;
    status_.stopping = true; queue_.push_back({Kind::Stop, {}, {}, 0, false}); wake_.notify_one();
}
void UartRecorder::append(std::string_view bytes, int64_t ms, bool tx) {
    if (bytes.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping) return;
    if (queue_.size() >= 4096 || bytes.size() > 8 * 1024 * 1024 - queued_bytes_) {
        // Stop visibly at the first gap. Never continue an apparently complete raw recording.
        status_.lost += bytes.size(); status_.stopping = true;
        queue_.push_back({Kind::Event, {}, "RECORDER INPUT GAP: " + std::to_string(bytes.size()) + " bytes not recorded; recording stopped", ms, false});
        queue_.push_back({Kind::Stop, {}, {}, 0, false}); wake_.notify_one(); return;
    }
    queue_.push_back({Kind::Data, {}, std::string(bytes), ms, tx}); queued_bytes_ += bytes.size(); wake_.notify_one();
}
void UartRecorder::event(std::string text, int64_t ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping || queue_.size() > 4096) return;
    if (text.size() > 1024) text.resize(1024);
    queue_.push_back({Kind::Event, {}, std::move(text), ms, false}); wake_.notify_one();
}
RecordingStatus UartRecorder::status() const { std::lock_guard<std::mutex> lock(mutex_); return status_; }
bool UartRecorder::wait_idle(unsigned ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    return idle_.wait_for(lock, std::chrono::milliseconds(ms), [this] { return !status_.active; });
}

void UartRecorder::run() {
    RecordingFile raw, text, index;
    RecordingOptions options; bool opened = false, first = true, after_cr = false;
    uint64_t segment = 0, segment_bytes = 0; int64_t day = 0;
    auto last_flush = std::chrono::steady_clock::now();
    const auto close = [&] { raw.close(); text.close(); index.close(); opened = false; };
    const auto flush = [&] { if (opened) { raw.flush(); text.flush(); index.flush(); } last_flush = std::chrono::steady_clock::now(); };
    const auto open_segment = [&](int64_t ms) {
        if (opened) flush(); close(); ++segment; segment_bytes = 0; day = day_at(ms); first = true; after_cr = false;
        const auto base = options.base.wstring() + L"_" + std::to_wstring(segment);
        // CREATE_NEW never overwrites a user's previous capture. Preserve partial evidence on failure.
        raw.open(base + L".bin"); text.open(base + L".log"); index.open(base + L".idx"); opened = true;
        index.write("SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n");
        std::lock_guard<std::mutex> lock(mutex_); status_.segments = segment; status_.detail = L"Recording: " + base;
    };
    for (;;) {
        Work work;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!wake_.wait_for(lock, std::chrono::milliseconds(250), [this] { return shutdown_ || !queue_.empty(); })) {
                lock.unlock();
                try { flush(); } catch (const std::exception& ex) {
                    close(); std::lock_guard<std::mutex> failed(mutex_); status_.active = status_.stopping = false;
                    status_.detail = failure_text(ex);
                    queue_.clear(); queued_bytes_ = 0; idle_.notify_all();
                }
                continue;
            }
            if (queue_.empty() && shutdown_) { lock.unlock(); try { flush(); } catch (...) {} break; }
            work = std::move(queue_.front()); queue_.pop_front();
            if (work.kind == Kind::Data) queued_bytes_ -= work.bytes.size();
        }
        try {
            if (work.kind == Kind::Start) { options = std::move(work.options); segment = 0; open_segment(message_unix_ms()); }
            else if (work.kind == Kind::Stop) {
                flush(); close(); std::lock_guard<std::mutex> lock(mutex_);
                status_.active = status_.stopping = false;
                status_.detail = status_.lost ? L"Recording STOPPED: input gap; see .idx" : L"Recording saved (stopped)";
                idle_.notify_all();
            } else if (opened && work.kind == Kind::Event) {
                for (auto& c : work.bytes) if (c == '\r' || c == '\n' || c == ',') c = ' ';
                index.write(std::to_string(segment_bytes) + ",0," + std::to_string(work.unix_ms) + "," + work.bytes + "\r\n");
            } else if (opened && work.kind == Kind::Data) {
                if (!work.tx && !segment_bytes) day = day_at(work.unix_ms);
                if (!work.tx && segment_bytes && ((options.rotate_daily && day_at(work.unix_ms) != day) || segment_bytes + work.bytes.size() > options.rotate_bytes)) open_segment(work.unix_ms);
                if (!work.tx) {
                    raw.write(work.bytes);
                    index.write(std::to_string(segment_bytes) + "," + std::to_string(work.bytes.size()) + "," + std::to_string(work.unix_ms) + ",RX\r\n");
                    segment_bytes += work.bytes.size();
                }
                MessageDisplayOptions display; display.clock = MessageClock::Local;
                if (work.tx) { text.write("\r\n[TX] " + message_utf8(format_message(work.bytes, 0, {{0,work.unix_ms}}, display).text) + "\r\n"); first = true; after_cr = false; }
                else {
                    std::string output;
                    for (const auto c : work.bytes) {
                        if (c == '\n' && after_cr) { after_cr = false; continue; }
                        after_cr = c == '\r';
                        if (first) { output += message_utf8(format_message(" ", 0, {{0,work.unix_ms}}, display).text); output.pop_back(); output += "[RX] "; first = false; }
                        if (c == '\r' || c == '\n') { output += "\r\n"; first = true; }
                        else if (c == '\t' || (static_cast<unsigned char>(c) >= 32 && static_cast<unsigned char>(c) < 127)) output += c;
                        else { static const char hex[] = "0123456789ABCDEF"; const auto b = static_cast<unsigned char>(c); output += "\\x"; output += hex[b >> 4]; output += hex[b & 15]; }
                    }
                    text.write(output);
                }
                { std::lock_guard<std::mutex> lock(mutex_); if (work.tx) status_.tx_bytes += work.bytes.size(); else status_.bytes += work.bytes.size(); }
                if (std::chrono::steady_clock::now() - last_flush >= std::chrono::milliseconds(250)) flush();
            }
        } catch (const std::exception& ex) {
            close(); std::lock_guard<std::mutex> lock(mutex_);
            status_.active = status_.stopping = false;
            status_.detail = failure_text(ex);
            queue_.clear(); queued_bytes_ = 0; idle_.notify_all();
        }
    }
}
