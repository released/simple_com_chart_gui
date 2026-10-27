#include <afxwin.h>
#include "plot_view.h"
#include "chart_time.h"
#include "resource.h"
#include "uart_files.h"
#include <windowsx.h>
#include <cmath>
#include <algorithm>

namespace {
class YDialog:public CDialog {
public:
    YDialog(HWND owner,double low,double high):CDialog(IDD_Y_RANGE,CWnd::FromHandle(owner)),low(low),high(high){}
    double low,high;
    BOOL OnInitDialog() override {CDialog::OnInitDialog();CString text;text.Format(L"%.10g",low);SetDlgItemTextW(IDC_Y_LOW,text);text.Format(L"%.10g",high);SetDlgItemTextW(IDC_Y_HIGH,text);return TRUE;}
    void OnOK() override {CString a,b;GetDlgItemTextW(IDC_Y_LOW,a);GetDlgItemTextW(IDC_Y_HIGH,b);
        if(!uart_number(a.GetString(),-1e100,1e100,low)||!uart_number(b.GetString(),-1e100,1e100,high)||low>=high){MessageBoxW(L"Enter finite minimum < maximum.",L"Fixed Y",MB_OK|MB_ICONWARNING);return;}CDialog::OnOK();}
};
}
void PlotView::return_to_latest() {follow_latest_=true;last_now_=latest_now_;fit_enabled_channels();}
void PlotView::navigate_to(double end) {if(!std::isfinite(end)) return;follow_latest_=false;last_now_=end;fit_enabled_channels();}
bool PlotView::set_fixed_y(bool enabled,double low,double high) {if(enabled && (!std::isfinite(low)||!std::isfinite(high)||low>=high||std::abs(low)>1e100||std::abs(high)>1e100)) return false;fixed_y_=enabled;if(enabled){y_min_=low;y_max_=high;InvalidateRect(hwnd_,nullptr,FALSE);}else fit_enabled_channels();return true;}
void PlotView::notify_cursor(bool second,int x) {
    RECT client{};GetClientRect(hwnd_,&client);const auto plot=plot_rect_from_client(client);
    const double time=std::max(absolute_time_axis_?0.0:-1e100,last_now_-time_window_+x_to_data(plot,x));
    if(!analysis_enabled_) {cursor_a_=cursor_b_=time;analysis_enabled_=true;}
    if(second) cursor_b_=time;else cursor_a_=time;
    InvalidateRect(hwnd_,nullptr,FALSE);
    SendMessageW(GetParent(hwnd_),kCursorMoved,second,reinterpret_cast<LPARAM>(&time));
}
bool PlotView::handle_navigation(UINT message,WPARAM w,LPARAM l,LRESULT& result) {
    result=0;
    if(message==WM_MOUSEWHEEL) {
        POINT point{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(hwnd_,&point);RECT client{};GetClientRect(hwnd_,&client);const auto plot=plot_rect_from_client(client);
        const double fraction=std::clamp(static_cast<double>(point.x-plot.left)/std::max(1L,plot.right-plot.left),0.0,1.0);
        const double anchor=last_now_-time_window_+fraction*time_window_,next=std::clamp(time_window_*(GET_WHEEL_DELTA_WPARAM(w)>0?0.8:1.25),chart_time::minimum,86400.0);
        time_window_=next;follow_latest_=false;last_now_=anchor+(1-fraction)*next;fit_enabled_channels();return true;
    }
    if(message==WM_MBUTTONDOWN || ((message==WM_LBUTTONDOWN) && (GetKeyState(VK_SPACE)<0 || (w&MK_CONTROL)))) {
        dragging_=3;drag_x_=GET_X_LPARAM(l);drag_end_=last_now_;follow_latest_=false;SetCapture(hwnd_);return true;
    }
    if(message==WM_RBUTTONDOWN && (w&MK_CONTROL)) {
        HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"Return to latest (double-click)");AppendMenuW(menu,MF_STRING,2,L"Automatic Y");AppendMenuW(menu,MF_STRING,3,L"Fixed Y range...");
        POINT point{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ClientToScreen(hwnd_,&point);
        const auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,hwnd_,nullptr);DestroyMenu(menu);
        if(command==1) return_to_latest();else if(command==2) set_fixed_y(false);else if(command==3) {YDialog dialog(hwnd_,y_min_,y_max_);if(dialog.DoModal()==IDOK && IsWindow(hwnd_)) set_fixed_y(true,dialog.low,dialog.high);}return true;
    }
    if(message==WM_LBUTTONDBLCLK) {return_to_latest();return true;}
    if(message==WM_LBUTTONDOWN||message==WM_RBUTTONDOWN) {
        RECT client{};GetClientRect(hwnd_,&client);const auto plot=plot_rect_from_client(client);POINT point{GET_X_LPARAM(l),GET_Y_LPARAM(l)};if(!PtInRect(&plot,point)) return false;
        dragging_=message==WM_RBUTTONDOWN?2:1;SetCapture(hwnd_);notify_cursor(dragging_==2,point.x);return true;
    }
    if(message==WM_MOUSEMOVE && dragging_) {
        if(dragging_==3) {RECT client{};GetClientRect(hwnd_,&client);const auto plot=plot_rect_from_client(client);last_now_=drag_end_-(GET_X_LPARAM(l)-drag_x_)*time_window_/std::max(1L,plot.right-plot.left);fit_enabled_channels();}
        else notify_cursor(dragging_==2,GET_X_LPARAM(l));return true;
    }
    if(message==WM_LBUTTONUP||message==WM_RBUTTONUP||message==WM_MBUTTONUP||message==WM_CAPTURECHANGED) {if(dragging_){dragging_=0;if(GetCapture()==hwnd_) ReleaseCapture();return true;}}
    return false;
}
