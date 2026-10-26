#include "digital_view.h"
#include "uart_files.h"
#include <algorithm>
#include <cmath>
BEGIN_MESSAGE_MAP(DigitalView,CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
LRESULT DigitalView::WindowProc(UINT message,WPARAM w,LPARAM l){if(message==WM_SETFONT){font_=reinterpret_cast<HFONT>(w);if(l&&GetSafeHwnd())Invalidate(FALSE);return 0;}if(message==WM_GETFONT)return reinterpret_cast<LRESULT>(font_);return CWnd::WindowProc(message,w,l);}
bool DigitalView::create(CWnd* owner) {
    const auto cls=AfxRegisterWndClass(0,::LoadCursorW(nullptr,IDC_ARROW),nullptr);
    return CreateEx(0,cls,L"Reported digital state history",WS_CHILD|WS_VISIBLE|WS_BORDER,CRect(0,0,1,1),owner,6800)!=FALSE;
}
void DigitalView::update(CaptureSnapshot data,std::vector<DigitalLane> lanes,double window,double hold) {
    data_=std::move(data);lanes_=std::move(lanes);window_=window;hold_=hold;Invalidate(FALSE);
}
void DigitalView::OnPaint() {
    CPaintDC target(this);CRect r;GetClientRect(&r);if(r.Width()<1||r.Height()<1)return;
    CDC dc;dc.CreateCompatibleDC(&target);CBitmap bitmap;bitmap.CreateCompatibleBitmap(&target,r.Width(),r.Height());const auto old_bitmap=dc.SelectObject(&bitmap);
    dc.FillSolidRect(r,RGB(250,250,250));dc.SetBkMode(TRANSPARENT);dc.SetTextColor(RGB(35,35,35));
    const auto old_font=dc.SelectObject(CFont::FromHandle(font_?font_:reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))));
    TEXTMETRICW metric{};dc.GetTextMetricsW(&metric);const int row=std::max<int>(26,metric.tmHeight+10);
    const int left=std::min(180,r.Width()/3),right=r.right-12;double end=0;for(const auto& item:data_.rows)end=std::max(end,item->seconds);const double start=std::max(0.,end-window_);
    const auto x=[&](double t){return left+static_cast<int>((std::clamp(t,start,end)-start)/std::max(0.001,end-start)*(right-left));};
    CPen pen(PS_SOLID,1,RGB(70,70,70));const auto old_pen=dc.SelectObject(&pen);
    for(size_t lane=0;lane<lanes_.size();++lane) {
        const int y=10+static_cast<int>(lane)*row*2;if(y+row>r.bottom-24)break;
        const auto& config=lanes_[lane];
        CRect label(8,y,left-6,y+row);const auto name=uart_wide(config.name);dc.DrawTextW(name.c_str(),-1,&label,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        dc.FillSolidRect(left,y,right-left,row,RGB(235,235,235));
        const auto history=digital_history(data_,config,window_,hold_);
        for(const auto& segment:history.segments) {
                const int a=x(segment.first),b=x(segment.last);if(b>a) {
                    const int level=config.width==1&&!config.whole?(segment.value?y+4:y+row-4):y+row/2;
                    dc.MoveTo(a,level);dc.LineTo(b,level);
                    if(segment.next&&*segment.next!=segment.value&&config.width==1&&!config.whole){dc.MoveTo(b,level);dc.LineTo(b,*segment.next?y+4:y+row-4);}
                    if(config.whole||config.width>1){const auto number=std::to_wstring(segment.value);CRect box(a+2,y,b,y+row);if(b-a>dc.GetTextExtent(number.c_str()).cx+4)dc.DrawTextW(number.c_str(),-1,&box,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);}
                }
        }
        if(history.latest){const int at=x(history.latest->first),level=config.width==1&&!config.whole?(history.latest->second?y+4:y+row-4):y+row/2;dc.Ellipse(at-2,level-2,at+3,level+3);CString latest;latest.Format(L"last %u @ %.3f s",history.latest->second,history.latest->first);CRect caption(8,y+row,left-6,y+row*2);dc.DrawTextW(latest,&caption,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);}
    }
    CString axis;axis.Format(L"%.3f .. %.3f s | reported values only; gaps/unknown are blank",start,end);CRect footer(8,r.bottom-24,r.right-8,r.bottom);dc.DrawTextW(axis,&footer,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);
    dc.SelectObject(old_pen);dc.SelectObject(old_font);target.BitBlt(0,0,r.Width(),r.Height(),&dc,0,0,SRCCOPY);dc.SelectObject(old_bitmap);
}
