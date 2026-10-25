#include "uart_tool_form.h"
#include "resource.h"
#include "dpi_support.h"
#include <algorithm>
#include <exception>

namespace {
std::wstring field_text(HWND h){std::wstring out(static_cast<size_t>(::GetWindowTextLengthW(h))+1,0);::GetWindowTextW(h,out.data(),static_cast<int>(out.size()));out.pop_back();return out;}
}
LRESULT UartFormViewport::WindowProc(UINT message,WPARAM w,LPARAM l){
    if(message==WM_COMMAND||message==WM_VSCROLL||(message>=WM_CTLCOLORMSGBOX&&message<=WM_CTLCOLORSTATIC))return ::SendMessageW(::GetParent(m_hWnd),message,w,message==WM_VSCROLL?0:l);
    if(message==WM_ERASEBKGND){CRect r;GetClientRect(&r);::FillRect(reinterpret_cast<HDC>(w),r,GetSysColorBrush(COLOR_3DFACE));return TRUE;}
    if(message==WM_PAINT){CPaintDC dc(this);CRect r;GetClientRect(&r);dc.FillSolidRect(r,GetSysColor(COLOR_3DFACE));return 0;}
    return CWnd::WindowProc(message,w,l);
}
BEGIN_MESSAGE_MAP(UartToolForm,CDialog)
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_WM_DESTROY()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(WM_DPICHANGED,&UartToolForm::OnDpiChanged)
END_MESSAGE_MAP()
UartToolForm::UartToolForm(std::wstring title,std::wstring help,std::vector<Field> fields,Preview preview,Validate validate,CWnd* owner)
    :CDialog(IDD_UART_TOOL_FORM,owner),title_(std::move(title)),help_(std::move(help)),fields_(std::move(fields)),preview_(std::move(preview)),validate_(std::move(validate)){}
BOOL UartToolForm::OnInitDialog(){
    CDialog::OnInitDialog();SetWindowTextW(title_.c_str());font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    const auto cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursorW(nullptr,IDC_ARROW),GetSysColorBrush(COLOR_3DFACE));
    viewport_.CreateEx(WS_EX_CONTROLPARENT,cls,L"Options",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_CLIPCHILDREN,CRect(0,0,1,1),this,4000);
    help_label_=::CreateWindowW(L"STATIC",help_.c_str(),WS_CHILD|WS_VISIBLE|SS_NOPREFIX,0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    for(size_t i=0;i<fields_.size();++i){const auto& f=fields_[i];labels_.push_back(::CreateWindowW(L"STATIC",f.label.c_str(),WS_CHILD|WS_VISIBLE|SS_NOPREFIX|SS_CENTERIMAGE,0,0,1,1,viewport_.GetSafeHwnd(),nullptr,nullptr,nullptr));
        const DWORD style=f.choices.empty()?WS_BORDER|(f.rows>1?ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL:ES_AUTOHSCROLL):CBS_DROPDOWNLIST|WS_VSCROLL;
        const auto input=::CreateWindowW(f.choices.empty()?L"EDIT":L"COMBOBOX",f.choices.empty()?f.value.c_str():L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,0,0,1,1,viewport_.GetSafeHwnd(),reinterpret_cast<HMENU>(static_cast<INT_PTR>(4100+i)),nullptr,nullptr);inputs_.push_back(input);
        if(f.choices.empty())::SendMessageW(input,EM_SETLIMITTEXT,32768,0);else {for(const auto& s:f.choices)::SendMessageW(input,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));const auto at=std::find(f.choices.begin(),f.choices.end(),f.value);::SendMessageW(input,CB_SETCURSEL,at==f.choices.end()?0:at-f.choices.begin(),0);}
    }
    preview_button_=::CreateWindowW(L"BUTTON",L"Preview",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(4500),nullptr,nullptr);::EnableWindow(preview_button_,static_cast<bool>(preview_));
    apply_=::CreateWindowW(L"BUTTON",L"Apply",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(IDOK),nullptr,nullptr);
    cancel_=::CreateWindowW(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(IDCANCEL),nullptr,nullptr);
    result_=::CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY,0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    ::EnumChildWindows(m_hWnd,[](HWND h,LPARAM font)->BOOL{::SendMessageW(h,WM_SETFONT,static_cast<WPARAM>(font),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font_?font_:GetStockObject(DEFAULT_GUI_FONT)));
    ready_=true;CRect work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);SetWindowPos(nullptr,work.left+60,work.top+30,std::min<int>(860,work.Width()-100),std::min<int>(900,work.Height()-60),SWP_NOZORDER);layout();return TRUE;
}
std::vector<std::wstring> UartToolForm::values()const{std::vector<std::wstring> out;for(size_t i=0;i<fields_.size();++i)out.push_back(!ready_?fields_[i].value:field_text(inputs_[i]));return out;}
void UartToolForm::layout(){if(!ready_)return;CRect r;GetClientRect(&r);HDC dc=::GetDC(m_hWnd);const auto old=SelectObject(dc,font_?font_:GetStockObject(DEFAULT_GUI_FONT));TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);row_=std::max<int>(24,tm.tmHeight+10);RECT paragraph{0,0,std::max(100,r.Width()-20),0};::DrawTextW(dc,help_.c_str(),static_cast<int>(help_.size()),&paragraph,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);const int help_height=std::max<int>(row_,paragraph.bottom+8);int measured_label=0;for(const auto& field:fields_){SIZE size{};GetTextExtentPoint32W(dc,field.label.c_str(),static_cast<int>(field.label.size()),&size);measured_label=std::max<int>(measured_label,size.cx+16);}SelectObject(dc,old);::ReleaseDC(m_hWnd,dc);
    ::MoveWindow(help_label_,10,8,r.Width()-20,help_height,TRUE);const int footer=row_*5+28,top=help_height+16;viewport_.MoveWindow(10,top,r.Width()-20,std::max(80,r.Height()-top-footer));CRect inner;viewport_.GetClientRect(&inner);int y=0;const int label_width=std::clamp(measured_label,inner.Width()/4,inner.Width()/2);
    content_=0;for(const auto& f:fields_)content_+=row_*std::max(1,f.rows)+8;offset_=std::clamp(offset_,0,std::max<int>(0,content_-inner.Height()));
    for(size_t i=0;i<fields_.size();++i){const int height=row_*std::max(1,fields_[i].rows);::MoveWindow(labels_[i],4,y-offset_,label_width-8,row_,TRUE);::MoveWindow(inputs_[i],label_width,y-offset_,std::max(60,inner.Width()-label_width-8),fields_[i].choices.empty()?height:row_*7,TRUE);y+=height+8;}
    content_=y;SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};info.nMin=0;info.nMax=std::max(0,content_-1);info.nPage=inner.Height();info.nPos=offset_;::SetScrollInfo(viewport_.GetSafeHwnd(),SB_VERT,&info,TRUE);
    const int result_y=r.Height()-footer+row_+8;::MoveWindow(preview_button_,10,r.Height()-footer,100,row_,TRUE);::MoveWindow(apply_,r.Width()-210,r.Height()-footer,90,row_,TRUE);::MoveWindow(cancel_,r.Width()-110,r.Height()-footer,100,row_,TRUE);::MoveWindow(result_,10,result_y,r.Width()-20,std::max(row_,r.Height()-result_y-10),TRUE);::RedrawWindow(m_hWnd,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}
