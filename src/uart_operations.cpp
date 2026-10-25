#include "uart_operations.h"
#include <windows.h>
#include <algorithm>
#include <cwctype>

bool operator==(const SerialControls& a, const SerialControls& b) { return a.flow == b.flow && a.dtr == b.dtr && a.rts == b.rts; }
bool valid_serial_controls(const SerialControls& value) { return static_cast<unsigned>(value.flow) <= static_cast<unsigned>(SerialFlow::DsrDtr); }

bool make_tx_payload(const TxEntry& entry, std::string& bytes, std::wstring& error) {
    error.clear(); bytes.clear();
    if (entry.text.size() > 16384 || static_cast<unsigned>(entry.ending) > 3) { error = L"TX input is too long or the line ending is invalid."; return false; }
    if (entry.hex) {
        int high = -1;
        for (const auto c : entry.text) {
            if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n') continue;
            const int n = c >= L'0' && c <= L'9' ? c - L'0' : c >= L'a' && c <= L'f' ? c - L'a' + 10 : c >= L'A' && c <= L'F' ? c - L'A' + 10 : -1;
            if (n < 0) { error = L"HEX accepts byte pairs such as 41 42 or 4142; no 0x prefix."; bytes.clear(); return false; }
            if (high < 0) high = n; else { bytes += static_cast<char>((high << 4) | n); high = -1; }
        }
        if (high >= 0) { error = L"HEX needs an even number of digits."; bytes.clear(); return false; }
    } else if (!entry.text.empty()) {
        const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, entry.text.data(), static_cast<int>(entry.text.size()), nullptr, 0, nullptr, nullptr);
        if (!n) { error = L"Text contains an invalid Unicode sequence."; return false; }
        bytes.resize(n); WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, entry.text.data(), static_cast<int>(entry.text.size()), bytes.data(), n, nullptr, nullptr);
    }
    if (entry.ending == TxEnding::CR || entry.ending == TxEnding::CRLF) bytes += '\r';
    if (entry.ending == TxEnding::LF || entry.ending == TxEnding::CRLF) bytes += '\n';
    if (bytes.empty() || bytes.size() > 4096) { error = L"TX must contain 1..4096 bytes including the selected line ending."; bytes.clear(); return false; }
    return true;
}

std::wstring tx_preview(std::string_view bytes, size_t maximum) {
    static const wchar_t hex[] = L"0123456789ABCDEF"; std::wstring result;
    for (size_t i = 0; i < std::min(bytes.size(), maximum); ++i) {
        if (i) result += L' '; const auto b = static_cast<unsigned char>(bytes[i]); result += hex[b >> 4]; result += hex[b & 15];
    }
    if (bytes.size() > maximum) result += L" ...";
    return result;
}

