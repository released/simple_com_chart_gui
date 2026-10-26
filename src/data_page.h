#pragma once
#include <windows.h>
#include <array>
#include <unordered_set>
#include "capture_store.h"
#include "plot_view.h"
#include "channel_panel.h"

class DataPage {
public:
    void set_ui_font(HFONT font);
    void set_display_preferences(const DisplayPreferences* value);
    static constexpr UINT kSelectionChanged = WM_APP + 3;
    static constexpr UINT kNavigate=WM_APP+10;
    bool select_source_time(double source_seconds);
    bool select_sequence(uint64_t sequence);
    bool create(HWND parent, CaptureStore* capture, HFONT font);
    HWND hwnd() const { return hwnd_; }
    HWND table() const { return table_; }
    void reset();
    void append(const std::unordered_map<std::string, double>& values, double timestamp);
    void refresh(double timestamp);
    void set_follow(bool enabled);
    bool follows() const;
    void select_field(size_t chart, const std::string& key);
    void set_selected_fields(const std::array<std::string, 4>& fields);
    const std::string& selected_field(size_t chart) const { return selected_[chart]; }
    double chart_time(size_t chart) const { return seconds_[chart]; }
    const ChannelModel& chart_model(size_t chart) const { return models_[chart]; }
    const PlotView& chart_plot(size_t chart) const { return plots_[chart]; }
    PlotView& chart_plot(size_t chart) { return plots_.at(chart); }
    void set_chart_time(size_t,double);
    std::unordered_map<std::string,bool> column_checks()const{return field_panel_.get_checkbox_state_map();}
    void set_column_checks(const std::unordered_map<std::string,bool>& values){sync_columns();field_panel_.set_checkbox_state_map(values);rebuild_columns();refresh(-1);}
    std::shared_ptr<const CaptureRow> selected_row()const;
    void set_show_names(bool enabled);
    size_t cached_field_count() const { return latest_values_.size(); }
private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT, WPARAM, LPARAM);
    void layout(int w, int h);
    void sync_columns();
    void rebuild_columns();
    void fit_columns();
    void ensure_selector_key(const std::string& key);
    void rebuild_chart(size_t chart);
    HWND hwnd_ = nullptr, table_ = nullptr, follow_ = nullptr, info_ = nullptr, reset_ = nullptr;
    HFONT font_ = nullptr;
    const DisplayPreferences* display_=nullptr;
    CaptureStore* capture_ = nullptr;
    ChannelPanel field_panel_;
    std::vector<size_t> visible_columns_;
    std::vector<int> column_widths_;
    std::vector<std::string> selector_keys_;
    std::unordered_map<std::string, double> latest_values_;
    std::unordered_set<std::string> accepted_fields_;
    int header_height_ = 0;
    std::array<HWND, 4> selectors_{}, time_selectors_{};
    std::array<PlotView, 4> plots_;
    std::array<ChannelModel, 4> models_;
    std::array<std::string, 4> selected_;
    std::array<double, 4> seconds_{{30, 30, 30, 30}};
    size_t known_fields_ = 0;
    uint64_t first_sequence_ = 0;
    double now_ = 0;
};
