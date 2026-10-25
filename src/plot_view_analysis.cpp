#include "plot_view.h"
#include <gdiplus.h>
#include <cmath>
void PlotView::set_analysis(double a,double b,bool reference,double threshold) {
    analysis_enabled_=std::isfinite(a)&&std::isfinite(b); cursor_a_=a; cursor_b_=b;
    reference_enabled_=reference&&std::isfinite(threshold); reference_y_=threshold;
    if (IsWindow(hwnd_)) InvalidateRect(hwnd_,nullptr,FALSE);
}
void PlotView::clear_analysis() { analysis_enabled_=reference_enabled_=false; if (IsWindow(hwnd_)) InvalidateRect(hwnd_,nullptr,FALSE); }
void PlotView::draw_analysis(Gdiplus::Graphics& graphics,const RECT& rect) {
    using namespace Gdiplus; analysis_lines_=0; if (!analysis_enabled_) return;
    Pen pen(Color(255,210,210,210),1.0f); pen.SetDashStyle(DashStyleDash);
    Font font(L"Segoe UI",9); SolidBrush brush(Color(255,230,230,230));
    const double start=last_now_-time_window_;
    const auto cursor=[&](double value,const wchar_t* name) {
        if (value<start || value>last_now_) return;
        const auto x=static_cast<float>(data_to_x(rect,value-start));
        graphics.DrawLine(&pen,x,static_cast<float>(rect.top),x,static_cast<float>(rect.bottom));
        graphics.DrawString(name,-1,&font,Gdiplus::PointF(x+2,static_cast<float>(rect.top+2)),&brush); ++analysis_lines_;
    };
    cursor(cursor_a_,L"A"); cursor(cursor_b_,L"B");
    if (reference_enabled_ && reference_y_>=y_min_ && reference_y_<=y_max_) {
        const auto y=static_cast<float>(data_to_y(rect,reference_y_));
        graphics.DrawLine(&pen,static_cast<float>(rect.left),y,static_cast<float>(rect.right),y); ++analysis_lines_;
    }
}
