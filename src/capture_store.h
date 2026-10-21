#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct CaptureRow {
    uint64_t sequence = 0;
    double seconds = 0;
    std::string raw;
    std::vector<std::pair<size_t, double>> cells;
};

struct CaptureSnapshot {
    std::vector<std::string> keys;
    std::vector<std::shared_ptr<const CaptureRow>> rows;
    uint64_t dropped = 0;
};

// Independent of the rolling chart window and the first page's checkboxes.
class CaptureStore {
public:
    explicit CaptureStore(size_t max_rows = 10000, size_t max_bytes = 32 * 1024 * 1024);
    void reset(size_t max_channels = 256);
    void append(const std::string& raw, const std::unordered_map<std::string, double>& values, double timestamp);
    CaptureSnapshot snapshot() const;
    const std::vector<std::string>& keys() const { return keys_; }
    const std::deque<std::shared_ptr<const CaptureRow>>& rows() const { return rows_; }
    uint64_t dropped() const { return dropped_; }
    static bool value(const CaptureRow& row, size_t column, double& result);

private:
    size_t max_rows_, max_bytes_, max_channels_ = 256, bytes_ = 0;
    uint64_t next_ = 1, dropped_ = 0;
    double start_ = 0;
    std::vector<std::string> keys_;
    std::unordered_map<std::string, size_t> columns_;
    std::deque<std::shared_ptr<const CaptureRow>> rows_;
};
