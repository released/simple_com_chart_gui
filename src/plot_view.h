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

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void paint();
    void draw_plot(HDC hdc, const RECT& client);
    void draw_hover(HDC hdc, const RECT& plot_rect);
    void draw_end_labels(Gdiplus::Graphics& graphics, const RECT& plot_rect);

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

    bool hover_active_ = false;
    double hover_t_ = 0.0;
    std::vector<std::pair<std::string, double>> hover_values_;

    std::unordered_map<std::string, COLORREF> color_map_;
    std::vector<std::string> color_order_;

    std::unordered_map<std::string, std::vector<ChannelSample>> frozen_series_;
    std::vector<std::string> frozen_keys_;
};
