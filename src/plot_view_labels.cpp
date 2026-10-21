#include "plot_view.h"
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

void PlotView::draw_end_labels(Gdiplus::Graphics& graphics, const RECT& plot) {
    using namespace Gdiplus;
    Font font(L"Segoe UI", compact_ ? 9.0f : 10.0f, FontStyleBold);
    SolidBrush background(Color(230, 0, 0, 0));
    const float available = static_cast<float>(plot.right - plot.left - 12);
    for (const auto& key : get_active_keys()) {
        std::vector<ChannelSample> live;
        const std::vector<ChannelSample>* series = nullptr;
        if (frozen_) {
            const auto found = frozen_series_.find(key);
            if (found != frozen_series_.end()) series = &found->second;
        } else { live = model_->get_series(key); series = &live; }
        if (!series || series->empty()) continue;
        const auto& latest = series->back();
        if (latest.t < last_now_ - time_window_ || !std::isfinite(latest.v)) continue;

        wchar_t value[64] = {};
        swprintf_s(value, L"%.10g", latest.v);
        std::wstring name;
        if (show_names_) {
            const int length = MultiByteToWideChar(CP_UTF8, 0, key.data(), static_cast<int>(key.size()), nullptr, 0);
            name.resize(length);
            MultiByteToWideChar(CP_UTF8, 0, key.data(), static_cast<int>(key.size()), name.data(), length);
        }
        std::wstring text = name.empty() ? value : name + L": " + value;
        RectF measured;
        const auto measure = [&] { graphics.MeasureString(text.c_str(), -1, &font, PointF(0, 0), &measured); };
        measure();
        // Shorten only the name, keeping the numeric value intact on small plots.
        while (!name.empty() && measured.Width > available) {
            name.pop_back();
            if (!name.empty() && name.back() >= 0xD800 && name.back() <= 0xDBFF) name.pop_back();
            text = name + L"...: " + value; measure();
        }
        if (measured.Width > available) { text = value; measure(); }

        const float point_x = static_cast<float>(data_to_x(plot, latest.t - (last_now_ - time_window_)));
        const float point_y = static_cast<float>(data_to_y(plot, latest.v));
        const float width = measured.Width + 8, height = measured.Height + 4;
        const float left_min = static_cast<float>(plot.left + 2), top_min = static_cast<float>(plot.top + 2);
        const float right_max = static_cast<float>(plot.right - 2), bottom_max = static_cast<float>(plot.bottom - 2);
        float left = std::clamp(point_x - width - 6, left_min, std::max(left_min, right_max - width));
        float top = std::clamp(point_y - height - 6, top_min, std::max(top_min, bottom_max - height));
        // Prefer nearby free space when several latest points share the same Y.
        for (const auto& previous : end_labels_) {
            const auto& box = previous.bounds;
            if (left < box.right + 4 && left + width > box.left - 4 && top < box.bottom + 4 && top + height > box.top - 4) {
                left = static_cast<float>(box.left) - width - 6;
                if (left < left_min) {
                    left = std::clamp(point_x - width - 6, left_min, std::max(left_min, right_max - width));
                    top = std::min(std::max(top_min, static_cast<float>(box.bottom + 4)), std::max(top_min, bottom_max - height));
                }
            }
        }
        const auto color = get_color(key);
        SolidBrush brush(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
        Pen connector(Color(150, GetRValue(color), GetGValue(color), GetBValue(color)), 1.0f);
        graphics.DrawLine(&connector, point_x, point_y, std::clamp(point_x, left, left + width), top + height);
        graphics.FillEllipse(&brush, point_x - 3, point_y - 3, 6.0f, 6.0f);
        graphics.FillRectangle(&background, left, top, width, height);
        graphics.DrawString(text.c_str(), -1, &font, PointF(left + 4, top + 2), &brush);
        end_labels_.push_back({key, text, latest.v,
            {static_cast<LONG>(std::floor(left)), static_cast<LONG>(std::floor(top)),
             static_cast<LONG>(std::ceil(left + width)), static_cast<LONG>(std::ceil(top + height))},
            {static_cast<LONG>(std::lround(point_x)), static_cast<LONG>(std::lround(point_y))}});
    }
}
