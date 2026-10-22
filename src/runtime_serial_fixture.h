#pragma once
#include "serial_manager.h"
#include <atomic>
#include <condition_variable>
#include <memory>

// Test-only transport: no Windows COM open/read/write API is called.
struct FakeSerialState {
    std::mutex mutex;
    std::condition_variable wake;
    std::vector<SerialPortInfo> ports;
    std::vector<SerialSettings> attempts;
    bool scan_ok = true, hold_open = false, read_error = false;
    unsigned fail_opens = 0, active = 0, closed = 0;
    std::wstring open_error = L"Simulated port busy";
    std::string raw;
    size_t attempt_count() { std::lock_guard<std::mutex> lock(mutex); return attempts.size(); }
    void release() { { std::lock_guard<std::mutex> lock(mutex); hold_open = false; } wake.notify_all(); }
};

class FakeSerialTransport : public SerialTransport {
public:
    explicit FakeSerialTransport(std::shared_ptr<FakeSerialState> state) : state_(std::move(state)) {}
    ~FakeSerialTransport() override { disconnect(); }
    bool is_connected() const override { return connected_.load(); }
    bool connect(const std::wstring& path, int baud, int data, int parity, int stop, std::wstring* error) override {
        std::unique_lock<std::mutex> lock(state_->mutex);
        SerialSettings settings; settings.port.open_path = path; settings.baud = baud; settings.data_bits = data;
        settings.parity = parity; settings.stop_bits = stop; state_->attempts.push_back(settings); state_->wake.notify_all();
        state_->wake.wait(lock, [&] { return !state_->hold_open; });
        if (state_->fail_opens) { --state_->fail_opens; if (error) *error = state_->open_error; return false; }
        connected_ = true; ++state_->active; return true;
    }
    void disconnect() override {
        if (!connected_.exchange(false)) return;
        std::lock_guard<std::mutex> lock(state_->mutex); --state_->active; ++state_->closed;
    }
    std::vector<std::string> read_lines(std::wstring* error, std::string* raw = nullptr) override {
        if (raw) raw->clear();
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (!connected_) return {};
        if (state_->read_error) {
            state_->read_error = false; connected_ = false; --state_->active; ++state_->closed;
            if (error) *error = L"Simulated bridge removal"; return {};
        }
        if (raw) *raw = state_->raw;
        partial_ += state_->raw; state_->raw.clear();
        std::vector<std::string> lines; size_t end = 0;
        while ((end = partial_.find('\n')) != std::string::npos) {
            auto line = partial_.substr(0, end); partial_.erase(0, end + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(std::move(line));
        }
        return lines;
    }
    int consume_rx_overflow() override { return 0; }
private:
    std::shared_ptr<FakeSerialState> state_;
    std::atomic<bool> connected_{false};
    std::string partial_;
};
