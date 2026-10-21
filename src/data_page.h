#pragma once
#include <windows.h>
#include <array>
#include <unordered_set>
#include "capture_store.h"
#include "plot_view.h"
#include "channel_panel.h"

class DataPage {
public:
    bool create(HWND parent, CaptureStore* capture, HFONT font);
    HWND hwnd() const { return hwnd_; }
    HWND table() const { return table_; }
    void reset();
    void append(const std::unordered_map<std::string, double>& values, double timestamp);
    void refresh(double timestamp);
    void set_follow(bool enabled);
    bool follows() const;
    void select_field(size_t chart, const std::string& key);
    const std::string& selected_field(size_t chart) const { return selected_[chart]; }
    double chart_time(size_t chart) const { return seconds_[chart]; }
    const ChannelModel& chart_model(size_t chart) const { return models_[chart]; }
    const PlotView& chart_plot(size_t chart) const { return plots_[chart]; }
    void set_show_names(bool enabled);
    size_t cached_field_count() const { return latest_values_.size(); }
private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT, WPARAM, LPARAM);
    void layout(int w, int h);
    void sync_columns();
    void rebuild_columns();
    void fit_columns();
    void rebuild_chart(size_t chart);
    HWND hwnd_ = nullptr, table_ = nullptr, follow_ = nullptr, info_ = nullptr;
    HFONT font_ = nullptr;
    CaptureStore* capture_ = nullptr;
    ChannelPanel field_panel_;
    std::vector<size_t> visible_columns_;
    std::unordered_map<std::string, double> latest_values_;
    std::unordered_set<std::string> accepted_fields_;
    bool custom_fields_ = false;
    std::array<HWND, 4> selectors_{}, time_selectors_{};
    std::array<PlotView, 4> plots_;
    std::array<ChannelModel, 4> models_;
    std::array<std::string, 4> selected_;
    std::array<bool, 4> assigned_{};
    std::array<double, 4> seconds_{{30, 30, 30, 30}};
    size_t known_fields_ = 0;
    uint64_t first_sequence_ = 0;
    double now_ = 0;
};
