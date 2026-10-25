#pragma once
#include "uart_operations.h"
#include <atomic>
enum class TxChecksum { None, Sum8, Xor8, ModbusCrc16 };
bool automation_payload(TxEntry entry, TxChecksum checksum, std::string& bytes, std::wstring& error);
struct AutomationConfig {
    enum class Mode { Periodic, Batch, AutoReply };
    Mode mode = Mode::Periodic;
    std::vector<std::string> commands;
    std::string wait_reply, trigger;
    unsigned interval_ms = 1000, count = 1, timeout_ms = 1000, retries = 0;
};
bool validate_automation(const AutomationConfig& config, std::wstring& error);
class UartAutomation {
public:
    bool start(AutomationConfig config, uint64_t now, std::wstring& error);
    void stop(const std::wstring& reason);
    void receive(std::string_view bytes);
    bool next(uint64_t now, std::string& bytes);
    void complete(bool full_success, uint64_t now);
    bool active() const;
    std::wstring status() const;
private:
    void advance(uint64_t now);
    void stop_locked(const std::wstring& reason);
    mutable std::mutex mutex_;
    AutomationConfig config_;
    bool active_ = false, pending_ = false, waiting_ = false, matched_ = false, triggered_ = false;
    size_t command_ = 0;
    unsigned cycles_ = 0, retries_ = 0;
    uint64_t due_ = 0, deadline_ = 0, started_ = 0;
    std::string reply_tail_, trigger_tail_;
    std::wstring status_ = L"Stopped; automation never starts automatically";
};
