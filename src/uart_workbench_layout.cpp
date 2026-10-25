#include "uart_workbench.h"
#include "dpi_support.h"
#include <algorithm>

int UartWorkbench::layout_group(size_t i,CRect bounds,bool measure){
    auto& g=groups_[i];if(!measure){g.bounds=bounds;::MoveWindow(g.box,bounds.left,bounds.top,bounds.Width(),bounds.Height(),TRUE);}
    HDC dc=::GetDC(m_hWnd);auto previous=SelectObject(dc,font_);TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);
    const auto width=[&](HWND h,int minimum){const auto n=::GetWindowTextLengthW(h);std::wstring s(static_cast<size_t>(n)+1,0);::GetWindowTextW(h,s.data(),n+1);SIZE size{};GetTextExtentPoint32W(dc,s.data(),n,&size);return std::max<int>(minimum,size.cx+padding_);};
    const int left=padding_,right=bounds.Width()-padding_;int x=left,y=tm.tmHeight+padding_;
    const auto place=[&](HWND h,int w,bool drop=false){if(x+w>right&&x>left){x=left;y+=row_+gap_;}if(!measure)::MoveWindow(h,x,y,std::min(w,right-x),drop?row_*7:row_,TRUE);x+=w+gap_;};
    const auto next=[&]{x=left;y+=row_+gap_;};
    SIZE disconnect{};GetTextExtentPoint32W(dc,L"Disconnect",10,&disconnect);const int connect_width=std::max<int>(100,disconnect.cx+24);
    const int com_width=std::clamp(bounds.Width()-width(g.port_label,40)-width(g.scan,60)-connect_width-width(g.baud_label,45)-120-gap_*7-padding_*2,150,390);
    place(g.port_label,width(g.port_label,40));place(g.port,com_width,true);place(g.scan,width(g.scan,60));place(g.connect,connect_width);place(g.baud_label,width(g.baud_label,45));place(g.baud,110,true);next();
    place(g.data_label,width(g.data_label,36));place(g.data,55,true);place(g.parity_label,width(g.parity_label,45));place(g.parity,82,true);place(g.stop_label,width(g.stop_label,38));place(g.stop,60,true);
    place(g.view,std::max<int>(160,tm.tmAveCharWidth*21),true);place(g.follow,width(g.follow,120)+padding_*2);place(g.clear,width(g.clear,65));place(g.save,width(g.save,88));place(g.record,width(g.record,110));next();
    const int hex_width=width(g.hex,85),send_width=width(g.send,65);place(g.input,std::max(120,right-left-hex_width-send_width-gap_*2));place(g.hex,hex_width);place(g.send,send_width);next();
    const int status_y=bounds.Height()-padding_-row_;const int log_bottom=status_y-gap_;
    if(!measure){::MoveWindow(g.log,left,y,right-left,std::max(24,log_bottom-y),TRUE);::MoveWindow(g.status,left,status_y,right-left,row_,TRUE);}
    SelectObject(dc,previous);::ReleaseDC(m_hWnd,dc);
    return y+90+gap_+row_+padding_;
}
void UartWorkbench::layout(){
    if(!ready_)return;SetRedraw(FALSE);CRect r;GetClientRect(&r);HDC dc=::GetDC(m_hWnd);auto old=SelectObject(dc,font_);TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);SelectObject(dc,old);::ReleaseDC(m_hWnd,dc);
    row_=std::max<int>(24,tm.tmHeight+10);padding_=std::max<int>(8,tm.tmAveCharWidth);gap_=std::max<int>(4,tm.tmAveCharWidth/2);
    ::MoveWindow(hint_,padding_,padding_,std::max(1,r.Width()-padding_*2),row_,TRUE);
    const int top=padding_+row_+gap_,bottom=r.bottom-padding_,usable=bottom-top-gap_*2;
    const auto probe=CRect(padding_,0,r.right-padding_,0);const int minA=std::min(usable/2,layout_group(0,probe,true)),minB=std::min(usable/2,layout_group(1,probe,true));split_y_=top+std::clamp(static_cast<int>(usable*ratio_),minA,std::max(minA,usable-minB));
    layout_group(0,CRect(padding_,top,r.right-padding_,split_y_-gap_));layout_group(1,CRect(padding_,split_y_+gap_,r.right-padding_,bottom));
    SetRedraw(TRUE);::RedrawWindow(m_hWnd,nullptr,nullptr,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
}
void UartWorkbench::OnSize(UINT type,int w,int h){CDialog::OnSize(type,w,h);if(type!=SIZE_MINIMIZED)layout();}
void UartWorkbench::OnGetMinMaxInfo(MINMAXINFO* info){CDialog::OnGetMinMaxInfo(info);info->ptMinTrackSize.x=900;info->ptMinTrackSize.y=700;if(ready_){const CRect probe(0,0,880-padding_*2,0);info->ptMinTrackSize.y=std::max<LONG>(700,layout_group(0,probe,true)+layout_group(1,probe,true)+row_+padding_*2+gap_*3+45);}}
LRESULT UartWorkbench::OnDpiChanged(WPARAM,LPARAM l){
    const auto prior=owned_font_;owned_font_=make_ui_font(m_hWnd);if(owned_font_)font_=owned_font_;
    ::EnumChildWindows(m_hWnd,[](HWND h,LPARAM font)->BOOL{::SendMessageW(h,WM_SETFONT,static_cast<WPARAM>(font),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font_));
    const auto old_log=log_font_;LOGFONTW lf{};GetObjectW(font_,sizeof(lf),&lf);wcscpy_s(lf.lfFaceName,L"Consolas");log_font_=CreateFontIndirectW(&lf);for(auto& g:groups_)::SendMessageW(g.log,WM_SETFONT,reinterpret_cast<WPARAM>(log_font_?log_font_:font_),FALSE);
    if(prior)DeleteObject(prior);if(old_log)DeleteObject(old_log);if(l){const auto r=reinterpret_cast<const RECT*>(l);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout();return 0;
}
void UartWorkbench::OnLButtonDown(UINT flags,CPoint p){if(std::abs(p.y-split_y_)<=gap_){dragging_=true;SetCapture();}else CDialog::OnLButtonDown(flags,p);}
void UartWorkbench::OnLButtonUp(UINT flags,CPoint p){if(dragging_){dragging_=false;ReleaseCapture();WritePrivateProfileStringW(L"DualCOM",L"SplitPercent",std::to_wstring(static_cast<int>(ratio_*100)).c_str(),ini_.c_str());}CDialog::OnLButtonUp(flags,p);}
void UartWorkbench::OnMouseMove(UINT flags,CPoint p){if(dragging_){CRect r;GetClientRect(&r);const int top=padding_+row_+gap_;ratio_=std::clamp(static_cast<double>(p.y-top)/std::max<int>(1,r.bottom-padding_-top),0.25,0.75);layout();}CDialog::OnMouseMove(flags,p);}
BOOL UartWorkbench::OnSetCursor(CWnd* wnd,UINT hit,UINT message){CPoint p;GetCursorPos(&p);ScreenToClient(&p);if(std::abs(p.y-split_y_)<=gap_){::SetCursor(LoadCursorW(nullptr,IDC_SIZENS));return TRUE;}return CDialog::OnSetCursor(wnd,hit,message);}
