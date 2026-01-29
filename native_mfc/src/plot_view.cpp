#include "plot_view.h"

#include <gdiplus.h>
#include <windowsx.h>

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <sstream>

using namespace Gdiplus;

namespace {
constexpr int kMarginLeft = 120;
constexpr int kMarginRight = 20;
constexpr int kMarginTop = 20;
constexpr int kMarginBottom = 60;

constexpr int kGridCount = 5;

constexpr double kMinVisibleSpan = 50.0;
constexpr int kAutoExpandPadPx = 12;

constexpr int kEndTagGapPx = 10;
constexpr int kEndTagYThresholdPx = 14;
constexpr int kEndTagXMarginPx = 8;
constexpr int kEndTagYOffsetPx = 10;
constexpr int kEndTagSafeMarginPx = 8;

COLORREF kColorTable[] = {
    RGB(255,  99,  71),
    RGB( 30, 144, 255),
    RGB( 50, 205,  50),
    RGB(255,  20, 147),
    RGB(138,  43, 226),
    RGB(255, 140,   0),
    RGB(  0, 206, 209),
    RGB(220,  20,  60),
};

std::wstring to_wstring(const std::string& s) {
    if (s.empty()) {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}

std::wstring to_wstring(int value) {
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"%d", value);
    return buf;
}

std::wstring to_wstring(double value) {
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"%.3f", value);
    return buf;
}

std::wstring format_tick(double value) {
    double rounded = std::round(value);
    if (std::abs(value - rounded) < 0.001) {
        return to_wstring(static_cast<int>(rounded));
    }
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"%.2f", value);
    return buf;
}

std::wstring format_tick_int(double value) {
    return to_wstring(static_cast<int>(std::lround(value)));
}

double nice_number(double value, bool round) {
    if (value <= 0.0) {
        return 1.0;
    }
    double expv = std::floor(std::log10(value));
    double f = value / std::pow(10.0, expv);
    double nf = 1.0;
    if (round) {
        if (f < 1.5) {
            nf = 1.0;
        } else if (f < 3.0) {
            nf = 2.0;
        } else if (f < 7.0) {
            nf = 5.0;
        } else {
            nf = 10.0;
        }
    } else {
        if (f <= 1.0) {
            nf = 1.0;
        } else if (f <= 2.0) {
            nf = 2.0;
        } else if (f <= 5.0) {
            nf = 5.0;
        } else {
            nf = 10.0;
        }
    }
    return nf * std::pow(10.0, expv);
}

double auto_tick_step(double min_val, double max_val, int target_ticks) {
    double span = max_val - min_val;
    if (span <= 0.0) {
        return 1.0;
    }
    double range = nice_number(span, false);
    double step = nice_number(range / std::max(2, target_ticks - 1), true);
    if (step < 1.0) {
        step = 1.0;
    }
    return step;
}
} // namespace

bool PlotView::create(HWND parent, int x, int y, int w, int h, int id) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = PlotView::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"PlotViewWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"", WS_CHILD | WS_VISIBLE,
                            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance, this);
    return hwnd_ != nullptr;
}

HWND PlotView::hwnd() const {
    return hwnd_;
}

void PlotView::set_model(ChannelModel* model) {
    model_ = model;
}

