#include "calculus_view.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
bool CalculusView::create(CWnd* owner){const auto cls=AfxRegisterWndClass(0,::LoadCursorW(nullptr,IDC_ARROW),GetSysColorBrush(COLOR_WINDOW));return CreateEx(WS_EX_CLIENTEDGE,cls,L"Time calculus result",WS_CHILD|WS_VISIBLE,CRect(0,0,1,1),owner,6030)!=FALSE;}
LRESULT CalculusView::WindowProc(UINT m,WPARAM w,LPARAM l){if(m==WM_SETFONT){font_=reinterpret_cast<HFONT>(w);if(l)Invalidate(FALSE);return 0;}if(m==WM_GETFONT)return reinterpret_cast<LRESULT>(font_);if(m==WM_ERASEBKGND)return 1;if(m==WM_PAINT){paint();return 0;}return CWnd::WindowProc(m,w,l);}
void CalculusView::paint(){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,GetSysColor(COLOR_WINDOW));const auto old=font_?dc.SelectObject(CFont::FromHandle(font_)):nullptr;dc.SetBkMode(TRANSPARENT);dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
    bool found=false;long double xmin=0,xmax=1,ymin=0,ymax=1;const CalculusPoint* latest=nullptr;
    if(data_)for(const auto& p:data_->points)if(p.valid){if(!found){xmin=xmax=p.seconds;ymin=ymax=p.value;found=true;}xmin=std::min(xmin,static_cast<long double>(p.seconds));xmax=std::max(xmax,static_cast<long double>(p.seconds));ymin=std::min(ymin,static_cast<long double>(p.value));ymax=std::max(ymax,static_cast<long double>(p.value));latest=&p;}
    CRect plot=r;plot.DeflateRect(90,30,20,35);if(plot.Width()<30||plot.Height()<20){if(old)dc.SelectObject(old);return;}
    if(!found){dc.TextOutW(10,10,L"Select a retained key and Calculate.");if(old)dc.SelectObject(old);return;}
    if(xmin==xmax){xmin-=.5L;xmax+=.5L;}if(ymin==ymax){const auto pad=std::max(1e-6L,std::abs(ymin)*.05L),bound=static_cast<long double>(std::numeric_limits<double>::max());ymin=ymin < -bound+pad?-bound:ymin-pad;ymax=ymax > bound-pad?bound:ymax+pad;}
    const auto position=[](long double v,long double a,long double b){const auto s=std::max({std::abs(a),std::abs(b),1e-100L});return std::clamp((v/s-a/s)/(b/s-a/s),0.0L,1.0L);};
    const auto number=[](long double v){wchar_t s[64]{};swprintf_s(s,L"%.7g",static_cast<double>(v));return std::wstring(s);};
    dc.Rectangle(plot);dc.TextOutW(3,plot.top,number(ymax).c_str());dc.TextOutW(3,plot.bottom-20,number(ymin).c_str());dc.TextOutW(plot.left,plot.bottom+5,number(xmin).c_str());dc.TextOutW(std::max(plot.left,plot.right-100),plot.bottom+5,number(xmax).c_str());
    CPen pen(PS_SOLID,1,RGB(40,40,40));const auto prior=dc.SelectObject(&pen);bool connected=false;size_t segment=0;const auto saved=dc.SaveDC();dc.IntersectClipRect(plot);
    for(const auto& p:data_->points){if(!p.valid){connected=false;continue;}const int x=plot.left+static_cast<int>(position(p.seconds,xmin,xmax)*(plot.Width()-1)),y=plot.bottom-1-static_cast<int>(position(p.value,ymin,ymax)*(plot.Height()-1));if(!connected||segment!=p.segment)dc.MoveTo(x,y);else dc.LineTo(x,y);dc.FillSolidRect(x-1,y-1,3,3,RGB(40,40,40));connected=true;segment=p.segment;}
    dc.RestoreDC(saved);dc.SelectObject(prior);if(latest)dc.TextOutW(plot.left,4,(L"Latest valid: "+number(latest->value)+L" @ "+number(latest->seconds)+L" s").c_str());if(old)dc.SelectObject(old);
}