void UartToolForm::scroll(int value){CRect r;viewport_.GetClientRect(&r);offset_=std::clamp(value,0,std::max<int>(0,content_-r.Height()));layout();}
void UartToolForm::OnVScroll(UINT code,UINT position,CScrollBar*){int next=offset_;CRect r;viewport_.GetClientRect(&r);if(code==SB_LINEUP)next-=row_;else if(code==SB_LINEDOWN)next+=row_;else if(code==SB_PAGEUP)next-=r.Height();else if(code==SB_PAGEDOWN)next+=r.Height();else if(code==SB_TOP)next=0;else if(code==SB_BOTTOM)next=content_;else if(code==SB_THUMBTRACK||code==SB_THUMBPOSITION){SCROLLINFO info{sizeof(info),SIF_TRACKPOS};::GetScrollInfo(viewport_.GetSafeHwnd(),SB_VERT,&info);next=info.nTrackPos;}else return;scroll(next);(void)position;}
BOOL UartToolForm::OnCommand(WPARAM w,LPARAM l){if(LOWORD(w)==4500&&preview_){try{const auto output=preview_(values());::SetWindowTextW(result_,output.substr(0,32768).c_str());}catch(const std::exception&){::SetWindowTextW(result_,L"Cannot preview these values; check encoding and field limits.");}return TRUE;}if((HIWORD(w)==EN_SETFOCUS||HIWORD(w)==CBN_SETFOCUS)&&l){CRect item,area;::GetWindowRect(reinterpret_cast<HWND>(l),&item);viewport_.GetClientRect(&area);viewport_.ScreenToClient(&item);if(item.Height()>area.Height())scroll(offset_+item.top);else if(item.top<0)scroll(offset_+item.top);else if(item.bottom>area.bottom)scroll(offset_+item.bottom-area.bottom);}return CDialog::OnCommand(w,l);}
void UartToolForm::OnOK(){try{const auto input=values();std::wstring error;if(validate_&&!validate_(input,error)){::SetWindowTextW(result_,error.c_str());return;}for(size_t i=0;i<fields_.size();++i)fields_[i].value=input[i];CDialog::OnOK();}catch(const std::exception&){::SetWindowTextW(result_,L"Invalid values or encoding; previous settings retained.");}}
LRESULT UartToolForm::OnDpiChanged(WPARAM,LPARAM l){const auto next=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);if(next){const auto old=font_;font_=next;::EnumChildWindows(m_hWnd,[](HWND h,LPARAM f)->BOOL{::SendMessageW(h,WM_SETFONT,static_cast<WPARAM>(f),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font_));if(old)DeleteObject(old);}if(l){const auto r=reinterpret_cast<const RECT*>(l);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout();return 0;}
void UartToolForm::OnSize(UINT type,int w,int h){CDialog::OnSize(type,w,h);if(type!=SIZE_MINIMIZED)layout();}
void UartToolForm::OnGetMinMaxInfo(MINMAXINFO* info){CDialog::OnGetMinMaxInfo(info);info->ptMinTrackSize.x=620;info->ptMinTrackSize.y=500;if(font_){HDC dc=::GetDC(m_hWnd);const auto old=SelectObject(dc,font_);RECT help{0,0,580,0};::DrawTextW(dc,help_.c_str(),static_cast<int>(help_.size()),&help,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);TEXTMETRICW metric{};GetTextMetricsW(dc,&metric);SelectObject(dc,old);::ReleaseDC(m_hWnd,dc);info->ptMinTrackSize.y=std::max<LONG>(500,help.bottom+std::max<LONG>(24,metric.tmHeight+10)*5+180);}}
void UartToolForm::OnDestroy(){ready_=false;for(auto h=::GetWindow(m_hWnd,GW_CHILD);h;){const auto next=::GetWindow(h,GW_HWNDNEXT);::DestroyWindow(h);h=next;}CDialog::OnDestroy();if(font_){DeleteObject(font_);font_=nullptr;}}
