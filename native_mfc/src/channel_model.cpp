#include "channel_model.h"

#include <algorithm>
#include <cmath>

void ChannelModel::reset_samples() {
    for (auto& kv : channels_) {
        kv.second.clear();
    }
    last_ts_.clear();
    total_samples_ = 0;
    rx_lines_ = 0;
    dropped_keys_ = 0;
}

void ChannelModel::reset() {
    channels_.clear();
    enabled_.clear();
    key_order_.clear();
    first_seen_ts_.clear();
    last_ts_.clear();
    total_samples_ = 0;
    rx_lines_ = 0;
    dropped_keys_ = 0;
}

void ChannelModel::set_time_window(double sec) {
    if (sec < 1.0) {
        sec = 1.0;
    }
    time_window_sec_ = sec;
}

double ChannelModel::get_time_window() const {
    return time_window_sec_;
}

bool ChannelModel::ensure_channel(const std::string& key, double timestamp) {
    if (channels_.find(key) != channels_.end()) {
        return true;
    }

    if (static_cast<int>(channels_.size()) >= kMaxChannels) {
        dropped_keys_ += 1;
        return false;
    }

    channels_[key] = std::deque<ChannelSample>();
    enabled_[key] = true;
    last_ts_[key] = 0.0;

    first_seen_ts_[key] = timestamp;
    key_order_.push_back(key);
    return true;
}

int ChannelModel::consume_dropped_keys() {
    int count = dropped_keys_;
    dropped_keys_ = 0;
    return count;
}

void ChannelModel::set_enabled(const std::string& key, bool enabled) {
    auto it = enabled_.find(key);
    if (it != enabled_.end()) {
        it->second = enabled;
    }
}

bool ChannelModel::is_enabled(const std::string& key) const {
    auto it = enabled_.find(key);
    if (it == enabled_.end()) {
        return true;
    }
    return it->second;
}

std::vector<std::string> ChannelModel::get_keys() const {
    return key_order_;
}

int ChannelModel::get_total_samples() const {
    return total_samples_;
}

int ChannelModel::get_enabled_count() const {
    int count = 0;
    for (const auto& kv : enabled_) {
        if (kv.second) {
            count++;
        }
    }
    return count;
}

void ChannelModel::update_from_kv(const std::unordered_map<std::string, int>& kv, double timestamp) {
    if (kv.empty()) {
        return;
    }

    for (const auto& pair : kv) {
        const std::string& key = pair.first;
        int value = pair.second;

        ensure_channel(key, timestamp);
        if (channels_.find(key) == channels_.end()) {
            continue;
        }

        if (value < 0) {
            continue;
        }

        double t = timestamp;
        auto it_last = last_ts_.find(key);
        double last = (it_last != last_ts_.end()) ? it_last->second : 0.0;
        if (t <= last) {
            t = last + ts_eps_;
        }
        last_ts_[key] = t;

        auto& buf = channels_[key];
        if (!buf.empty() && std::abs(t - buf.back().t) < ts_eps_) {
            buf.back().t = t;
            buf.back().v = value;
        } else {
            buf.push_back(ChannelSample{t, value});
            total_samples_ += 1;
        }
    }
}

void ChannelModel::prune(double now) {
    double cutoff = now - time_window_sec_;
    for (auto& kv : channels_) {
        auto& buf = kv.second;
        while (!buf.empty() && buf.front().t < cutoff) {
            buf.pop_front();
        }
    }
}

std::vector<std::string> ChannelModel::get_enabled_keys_with_data() const {
    std::vector<std::string> keys;
    for (const auto& key : key_order_) {
        auto it = channels_.find(key);
        if (it == channels_.end()) {
            continue;
        }
        if (!it->second.empty() && is_enabled(key)) {
            keys.push_back(key);
        }
    }
    return keys;
}

std::vector<ChannelSample> ChannelModel::get_series(const std::string& key) const {
    auto it = channels_.find(key);
    if (it == channels_.end()) {
        return {};
    }
    return std::vector<ChannelSample>(it->second.begin(), it->second.end());
}
