#include "protocol_page.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include <commctrl.h>
#include <algorithm>

BEGIN_MESSAGE_MAP(ProtocolPage,CWnd)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_VSCROLL()
END_MESSAGE_MAP()
bool ProtocolPage::create(CWnd* parent,Mode mode,HFONT font,UINT id,std::function<void(unsigned)> action) {
    mode_=mode;font_=font;action_=std::move(action);
    const auto cls=AfxRegisterWndClass(0,LoadCursorW(nullptr,IDC_ARROW),GetSysColorBrush(COLOR_BTNFACE));
    if(!CreateEx(WS_EX_CONTROLPARENT,cls,mode==Mode::Modbus?L"Modbus":L"LIN",WS_CHILD|WS_VSCROLL|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CRect(0,0,1,1),parent,id))return false;
    const auto make=[&](const wchar_t* type,const wchar_t* text,DWORD style,unsigned control){return ui::create_window(type,text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(control)),nullptr,nullptr);};
    std::vector<const wchar_t*> captions,columns;
    if(mode_==Mode::Modbus){
        target_=make(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,50);ui::choices(target_);
        for(const auto s:{L"Port A",L"Port B"})::SendMessageW(target_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s));::SendMessageW(target_,CB_SETCURSEL,0,0);toolbar_.push_back(target_);
        captions={L"Connections...",L"Add block...",L"Edit block...",L"Remove block",L"Load definitions...",L"Save definitions...",L"Start polling",L"Stop polling",L"Transactions..."};
        columns={L"Name",L"Unit ID",L"Function",L"Wire offset",L"Reference",L"Count",L"Type",L"Order",L"Value",L"Age (ms)",L"State"};
    }else{
        captions={L"Load LIN trace...",L"Enter LIN frames...",L"Signal definitions...",L"Load LDF...",L"Save definitions...",L"Apply to charts",L"Save LIN trace...",L"Clear LIN view",L"LIN schedule...",L"LIN interface...",L"Load signals..."};
        columns={L"Time (s)",L"ID (HEX)",L"PID",L"Data HEX",L"Checksum",L"PID check",L"Checksum check",L"Break",L"Direction",L"Signals"};
    }
    for(size_t n=0;n<captions.size();++n)toolbar_.push_back(make(L"BUTTON",captions[n],BS_PUSHBUTTON,static_cast<unsigned>(n+1)));
    if(mode_==Mode::Lin){unknown_=make(L"BUTTON",L"Allow unverified frames in charts",BS_AUTOCHECKBOX,51);toolbar_.push_back(unknown_);}
    help_=ui::create_window(L"STATIC",mode_==Mode::Modbus?
        L"Read-only RTU. Start polling begins a new numeric capture; raw recording is unchanged. Select signals in Chart / 4 Charts.":
        L"LIN offline analysis. Missing PID, checksum or Break metadata remains unknown. Invalid frames never update charts. Hardware interface is selected separately.",
        WS_CHILD|WS_VISIBLE|SS_NOPREFIX,0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    list_=make(WC_LISTVIEWW,L"",WS_BORDER|LVS_REPORT|LVS_OWNERDATA|LVS_SHOWSELALWAYS|LVS_SINGLESEL,60);
    ListView_SetExtendedListViewStyle(list_,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
    for(size_t n=0;n<columns.size();++n){LVCOLUMNW column{};column.mask=LVCF_TEXT|LVCF_WIDTH;column.pszText=const_cast<wchar_t*>(ui::caption(columns[n]));column.cx=n==3&&mode_==Mode::Lin?210:110;ListView_InsertColumn(list_,static_cast<int>(n),&column);}
    status_=make(L"EDIT",L"",WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,61);
    set_font(font);state(false);return true;
}
void ProtocolPage::set_font(HFONT font) {
    font_=font;if(!GetSafeHwnd())return;
    ::EnumChildWindows(m_hWnd,[](HWND child,LPARAM value)->BOOL{::SendMessageW(child,WM_SETFONT,static_cast<WPARAM>(value),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font));
    layout();
}
size_t ProtocolPage::target()const {return target_&&::SendMessageW(target_,CB_GETCURSEL,0,0)==1?1:0;}
bool ProtocolPage::allow_unknown()const {return unknown_&&::SendMessageW(unknown_,BM_GETCHECK,0,0)==BST_CHECKED;}
int ProtocolPage::selected()const {return list_?ListView_GetNextItem(list_,-1,LVNI_SELECTED):-1;}
void ProtocolPage::set_rows(Rows rows,bool preserve) {
    if(rows==rows_)return;
    const int selection=preserve?selected():-1;const int top=ListView_GetTopIndex(list_);
    const bool count_changed=rows.size()!=rows_.size();rows_=std::move(rows);if(count_changed)ListView_SetItemCountEx(list_,static_cast<int>(rows_.size()),LVSICF_NOSCROLL);
    if(!preserve){ListView_SetItemState(list_,-1,0,LVIS_SELECTED|LVIS_FOCUSED);}
    else if(selection>=0&&static_cast<size_t>(selection)<rows_.size()){ListView_SetItemState(list_,selection,LVIS_SELECTED,LVIS_SELECTED);}
    if(preserve&&top>=0&&static_cast<size_t>(top)<rows_.size())ListView_EnsureVisible(list_,top,FALSE);
    ::InvalidateRect(list_,nullptr,FALSE);
}
void ProtocolPage::status(const std::wstring& text) {
    notice_=text;update_status();
}
void ProtocolPage::summary(const std::wstring& text){summary_=text;update_status();}
void ProtocolPage::update_status(){const auto text=summary_+(summary_.empty()||notice_.empty()?L"":L"\r\n")+notice_;const auto current=ui::control_value(status_);if(current!=text)::SetWindowTextW(status_,text.substr(0,8192).c_str());}
void ProtocolPage::state(bool busy,bool running,bool has_rows) {
    for(auto child:toolbar_){const auto id=::GetDlgCtrlID(child);bool enabled=!busy;
        if(mode_==Mode::Modbus){if(id==8)enabled=running;else if(id>=2&&id<=7)enabled=!busy&&!running;}
        else if(id==8)enabled=true;
        else if(id==6||id==7)enabled=!busy&&has_rows;
        if((::IsWindowEnabled(child)!=FALSE)!=enabled)::EnableWindow(child,enabled);}
}
BOOL ProtocolPage::OnCommand(WPARAM w,LPARAM l) {
    const auto id=LOWORD(w);
    if((id>=1&&id<=11&&HIWORD(w)==BN_CLICKED)||(id==50&&HIWORD(w)==CBN_SELCHANGE)) {
        if(l&&!::IsWindowEnabled(reinterpret_cast<HWND>(l)))return TRUE;
        const auto action=action_;if(action)action(id);return TRUE;
    }
    return CWnd::OnCommand(w,l);
}
BOOL ProtocolPage::OnNotify(WPARAM w,LPARAM l,LRESULT* result) {
    const auto header=reinterpret_cast<NMHDR*>(l);
    if(header&&header->hwndFrom==list_&&header->code==LVN_GETDISPINFOW){const auto display=reinterpret_cast<NMLVDISPINFOW*>(l);
        if(display->item.mask&LVIF_TEXT){const auto row=display->item.iItem,column=display->item.iSubItem;
            display->item.pszText=const_cast<wchar_t*>(row>=0&&column>=0&&static_cast<size_t>(row)<rows_.size()&&static_cast<size_t>(column)<rows_[row].size()?rows_[row][column].c_str():L"");}
        *result=0;return TRUE;
    }
    if(header&&header->hwndFrom==list_&&header->code==NM_DBLCLK&&mode_==Mode::Modbus){const auto action=action_;if(action)action(3);*result=0;return TRUE;}
    return CWnd::OnNotify(w,l,result);
}
void ProtocolPage::layout() {
    if(!list_||!GetSafeHwnd()||laying_out_)return;laying_out_=true;
    struct Guard{bool& flag;~Guard(){flag=false;}}guard{laying_out_};
    CRect r;GetClientRect(&r);const int gap=6,row=std::max(28,control_font_height(help_)+12),width=std::max(1,r.Width()-2*gap);
    int x=gap,y=gap;
    for(const auto child:toolbar_){const auto id=::GetDlgCtrlID(child);const int size=std::min(width,id==50?110:std::max(85,control_text_width(child,id==51?30:26)));
        if(x>gap&&x+size>r.right-gap){x=gap;y+=row+gap;}::MoveWindow(child,x,y,size,id==50?row*5:row,TRUE);x+=size+gap;}
    y+=row+gap;HDC dc=::GetDC(help_);const auto previous=SelectObject(dc,font_?font_:GetStockObject(DEFAULT_GUI_FONT));const auto text=ui::control_value(help_);RECT paragraph{0,0,width,0};
    ::DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&paragraph,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,previous);::ReleaseDC(help_,dc);
    const int help_height=std::max(row,static_cast<int>(paragraph.bottom));::MoveWindow(help_,gap,y,width,help_height,TRUE);y+=help_height+gap;
    const int footer=row*3;content_height_=std::max(r.Height(),y+footer+90+2*gap);
    scroll_=std::clamp(scroll_,0,std::max(0,content_height_-r.Height()));
    for(const auto child:toolbar_){RECT rect{};::GetWindowRect(child,&rect);::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&rect),2);::MoveWindow(child,rect.left,rect.top-scroll_,rect.right-rect.left,rect.bottom-rect.top,TRUE);}
    ::MoveWindow(help_,gap,y-help_height-gap-scroll_,width,help_height,TRUE);
    const int table_height=std::max(90,content_height_-y-footer-2*gap);
    ::MoveWindow(list_,gap,y-scroll_,width,table_height,TRUE);::MoveWindow(status_,gap,y+table_height+gap-scroll_,width,footer,TRUE);
    SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};info.nMax=content_height_-1;info.nPage=std::max(1,r.Height());info.nPos=scroll_;SetScrollInfo(SB_VERT,&info,TRUE);
}
void ProtocolPage::OnSize(UINT type,int w,int h){CWnd::OnSize(type,w,h);if(type!=SIZE_MINIMIZED)layout();}
BOOL ProtocolPage::OnEraseBkgnd(CDC* dc){CRect r;GetClientRect(&r);::FillRect(dc->GetSafeHdc(),r,GetSysColorBrush(COLOR_BTNFACE));return TRUE;}
void ProtocolPage::OnVScroll(UINT code,UINT,CScrollBar*) {
    CRect r;GetClientRect(&r);int next=scroll_;
    if(code==SB_LINEUP)next-=32;else if(code==SB_LINEDOWN)next+=32;else if(code==SB_PAGEUP)next-=r.Height();else if(code==SB_PAGEDOWN)next+=r.Height();
    else if(code==SB_TOP)next=0;else if(code==SB_BOTTOM)next=content_height_;else if(code==SB_THUMBPOSITION||code==SB_THUMBTRACK){SCROLLINFO info{sizeof(info),SIF_TRACKPOS};GetScrollInfo(SB_VERT,&info);next=info.nTrackPos;}else return;
    scroll_=std::clamp(next,0,std::max(0,content_height_-r.Height()));layout();Invalidate(FALSE);
}
void ProtocolPage::reveal_focus() {
    const auto focus=::GetFocus();if(!focus||!::IsChild(m_hWnd,focus))return;CRect item,r;::GetWindowRect(focus,&item);ScreenToClient(&item);GetClientRect(&r);
    int delta=item.top<0?item.top:item.bottom>r.bottom?item.bottom-r.bottom:0;
    if(delta){scroll_=std::clamp(scroll_+delta,0,std::max(0,content_height_-r.Height()));layout();Invalidate(FALSE);}
}
