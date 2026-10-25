#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

#include "channel_model.h"

namespace Gdiplus { class Graphics; }

class PlotView {
public:
    static constexpr UINT kCursorMoved = WM_APP + 9;
    struct EndLabel {
        std::string key;
        std::wstring text;
        double value;
        RECT bounds;
        POINT point;
    };
    bool create(HWND parent, int x, int y, int w, int h, int id);
    HWND hwnd() const;

    void set_model(ChannelModel* model);
    void set_compact(bool compact) { compact_ = compact; }
    void set_time_window(double sec);
    void update_from_model(double now);
    void reset_visual();

    void fit_enabled_channels();
    std::pair<double, double> y_range() const { return {y_min_, y_max_}; }

    void set_overlay_enabled(bool enabled);
    void set_show_names(bool enabled);
    const std::vector<EndLabel>& end_labels() const { return end_labels_; }
    void set_frozen(bool frozen);
    void set_analysis(double a, double b, bool reference, double threshold);
    void clear_analysis();
    int analysis_lines() const { return analysis_lines_; }
    void set_absolute_time_axis(bool enabled) { absolute_time_axis_=enabled; }
    void return_to_latest();
    void navigate_to(double end);
    bool set_fixed_y(bool enabled,double low=0,double high=50);
    bool following_latest() const {return follow_latest_;}
    double view_end() const {return last_now_;}
    double view_window() const {return time_window_;}

private:
    bool handle_navigation(UINT,WPARAM,LPARAM,LRESULT&);
    void notify_cursor(bool second,int x);
    bool follow_latest_=true,fixed_y_=false;
    int dragging_=0,drag_x_=0;
    double latest_now_=0,drag_end_=0;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void paint();
    void draw_plot(HDC hdc, const RECT& client);
    void draw_hover(HDC hdc, const RECT& plot_rect);
    void draw_end_labels(Gdiplus::Graphics& graphics, const RECT& plot_rect);
    void draw_analysis(Gdiplus::Graphics& graphics, const RECT& plot_rect);

    RECT plot_rect_from_client(const RECT& client) const;
    void ensure_color(const std::string& key);
    COLORREF get_color(const std::string& key) const;

    void update_y_range(double data_min, double data_max);

    void capture_snapshot();
    std::vector<std::string> get_active_keys() const;

    double data_to_x(const RECT& plot_rect, double x) const;
    double data_to_y(const RECT& plot_rect, double y) const;
    double x_to_data(const RECT& plot_rect, int x) const;

    HWND hwnd_ = nullptr;
    ChannelModel* model_ = nullptr;

    double time_window_ = 5.0;
    double y_min_ = 0.0;
    double y_max_ = 50.0;

    bool overlay_enabled_ = true;
    bool show_names_ = true;
    std::vector<EndLabel> end_labels_;
    bool compact_ = false;
    bool frozen_ = false;
    double last_now_ = 0.0;
    bool analysis_enabled_ = false, reference_enabled_ = false;
    double cursor_a_ = 0, cursor_b_ = 0, reference_y_ = 0;
    int analysis_lines_ = 0;
    bool absolute_time_axis_ = false;

    bool hover_active_ = false;
    double hover_t_ = 0.0;
    std::vector<std::pair<std::string, double>> hover_values_;

    std::unordered_map<std::string, COLORREF> color_map_;
    std::vector<std::string> color_order_;

    std::unordered_map<std::string, std::vector<ChannelSample>> frozen_series_;
    std::vector<std::string> frozen_keys_;
};