void SerialOperationQueue::begin() { cancel(); std::lock_guard<std::mutex> lock(mutex_); active_ = true; }
void SerialOperationQueue::cancel() {
    std::lock_guard<std::mutex> lock(mutex_); active_ = false; ++generation_;
    for(auto& item:running_)*item.second=true;
    for (auto& operation : pending_) {
        if (completed_.size() < 64) completed_.push_back({std::move(operation), 0, false, true, L"Cancelled before execution; not replayed"});
    }
    pending_.clear(); bytes_ = 0;
}
bool SerialOperationQueue::submit(SerialOperation operation, uint64_t& id, std::wstring& error) {
    std::lock_guard<std::mutex> lock(mutex_); error.clear();
    if (!active_) { error = L"Connect first; no operation is queued while disconnected or stopping."; return false; }
    if(operation.required_generation&&operation.required_generation!=generation_){error=L"Connection generation changed; nothing queued.";return false;}
    if (operation.bytes.size() > 4096 || pending_.size() >= 16 || bytes_ + operation.bytes.size() > 65536 || completed_.size() >= 48) { error = L"UART operation queue is full; wait for completion."; return false; }
    if(!operation.pacing_override)operation.pacing=pacing_;
    if(!valid_tx_pacing(operation.pacing)){error=L"Invalid TX pacing; operation not queued.";return false;}
    if(operation.pacing.byte_ms||operation.pacing.line_ms){uint64_t delay=static_cast<uint64_t>(operation.bytes.size())*operation.pacing.byte_ms;for(size_t n=0;n<operation.bytes.size();++n)if(operation.bytes[n]=='\n'||(operation.bytes[n]=='\r'&&(n+1==operation.bytes.size()||operation.bytes[n+1]!='\n')))delay+=operation.pacing.line_ms>operation.pacing.byte_ms?operation.pacing.line_ms-operation.pacing.byte_ms:0;if(delay>3600000){error=L"TX pacing would exceed one hour for this payload; reduce the delays or payload.";return false;}}
    operation.generation = generation_; operation.id = ++id_; id = operation.id;
    operation.cancel=std::make_shared<std::atomic<bool>>(false);
    bytes_ += operation.bytes.size(); pending_.push_back(std::move(operation)); return true;
}
bool SerialOperationQueue::take(SerialOperation& operation) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ || pending_.empty()) return false;
    operation = std::move(pending_.front()); pending_.pop_front(); bytes_ -= operation.bytes.size();running_[operation.id]=operation.cancel;return true;
}
bool SerialOperationQueue::current(uint64_t generation) const { std::lock_guard<std::mutex> lock(mutex_); return active_ && generation == generation_; }
uint64_t SerialOperationQueue::generation() const { std::lock_guard<std::mutex> lock(mutex_); return generation_; }
void SerialOperationQueue::complete(SerialOperationResult result) {
    const auto notify=result.operation.completion;
    {
    std::lock_guard<std::mutex> lock(mutex_);
    running_.erase(result.operation.id);
    if (!active_ || result.operation.generation != generation_ || (result.operation.cancel&&result.operation.cancel->load())) {
        result.cancelled = true;
        result.error += L" Connection closed; an in-flight operation may already have taken effect. No replay.";
    }
    if (completed_.size() < 64) {auto visible=result;visible.operation.completion={};completed_.push_back(std::move(visible));}
    }
    if(notify)notify(result);
}
std::vector<SerialOperationResult> SerialOperationQueue::results() { std::lock_guard<std::mutex> lock(mutex_); auto result = std::move(completed_); completed_.clear(); return result; }
bool SerialOperationQueue::cancel_pending(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto current=running_.find(id);if(current!=running_.end()){*current->second=true;return true;}
    for (auto it=pending_.begin();it!=pending_.end();++it) if (it->id==id) {
        bytes_-=it->bytes.size(); SerialOperationResult result; result.operation=std::move(*it); result.cancelled=true;
        result.error=L"Cancelled before driver submission"; pending_.erase(it); if (completed_.size()<64) completed_.push_back(std::move(result)); return true;
    }
    return false;
}
size_t SerialOperationQueue::queued() const { std::lock_guard<std::mutex> lock(mutex_); return pending_.size(); }
bool SerialOperationQueue::busy()const{std::lock_guard<std::mutex> lock(mutex_);return !pending_.empty()||!running_.empty();}
void SerialOperationQueue::pacing(TxPacing value){std::lock_guard<std::mutex> lock(mutex_);pacing_=value;}

void UartStatisticsStore::reset() { std::lock_guard<std::mutex> lock(mutex_); value_ = {}; rates_.clear(); }
void UartStatisticsStore::rate(uint64_t now, uint64_t rx, uint64_t tx) {
    if (!rates_.empty() && rates_.back().ms / 20 == now / 20) { rates_.back().rx += rx; rates_.back().tx += tx; }
    else rates_.push_back({now, rx, tx});
    while (!rates_.empty() && (rates_.front().ms + 1000 <= now || rates_.size() > 128)) rates_.pop_front();
}
void UartStatisticsStore::receive(size_t bytes, uint64_t now) {
    if (!bytes) return; std::lock_guard<std::mutex> lock(mutex_); value_.rx_bytes += bytes; value_.last_rx_ms = now; rate(now, bytes, 0);
}
void UartStatisticsStore::transmit(size_t bytes, bool failed, uint64_t now) {
    std::lock_guard<std::mutex> lock(mutex_); value_.tx_bytes += bytes; if (failed) ++value_.tx_failures; rate(now, 0, bytes);
}
void UartStatisticsStore::observe(SerialLineStatus line) {
    std::lock_guard<std::mutex> lock(mutex_); value_.line = line;
    if (line.errors & CE_FRAME) ++value_.frame;
    if (line.errors & CE_RXPARITY) ++value_.parity;
    if (line.errors & CE_OVERRUN) ++value_.overrun;
    if (line.errors & CE_RXOVER) ++value_.rx_overflow;
    if (line.errors & CE_BREAK) ++value_.breaks;
}
UartStatistics UartStatisticsStore::snapshot(uint64_t now) const {
    std::lock_guard<std::mutex> lock(mutex_); auto result = value_;
    for (const auto& item : rates_) if (item.ms + 1000 > now && item.ms <= now) { result.rx_per_second += static_cast<double>(item.rx); result.tx_per_second += static_cast<double>(item.tx); }
    return result;
}
