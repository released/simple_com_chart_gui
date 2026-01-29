#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

struct ChannelSample {
    double t = 0.0;
    int v = 0;
};

class ChannelModel {
public:
    void reset_samples();
    void reset();

    void set_time_window(double sec);
    double get_time_window() const;

    bool ensure_channel(const std::string& key, double timestamp);
    int consume_dropped_keys();

    void set_enabled(const std::string& key, bool enabled);
    bool is_enabled(const std::string& key) const;

    std::vector<std::string> get_keys() const;

    int get_total_samples() const;
    int get_enabled_count() const;

    void update_from_kv(const std::unordered_map<std::string, int>& kv, double timestamp);
    void prune(double now);

    std::vector<std::string> get_enabled_keys_with_data() const;
    std::vector<ChannelSample> get_series(const std::string& key) const;

private:
    static constexpr int kMaxChannels = 16;

    double time_window_sec_ = 5.0;

    std::unordered_map<std::string, std::deque<ChannelSample>> channels_;
    std::unordered_map<std::string, bool> enabled_;
    std::vector<std::string> key_order_;
    std::unordered_map<std::string, double> first_seen_ts_;
    std::unordered_map<std::string, double> last_ts_;

    int total_samples_ = 0;
    int rx_lines_ = 0;
    int dropped_keys_ = 0;

    double ts_eps_ = 0.0005;
};
