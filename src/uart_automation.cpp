#include "uart_automation.h"
#include <algorithm>

bool automation_payload(TxEntry entry, TxChecksum checksum, std::string& bytes, std::wstring& error) {
    if (static_cast<int>(entry.ending)<0 || static_cast<int>(entry.ending)>3) { error=L"Invalid ending selection"; return false; }
    const auto ending = entry.ending; entry.ending = TxEnding::None;
    if (!make_tx_payload(entry,bytes,error)) return false;
    if (checksum == TxChecksum::Sum8 || checksum == TxChecksum::Xor8) {
        unsigned char sum = 0; for (const unsigned char byte : bytes) sum = checksum == TxChecksum::Sum8 ? static_cast<unsigned char>(sum+byte) : sum^byte;
        bytes += static_cast<char>(sum);
    } else if (checksum == TxChecksum::ModbusCrc16) {
        uint16_t crc = 0xffff;
        for (const unsigned char byte : bytes) { crc ^= byte; for (int bit=0; bit<8; ++bit) crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1)^0xa001) : crc >> 1; }
        bytes += static_cast<char>(crc & 255); bytes += static_cast<char>(crc >> 8);
    } else if (checksum != TxChecksum::None) { error = L"Invalid checksum selection"; return false; }
    if (ending == TxEnding::CR || ending == TxEnding::CRLF) bytes += '\r';
    if (ending == TxEnding::LF || ending == TxEnding::CRLF) bytes += '\n';
    if (bytes.size() > 4096) { error = L"Payload plus checksum/ending exceeds 4096 bytes"; return false; } return true;
}
bool validate_automation(const AutomationConfig& config, std::wstring& error) {
    if (static_cast<int>(config.mode) < 0 || static_cast<int>(config.mode) > 2 || config.commands.empty() || config.commands.size() > 64 ||
        (config.mode != AutomationConfig::Mode::Batch && config.commands.size() != 1) ||
        config.interval_ms < 100 || config.interval_ms > 60000 || !config.count || config.count > 10000 || config.timeout_ms < 100 || config.timeout_ms > 60000 ||
        config.retries > 3 || config.wait_reply.size() > 256 || config.trigger.size() > 256 ||
        (config.mode == AutomationConfig::Mode::AutoReply && config.trigger.empty())) { error = L"Invalid automation limits/mode/pattern; no commands sent"; return false; }
    for (const auto& command : config.commands) if (command.empty() || command.size() > 4096) { error = L"Each command must contain 1..4096 bytes"; return false; }
    error.clear(); return true;
}
bool UartAutomation::start(AutomationConfig config, uint64_t now, std::wstring& error) {
    if (!validate_automation(config,error)) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_) { error = L"Stop the current automation first"; return false; }
    config_ = std::move(config); active_ = true; pending_ = waiting_ = matched_ = triggered_ = false;
    command_ = 0; cycles_ = retries_ = 0; due_ = started_ = now; reply_tail_.clear(); trigger_tail_.clear(); status_ = L"Armed; explicit start, no reconnect resume"; return true;
}
void UartAutomation::stop_locked(const std::wstring& reason) { active_ = pending_ = waiting_ = false; reply_tail_.clear(); trigger_tail_.clear(); status_ = reason; }
void UartAutomation::stop(const std::wstring& reason) { std::lock_guard<std::mutex> lock(mutex_); stop_locked(reason); }
bool UartAutomation::active() const { std::lock_guard<std::mutex> lock(mutex_); return active_; }
std::wstring UartAutomation::status() const { std::lock_guard<std::mutex> lock(mutex_); return status_; }
void UartAutomation::receive(std::string_view bytes) {
    std::lock_guard<std::mutex> lock(mutex_); if (!active_) return;
    const auto match = [&](std::string& tail, const std::string& pattern) {
        if (pattern.empty()) return false;
        // Small slices bound temporary memory while preserving matches crossing reads.
        bool found = false;
        for (size_t offset=0; offset<bytes.size(); offset+=1024) {
            tail.append(bytes.substr(offset,1024)); found = found || tail.find(pattern) != std::string::npos;
            if (tail.size() >= pattern.size()) tail.erase(0,tail.size()-pattern.size()+1);
        }
        return found;
    };
    if ((pending_ || waiting_) && !config_.wait_reply.empty()) matched_ = match(reply_tail_,config_.wait_reply) || matched_;
    if (!pending_ && !waiting_ && config_.mode == AutomationConfig::Mode::AutoReply) triggered_ = match(trigger_tail_,config_.trigger) || triggered_;
}
void UartAutomation::advance(uint64_t now) {
    waiting_ = pending_ = false; retries_ = 0; reply_tail_.clear();
    if (++command_ == config_.commands.size()) { command_ = 0; if (++cycles_ == config_.count) { stop_locked(L"Completed configured count"); return; } }
    due_ = now + config_.interval_ms; status_ = L"Running; completed cycles " + std::to_wstring(cycles_);
}
bool UartAutomation::next(uint64_t now, std::string& bytes) {
    std::lock_guard<std::mutex> lock(mutex_); if (!active_) return false;
    if (now-started_ >= 3600000) { stop_locked(L"Stopped at one-hour safety limit"); return false; }
    if (waiting_) {
        if (matched_) advance(now);
        else if (now >= deadline_) {
            if (retries_ < config_.retries) { ++retries_; waiting_ = false; due_ = now; }
            else { stop_locked(L"Stopped: reply timeout; retry limit reached"); return false; }
        }
    }
    if (!active_ || pending_ || waiting_ || now < due_) return false;
    if (config_.mode == AutomationConfig::Mode::AutoReply && !triggered_ && !retries_) return false;
    bytes = config_.commands[command_]; pending_ = true; matched_ = triggered_ = false; reply_tail_.clear(); trigger_tail_.clear();
    status_ = L"Waiting for driver completion; no overlapping automation TX"; return true;
}
void UartAutomation::complete(bool full_success, uint64_t now) {
    std::lock_guard<std::mutex> lock(mutex_); if (!active_ || !pending_) return;
    pending_ = false;
    if (!full_success) { stop_locked(L"Stopped: TX failed/partial/cancelled; outcome uncertain, not retried"); return; }
    if (!config_.wait_reply.empty() && !matched_) { waiting_ = true; deadline_ = now + config_.timeout_ms; status_ = L"Waiting for literal RX reply"; }
    else advance(now);
}
