#include "capture_store.h"

#include <algorithm>
#include <cmath>

CaptureStore::CaptureStore(size_t max_rows, size_t max_bytes)
    : max_rows_(std::max<size_t>(1, max_rows)), max_bytes_(std::max<size_t>(1, max_bytes)) {}

void CaptureStore::reset(size_t max_channels) {
    max_channels_ = std::clamp<size_t>(max_channels, 1, 1024);
    keys_.clear(); columns_.clear(); rows_.clear();
    bytes_ = 0; next_ = 1; dropped_ = 0; start_ = 0;
}

void CaptureStore::append(const std::string& raw,
    const std::unordered_map<std::string, double>& values, double timestamp,uint64_t source_offset,uint64_t source_count) {
    if (!std::isfinite(timestamp)) return;
    if (next_ == 1) start_ = timestamp;
    auto row = std::make_shared<CaptureRow>();
    row->sequence = next_++;
    row->source_offset=source_offset;row->source_seconds=timestamp;
    row->source_count=source_count?source_count:raw.size();
    row->seconds = std::max(0.0, timestamp - start_);
    row->raw = raw;
    std::vector<std::string> new_keys;
    for (const auto& item : values) {
        if (std::isfinite(item.second) && !columns_.count(item.first)) new_keys.push_back(item.first);
    }
    // The parser uses a hash map; first-seen batches use a deterministic order.
    std::sort(new_keys.begin(), new_keys.end());
    for (const auto& key : new_keys) {
        if (keys_.size() >= max_channels_) break;
        columns_[key] = keys_.size(); keys_.push_back(key);
    }
    for (const auto& item : values) {
        const auto column = columns_.find(item.first);
        if (column != columns_.end() && std::isfinite(item.second))
            row->cells.emplace_back(column->second, item.second);
    }
    std::sort(row->cells.begin(), row->cells.end());
    const auto size = sizeof(CaptureRow) + row->raw.size() + row->cells.size() * sizeof(row->cells[0]);
    rows_.push_back(row); bytes_ += size;
    while (rows_.size() > max_rows_ || bytes_ > max_bytes_) {
        const auto& old = *rows_.front();
        bytes_ -= sizeof(CaptureRow) + old.raw.size() + old.cells.size() * sizeof(old.cells[0]);
        rows_.pop_front(); ++dropped_;
    }
}

CaptureSnapshot CaptureStore::snapshot() const {
    return {keys_, {rows_.begin(), rows_.end()}, dropped_};
}

bool CaptureStore::value(const CaptureRow& row, size_t column, double& result) {
    const auto it = std::lower_bound(row.cells.begin(), row.cells.end(), column,
        [](const auto& cell, size_t index) { return cell.first < index; });
    if (it == row.cells.end() || it->first != column) return false;
    result = it->second; return true;
}