void PlotView::set_time_window(double sec) {
    if (sec < 1.0) {
        sec = 1.0;
    }
    time_window_ = sec;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::set_overlay_enabled(bool enabled) {
    overlay_enabled_ = enabled;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::set_frozen(bool frozen) {
    frozen_ = frozen;
    if (frozen_) {
        capture_snapshot();
    } else {
        frozen_series_.clear();
        frozen_keys_.clear();
    }
    if (!frozen_) {
        hover_active_ = false;
        hover_values_.clear();
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::reset_visual() {
    hover_active_ = false;
    hover_values_.clear();
    frozen_series_.clear();
    frozen_keys_.clear();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::request_temporary_fit(double now, double duration_sec) {
    fit_until_ts_ = now + duration_sec;
}

void PlotView::update_from_model(double now) {
    last_now_ = now;
    if (frozen_) {
        return;
    }

    if (!model_) {
        return;
    }

    double data_min = 0.0;
    double data_max = 0.0;
    bool has_data = false;

    auto enabled_keys = model_->get_enabled_keys_with_data();
    auto keys = model_->get_keys();
    for (const auto& key : keys) {
        ensure_color(key);
    }

    for (const auto& key : enabled_keys) {
        auto series = model_->get_series(key);
        if (series.empty()) {
            continue;
        }

        double t_end = series.back().t;
        double t_start = t_end - time_window_;

        for (const auto& sample : series) {
            if (sample.t < t_start) {
                continue;
            }
            if (!has_data) {
                data_min = sample.v;
                data_max = sample.v;
                has_data = true;
            } else {
                data_min = std::min<double>(data_min, sample.v);
                data_max = std::max<double>(data_max, sample.v);
            }
        }
    }

    if (has_data) {
        update_y_range(data_min, data_max);
    }

    if (fit_until_ts_ > now) {
        fit_enabled_channels();
    }

    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::capture_snapshot() {
    frozen_series_.clear();
    frozen_keys_.clear();
    if (!model_) {
        return;
    }

    auto keys = model_->get_keys();
    for (const auto& key : keys) {
        if (!model_->is_enabled(key)) {
            continue;
        }
        auto series = model_->get_series(key);
        if (series.empty()) {
            continue;
        }
        ensure_color(key);
        frozen_series_[key] = std::move(series);
        frozen_keys_.push_back(key);
    }
}

std::vector<std::string> PlotView::get_active_keys() const {
    if (frozen_) {
        return frozen_keys_;
    }
    if (!model_) {
        return {};
    }
    return model_->get_enabled_keys_with_data();
}

void PlotView::fit_enabled_channels() {
    if (!model_) {
        return;
    }

    std::vector<int> vals;
    auto enabled = model_->get_enabled_keys_with_data();
    for (const auto& key : enabled) {
        auto series = model_->get_series(key);
        for (const auto& sample : series) {
            vals.push_back(sample.v);
        }
    }

    if (vals.empty()) {
        return;
    }

    int y_min = *std::min_element(vals.begin(), vals.end());
    int y_max = *std::max_element(vals.begin(), vals.end());

    double span = static_cast<double>(y_max - y_min);
    if (span < 1.0) {
        span = 1.0;
    }

    double pad = std::max<double>(span * 0.05, 1.0);
    double target_min = static_cast<double>(y_min) - pad;
    double target_max = static_cast<double>(y_max) + pad;

    if (target_min < 0 && y_min >= 0) {
        target_min = 0;
    }

    if (target_max - target_min < kMinVisibleSpan) {
        target_max = target_min + kMinVisibleSpan;
    }

    y_min_ = target_min;
    y_max_ = target_max;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void PlotView::update_y_range(double data_min, double data_max) {
    (void)data_min;
    RECT client = {};
    GetClientRect(hwnd_, &client);
    RECT plot_rect = plot_rect_from_client(client);
    int plot_h = std::max<int>(1, static_cast<int>(plot_rect.bottom - plot_rect.top));
    if (plot_h < 10) {
        return;
    }
    double y_per_px = (y_max_ - y_min_) / static_cast<double>(plot_h);

    double data_required_max = data_max + (kAutoExpandPadPx * y_per_px);
    double overlay_required_max = compute_overlay_required_y_max(plot_rect);
    double required_max = std::max<double>(data_required_max, overlay_required_max);

    double minspan_required_max = y_min_ + kMinVisibleSpan;
    if (required_max < minspan_required_max) {
        required_max = minspan_required_max;
    }

    if (required_max > y_max_) {
        y_max_ = required_max;
    }
}

double PlotView::compute_overlay_required_y_max(const RECT& plot_rect) const {
    if (!model_ || !overlay_enabled_) {
        return -1.0;
    }

    int plot_h = std::max<int>(1, static_cast<int>(plot_rect.bottom - plot_rect.top));
    if (plot_h < 10) {
        return -1.0;
    }
    double y_per_px = (y_max_ - y_min_) / static_cast<double>(plot_h);
    double dy = kEndTagYOffsetPx * y_per_px + (kAutoExpandPadPx * y_per_px);

    double required = -1.0;
    auto enabled = model_->get_enabled_keys_with_data();
    for (const auto& key : enabled) {
        auto series = model_->get_series(key);
        if (series.empty()) {
            continue;
        }
        double v = static_cast<double>(series.back().v);
        double y = v + dy;
        if (y > required) {
            required = y;
        }
    }
    return required;
}

RECT PlotView::plot_rect_from_client(const RECT& client) const {
    RECT plot = client;
    plot.left += kMarginLeft;
    plot.right -= kMarginRight;
    plot.top += kMarginTop;
    plot.bottom -= kMarginBottom;
    if (plot.right < plot.left + 10) {
        plot.right = plot.left + 10;
    }
    if (plot.bottom < plot.top + 10) {
        plot.bottom = plot.top + 10;
    }
    return plot;
}

double PlotView::data_to_x(const RECT& plot_rect, double x) const {
    double w = static_cast<double>(plot_rect.right - plot_rect.left);
    if (time_window_ <= 0.0) {
        return plot_rect.left;
    }
    return plot_rect.left + (x / time_window_) * w;
}

double PlotView::data_to_y(const RECT& plot_rect, double y) const {
    double h = static_cast<double>(plot_rect.bottom - plot_rect.top);
    double span = y_max_ - y_min_;
    if (span <= 0.0) {
        span = 1.0;
    }
    return plot_rect.bottom - ((y - y_min_) / span) * h;
}

double PlotView::x_to_data(const RECT& plot_rect, int x) const {
    double w = static_cast<double>(plot_rect.right - plot_rect.left);
    if (w <= 0.0) {
        return 0.0;
    }
    double t = (static_cast<double>(x - plot_rect.left) / w) * time_window_;
    if (t < 0.0) {
        t = 0.0;
    }
    if (t > time_window_) {
        t = time_window_;
    }
    return t;
}

void PlotView::ensure_color(const std::string& key) {
    if (color_map_.find(key) != color_map_.end()) {
        return;
    }
    size_t idx = color_order_.size() % (sizeof(kColorTable) / sizeof(kColorTable[0]));
    color_map_[key] = kColorTable[idx];
    color_order_.push_back(key);
}

COLORREF PlotView::get_color(const std::string& key) const {
    auto it = color_map_.find(key);
    if (it != color_map_.end()) {
        return it->second;
    }
    return RGB(200, 200, 200);
}

LRESULT CALLBACK PlotView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PlotView* self = reinterpret_cast<PlotView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<PlotView*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        return self->handle_message(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PlotView::handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        paint();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEMOVE: {
        if (!model_ || !frozen_) {
            break;
        }
        RECT client = {};
        GetClientRect(hwnd, &client);
        RECT plot_rect = plot_rect_from_client(client);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        POINT pt = { x, y };
        if (!PtInRect(&plot_rect, pt)) {
            if (hover_active_) {
                hover_active_ = false;
                hover_values_.clear();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        }

        double t_view = x_to_data(plot_rect, x);
        if (t_view < 0.0 || t_view > time_window_) {
            if (hover_active_) {
                hover_active_ = false;
                hover_values_.clear();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        }

        hover_values_.clear();
        double snap_t = -1.0;
        auto enabled = get_active_keys();
        for (const auto& key : enabled) {
            std::vector<ChannelSample> temp;
            const std::vector<ChannelSample>* series_ptr = nullptr;
            if (frozen_) {
                auto it = frozen_series_.find(key);
                if (it == frozen_series_.end()) {
                    continue;
                }
                series_ptr = &it->second;
            } else {
                temp = model_->get_series(key);
                if (temp.empty()) {
                    continue;
                }
                series_ptr = &temp;
            }

            const auto& series = *series_ptr;
            if (series.empty()) {
                continue;
            }
            double t_end = series.back().t;
            double t_start = t_end - time_window_;

            std::vector<std::pair<double, int>> windowed;
            for (const auto& sample : series) {
                if (sample.t >= t_start) {
                    windowed.emplace_back(sample.t - t_start, sample.v);
                }
            }
            if (windowed.empty()) {
                continue;
            }

            size_t best = 0;
            double best_dist = std::abs(windowed[0].first - t_view);
            for (size_t i = 1; i < windowed.size(); ++i) {
                double dist = std::abs(windowed[i].first - t_view);
                if (dist < best_dist) {
                    best_dist = dist;
                    best = i;
                }
            }

            double real_t = windowed[best].first;
            int value = windowed[best].second;
            if (snap_t < 0.0) {
                snap_t = real_t;
            }
            hover_values_.push_back({key, value});
        }

        if (hover_values_.empty()) {
            if (hover_active_) {
                hover_active_ = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        }

        hover_t_ = snap_t;
        hover_active_ = true;
        InvalidateRect(hwnd, nullptr, FALSE);

        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        break;
    }
    case WM_MOUSELEAVE:
        hover_active_ = false;
        hover_values_.clear();
        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PlotView::paint() {
    PAINTSTRUCT ps = {};
    HDC hdc = BeginPaint(hwnd_, &ps);
    if (hdc) {
        RECT client = {};
        GetClientRect(hwnd_, &client);
        int w = static_cast<int>(client.right - client.left);
        int h = static_cast<int>(client.bottom - client.top);

        if (w > 0 && h > 0) {
            HDC memdc = CreateCompatibleDC(hdc);
            HBITMAP membmp = CreateCompatibleBitmap(hdc, w, h);
            HGDIOBJ oldbmp = SelectObject(memdc, membmp);

            draw_plot(memdc, client);

            BitBlt(hdc, 0, 0, w, h, memdc, 0, 0, SRCCOPY);

            SelectObject(memdc, oldbmp);
            DeleteObject(membmp);
            DeleteDC(memdc);
        } else {
            draw_plot(hdc, client);
        }
    }
    EndPaint(hwnd_, &ps);
}

void PlotView::draw_plot(HDC hdc, const RECT& client) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.SetCompositingQuality(CompositingQualityHighQuality);
    g.Clear(Color(255, 0, 0, 0));

    RECT plot_rect = plot_rect_from_client(client);

    Font tick_font(L"Segoe UI", 9, FontStyleRegular);
    SolidBrush tick_brush(Color(180, 180, 180));

    Pen grid_pen(Color(255, 50, 50, 50), 1.0f);
    Pen axis_pen(Color(255, 90, 90, 90), 1.0f);
    g.DrawRectangle(&axis_pen,
                    static_cast<float>(plot_rect.left),
                    static_cast<float>(plot_rect.top),
                    static_cast<float>(plot_rect.right - plot_rect.left),
                    static_cast<float>(plot_rect.bottom - plot_rect.top));

    Font label_font(L"Segoe UI", 10, FontStyleRegular);
    SolidBrush label_brush(Color(220, 220, 220));
    RectF layout;
    std::wstring x_label = L"Time (s)";
    std::vector<std::string> enabled;
    if (model_) {
        enabled = get_active_keys();
    }

    double y_span = y_max_ - y_min_;
    if (y_span <= 0.0) {
        y_span = 1.0;
    }

    // Y ticks: python-style auto ticks (nice numbers)
    double y_step = auto_tick_step(y_min_, y_max_, 6);

    if (y_step <= 0.0) {
        y_step = 1.0;
    }

    double y_start = std::floor(y_min_ / y_step) * y_step;
    double y_end = std::ceil(y_max_ / y_step) * y_step;
    if (y_end - y_start < y_step) {
        y_end = y_start + y_step;
    }

    float max_y_tick_w = 0.0f;
    {
        std::wstring y_text = format_tick_int(y_end);
        RectF bounds;
        g.MeasureString(y_text.c_str(), -1, &tick_font, PointF(0, 0), &bounds);
        max_y_tick_w = bounds.Width;
    }

    // X label outside tick numbers
    g.MeasureString(x_label.c_str(), -1, &label_font, PointF(0, 0), &layout);
    float x_label_x = (plot_rect.left + plot_rect.right) * 0.5f - layout.Width * 0.5f;
    float x_label_y = static_cast<float>(plot_rect.bottom) + tick_font.GetHeight(&g) + 6.0f;
    g.DrawString(x_label.c_str(), -1, &label_font, PointF(x_label_x, x_label_y), &label_brush);

    // Y label outside tick numbers
    std::wstring y_label = L"Value";
    g.MeasureString(y_label.c_str(), -1, &label_font, PointF(0, 0), &layout);
    GraphicsState state = g.Save();
    float y_center = (plot_rect.top + plot_rect.bottom) * 0.5f;
    float y_label_x = static_cast<float>(plot_rect.left) - max_y_tick_w - layout.Height - 14.0f;
    if (y_label_x < static_cast<float>(client.left + 4)) {
        y_label_x = static_cast<float>(client.left + 4);
    }
    g.TranslateTransform(y_label_x, y_center);
    g.RotateTransform(-90.0f);
    g.DrawString(y_label.c_str(), -1, &label_font, PointF(0.0f, -layout.Width * 0.5f), &label_brush);
    g.Restore(state);

    for (double y_val = y_start; y_val <= y_end + 0.001; y_val += y_step) {
        if (y_val < y_min_ - 1e-6 || y_val > y_max_ + 1e-6) {
            continue;
        }
        float y = static_cast<float>(data_to_y(plot_rect, y_val));
        g.DrawLine(&grid_pen, static_cast<float>(plot_rect.left), y,
                   static_cast<float>(plot_rect.right), y);

        std::wstring y_text = format_tick_int(y_val);
        RectF bounds;
        g.MeasureString(y_text.c_str(), -1, &tick_font, PointF(0, 0), &bounds);
        float tx = static_cast<float>(plot_rect.left) - bounds.Width - 6.0f;
        float ty = y - bounds.Height * 0.5f;
        g.DrawString(y_text.c_str(), -1, &tick_font, PointF(tx, ty), &tick_brush);
    }

    // X ticks: start at 1, integer labels
    double x_step = 1.0;
    if (time_window_ > 1.0) {
        RectF bounds;
        std::wstring sample = format_tick_int(time_window_);
        g.MeasureString(sample.c_str(), -1, &tick_font, PointF(0, 0), &bounds);
        int plot_w = std::max<int>(1, static_cast<int>(plot_rect.right - plot_rect.left));
        int max_labels = std::max<int>(1, static_cast<int>(plot_w / std::max(1.0f, bounds.Width + 10.0f)));
        double raw_step = time_window_ / static_cast<double>(max_labels);
        x_step = std::max(1.0, std::ceil(raw_step));
    }

    for (double x_val = x_step; x_val <= time_window_ + 0.001; x_val += x_step) {
        float x = static_cast<float>(data_to_x(plot_rect, x_val));
        g.DrawLine(&grid_pen, x, static_cast<float>(plot_rect.top),
                   x, static_cast<float>(plot_rect.bottom));

        std::wstring x_text = format_tick_int(x_val);
        RectF bounds;
        g.MeasureString(x_text.c_str(), -1, &tick_font, PointF(0, 0), &bounds);
        float tx = x - bounds.Width * 0.5f;
        float ty = static_cast<float>(plot_rect.bottom + 2);
        g.DrawString(x_text.c_str(), -1, &tick_font, PointF(tx, ty), &tick_brush);
    }

    if (!model_) {
        return;
    }

    for (const auto& key : enabled) {
        ensure_color(key);
        std::vector<ChannelSample> temp;
        const std::vector<ChannelSample>* series_ptr = nullptr;
        if (frozen_) {
            auto it = frozen_series_.find(key);
            if (it == frozen_series_.end()) {
                continue;
            }
            series_ptr = &it->second;
        } else {
            temp = model_->get_series(key);
            if (temp.empty()) {
                continue;
            }
            series_ptr = &temp;
        }

        const auto& series = *series_ptr;
        if (series.size() < 2) {
            continue;
        }

        double t_end = series.back().t;
        double t_start = t_end - time_window_;
        std::vector<ChannelSample> windowed;
        for (const auto& sample : series) {
            if (sample.t >= t_start) {
                windowed.push_back(sample);
            }
        }
        if (windowed.size() < 2) {
            continue;
        }

        std::vector<ChannelSample> simplified;
        simplified.reserve(windowed.size());
        int last_px = INT_MIN;
        for (const auto& sample : windowed) {
            double x_data = sample.t - t_start;
            int px = static_cast<int>(std::lround(data_to_x(plot_rect, x_data)));
            if (px == last_px) {
                if (!simplified.empty()) {
                    simplified.back() = sample;
                }
            } else {
                simplified.push_back(sample);
                last_px = px;
            }
        }
        const auto& draw_series = simplified.size() >= 2 ? simplified : windowed;

        COLORREF color = get_color(key);
        Pen pen(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)), 5.0f);
        pen.SetLineJoin(LineJoinRound);

        for (size_t i = 0; i + 1 < draw_series.size(); ++i) {
            double x0 = draw_series[i].t - t_start;
            double x1 = draw_series[i + 1].t - t_start;
            double y0 = draw_series[i].v;
            double y1 = draw_series[i + 1].v;

            float px0 = static_cast<float>(data_to_x(plot_rect, x0));
            float px1 = static_cast<float>(data_to_x(plot_rect, x1));
            float py0 = static_cast<float>(data_to_y(plot_rect, y0));
            float py1 = static_cast<float>(data_to_y(plot_rect, y1));

            g.DrawLine(&pen, px0, py0, px1, py0);
            g.DrawLine(&pen, px1, py0, px1, py1);
        }
    }

    if (overlay_enabled_) {
        int plot_w = std::max<int>(1, static_cast<int>(plot_rect.right - plot_rect.left));
        int plot_h = std::max<int>(1, static_cast<int>(plot_rect.bottom - plot_rect.top));
        double x_per_px = time_window_ / static_cast<double>(plot_w);
        double y_per_px = (y_max_ - y_min_) / static_cast<double>(plot_h);
        double base_x = time_window_ - (kEndTagXMarginPx * x_per_px);

        std::vector<std::pair<float, double>> placed; // scene_y, left_x_data

        Font tag_font(L"Segoe UI", 10, FontStyleBold);
        SolidBrush bg_brush(Color(25, 0, 0, 0));

        for (const auto& key : enabled) {
            std::vector<ChannelSample> temp;
            const std::vector<ChannelSample>* series_ptr = nullptr;
            if (frozen_) {
                auto it = frozen_series_.find(key);
                if (it == frozen_series_.end()) {
                    continue;
                }
                series_ptr = &it->second;
            } else {
                temp = model_->get_series(key);
                if (temp.empty()) {
                    continue;
                }
                series_ptr = &temp;
            }

            const auto& series = *series_ptr;
            if (series.empty()) {
                continue;
            }
            double value = series.back().v;
            double y = value + (kEndTagYOffsetPx * y_per_px);

            double top_limit = y_max_ - (kEndTagSafeMarginPx * y_per_px);
            double bot_limit = y_min_ + (kEndTagSafeMarginPx * y_per_px);
            if (y > top_limit) {
                y = top_limit;
            } else if (y < bot_limit) {
                y = bot_limit;
            }

            double x = base_x;

            std::wstring text = to_wstring(static_cast<int>(value));
            RectF bounds;
            g.MeasureString(text.c_str(), -1, &tag_font, PointF(0, 0), &bounds);

            float scene_y = static_cast<float>(data_to_y(plot_rect, y));
            for (const auto& prev : placed) {
                if (std::abs(prev.first - scene_y) < kEndTagYThresholdPx) {
                    x = prev.second - (kEndTagGapPx * x_per_px);
                }
            }

            double left_x_data = x - (bounds.Width * x_per_px);
            if (left_x_data < 0.0) {
                x = (bounds.Width * x_per_px) + (kEndTagSafeMarginPx * x_per_px);
                left_x_data = x - (bounds.Width * x_per_px);
            }
            if (x > time_window_) {
                x = time_window_ - (kEndTagSafeMarginPx * x_per_px);
            }

            float px = static_cast<float>(data_to_x(plot_rect, x));
            float py = static_cast<float>(data_to_y(plot_rect, y));

            RectF bg(px - bounds.Width - 6.0f, py - bounds.Height * 0.5f - 3.0f,
                     bounds.Width + 12.0f, bounds.Height + 6.0f);
            g.FillRectangle(&bg_brush, bg);

            COLORREF color = get_color(key);
            SolidBrush text_brush(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
            g.DrawString(text.c_str(), -1, &tag_font, PointF(px - bounds.Width, py - bounds.Height * 0.5f), &text_brush);

            placed.push_back({scene_y, left_x_data});
        }
    }

    if (hover_active_) {
        draw_hover(hdc, plot_rect);
    }

}

void PlotView::draw_hover(HDC hdc, const RECT& plot_rect) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.SetCompositingQuality(CompositingQualityHighQuality);

    double x = data_to_x(plot_rect, hover_t_);
    Pen line_pen(Color(180, 180, 180), 1.0f);
    line_pen.SetDashStyle(DashStyleDash);
    g.DrawLine(&line_pen, static_cast<float>(x), static_cast<float>(plot_rect.top),
               static_cast<float>(x), static_cast<float>(plot_rect.bottom));

    std::wstringstream ss;
    ss << L"t = " << to_wstring(hover_t_) << L" s";

    std::vector<std::wstring> lines;
    lines.push_back(ss.str());
    for (const auto& item : hover_values_) {
        std::wstring key = to_wstring(item.first);
        std::wstring line = key + L": " + to_wstring(item.second);
        lines.push_back(line);
    }

    Font font(L"Segoe UI", 9, FontStyleRegular);
    float max_w = 0.0f;
    float line_h = 0.0f;
    for (const auto& line : lines) {
        RectF bounds;
        g.MeasureString(line.c_str(), -1, &font, PointF(0, 0), &bounds);
        max_w = std::max<float>(max_w, bounds.Width);
        line_h = std::max<float>(line_h, bounds.Height);
    }

    float padding = 6.0f;
    float box_w = max_w + padding * 2.0f;
    float box_h = line_h * static_cast<float>(lines.size()) + padding * 2.0f;

    float box_x = static_cast<float>(plot_rect.left + 6);
    float box_y = static_cast<float>(plot_rect.top + 6);

    SolidBrush bg(Color(180, 0, 0, 0));
    g.FillRectangle(&bg, box_x, box_y, box_w, box_h);

    for (size_t i = 0; i < lines.size(); ++i) {
        float y = box_y + padding + line_h * static_cast<float>(i);
        if (i == 0) {
            SolidBrush white(Color(255, 255, 255, 255));
            g.DrawString(lines[i].c_str(), -1, &font, PointF(box_x + padding, y), &white);
        } else {
            const auto& key = hover_values_[i - 1].first;
            COLORREF color = get_color(key);
            SolidBrush brush(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
            g.DrawString(lines[i].c_str(), -1, &font, PointF(box_x + padding, y), &brush);
        }
    }
}
