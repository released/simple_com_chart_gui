#pragma once
#include <string>
#include <string_view>
#include <deque>
#include <vector>
#include <mutex>
#include <cstdint>
#include "tx_pacing.h"
#include <map>
#include <memory>
#include <atomic>
#include <functional>

enum class SerialFlow { None, RtsCts, XonXoff, DsrDtr };
struct SerialControls {
    SerialFlow flow = SerialFlow::None;
    bool dtr = true, rts = true;
};
bool operator==(const SerialControls& a, const SerialControls& b);
bool valid_serial_controls(const SerialControls& options);
enum class TxEnding { None, CR, LF, CRLF };
struct TxEntry { std::wstring text; bool hex = false; TxEnding ending = TxEnding::None; };
bool make_tx_payload(const TxEntry& entry, std::string& bytes, std::wstring& error);
std::wstring tx_preview(std::string_view bytes, size_t maximum = 64);

struct SerialLineStatus {
    uint32_t errors = 0, modem = 0;
    bool modem_valid = false;
};
struct SerialOperationResult;
struct SerialOperation {
    enum class Kind { Write, Controls, Break };
    Kind kind = Kind::Write;
    uint64_t id = 0, generation = 0;
    uint64_t required_generation=0;
    std::wstring target;
    std::string bytes;
    SerialControls controls;
    TxPacing pacing;
    bool pacing_override=false;
    std::shared_ptr<std::atomic<bool>> cancel;
    std::function<void(const SerialOperationResult&)> completion;
};
struct SerialOperationResult {
    SerialOperation operation;
    size_t accepted = 0;
    bool success = false, cancelled = false;
    std::wstring error;
};
class SerialOperationQueue {
public:
    void begin();
    void cancel();
    bool cancel_pending(uint64_t id);
    bool submit(SerialOperation operation, uint64_t& id, std::wstring& error);
    bool take(SerialOperation& operation);
    bool current(uint64_t generation) const;
    uint64_t generation() const;
    void complete(SerialOperationResult result);
    std::vector<SerialOperationResult> results();
    size_t queued() const;
    bool busy()const;
    void pacing(TxPacing);
private:
    mutable std::mutex mutex_;
    std::deque<SerialOperation> pending_;
    std::vector<SerialOperationResult> completed_;
    size_t bytes_ = 0;
    uint64_t generation_ = 0, id_ = 0;
    bool active_ = false;
    TxPacing pacing_;
    std::map<uint64_t,std::shared_ptr<std::atomic<bool>>> running_;
};

struct UartStatistics {
    uint64_t rx_bytes = 0, tx_bytes = 0, tx_failures = 0;
    uint64_t frame = 0, parity = 0, overrun = 0, rx_overflow = 0, breaks = 0;
    uint64_t last_rx_ms = 0;
    double rx_per_second = 0, tx_per_second = 0;
    SerialLineStatus line;
};
class UartStatisticsStore {
public:
    void reset();
    void receive(size_t bytes, uint64_t now);
    void transmit(size_t bytes, bool failed, uint64_t now);
    void observe(SerialLineStatus line);
    UartStatistics snapshot(uint64_t now) const;
private:
    struct Rate { uint64_t ms, rx, tx; };
    void rate(uint64_t now, uint64_t rx, uint64_t tx);
    mutable std::mutex mutex_;
    UartStatistics value_;
    std::deque<Rate> rates_;
};
