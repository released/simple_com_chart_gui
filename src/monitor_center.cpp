#include "monitor_center.h"
#include "resource.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include "dpi_support.h"
#include "message_format.h"
#include <commctrl.h>
#include <algorithm>
BEGIN_MESSAGE_MAP(MonitorCenter,CDialog)
 ON_WM_SIZE()
 ON_WM_DESTROY()
 ON_WM_GETMINMAXINFO()
 ON_MESSAGE(WM_DPICHANGED,&MonitorCenter::OnDpiChanged)
END_MESSAGE_MAP()
MonitorCenter::MonitorCenter():CDialog(IDD_UART_TOOL_FORM){}
bool MonitorCenter::show(CWnd* owner,bool events){if(!GetSafeHwnd()&&!Create(IDD_UART_TOOL_FORM,owner))return false;::SendMessageW(mode_,CB_SETCURSEL,events?1:0,0);rebuild();ShowWindow(SW_SHOW);SetForegroundWindow();return true;}
BOOL MonitorCenter::OnInitDialog(){CDialog::OnInitDialog();SetWindowTextW(ui::caption(L"Monitor center"));font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    const auto child=[&](const wchar_t* cls,const wchar_t* caption,DWORD style,int id){auto h=ui::create_window(cls,caption,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);::SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(font_),FALSE);return h;};
    mode_=child(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,10);for(const auto text:{L"Recent resources",L"Events"})::SendMessageW(mode_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(ui::caption(text)));::SendMessageW(mode_,CB_SETCURSEL,0,0);
    flash_=child(L"BUTTON",L"Flash taskbar for background errors",BS_AUTOCHECKBOX,11);
    list_=child(WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|WS_BORDER,12);ListView_SetExtendedListViewStyle(list_,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
    name_=child(L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,13);::SendMessageW(name_,EM_SETLIMITTEXT,64,0);
    const wchar_t* captions[]={L"Stage in tool",L"Open folder",L"Rename label",L"Remove recent",L"Clear events",L"Close",L"Add resource..."};for(int i=0;i<7;++i)buttons_.push_back(child(L"BUTTON",captions[i],BS_PUSHBUTTON,20+i));
    details_=child(L"EDIT",L"",WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY,14);::SendMessageW(details_,EM_SETLIMITTEXT,32768,0);
    ready_=true;RECT area{};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);SetWindowPos(nullptr,area.left+30,area.top+30,std::min<LONG>(1100,area.right-area.left-60),std::min<LONG>(760,area.bottom-area.top-60),SWP_NOZORDER);layout();rebuild();return TRUE;
}
void MonitorCenter::update(const std::vector<RecentResource>& recent,const MonitorEvents& events,bool flash){
    const bool changed=encode_recent_resources(recent_)!=encode_recent_resources(recent);
    const bool events_changed=event_revision_!=events.revision();event_revision_=events.revision();
    const bool event_mode=ready_&&::SendMessageW(mode_,CB_GETCURSEL,0,0)==1;
    const int old_index=ready_?ListView_GetNextItem(list_,-1,LVNI_SELECTED):-1;
    uint64_t sequence=0;RecentResource selected_resource;bool had_resource=false;
    if(old_index>=0){if(event_mode&&static_cast<size_t>(old_index)<events_.size())sequence=events_[events_.size()-1-old_index].sequence;
        else if(!event_mode&&static_cast<size_t>(old_index)<recent_.size()){selected_resource=recent_[old_index];had_resource=true;}}
    const bool editing=ready_&&::GetFocus()==name_;const auto edited=editing?ui::control_value(name_):std::wstring{};
    recent_=recent;events_=events.rows();if(!ready_)return;
    ::SendMessageW(flash_,BM_SETCHECK,flash?BST_CHECKED:BST_UNCHECKED,0);
    if(!IsWindowVisible()||(event_mode?!events_changed:!changed))return;
    rebuild();int selected_index=-1;
    if(event_mode&&sequence){for(size_t i=0;i<events_.size();++i)if(events_[i].sequence==sequence)selected_index=static_cast<int>(events_.size()-1-i);}
    else if(had_resource){for(size_t i=0;i<recent_.size();++i)if(recent_[i].kind==selected_resource.kind&&recent_[i].path==selected_resource.path)selected_index=static_cast<int>(i);}
    if(selected_index>=0){ListView_SetItemState(list_,-1,0,LVIS_SELECTED|LVIS_FOCUSED);ListView_SetItemState(list_,selected_index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);selected();if(editing&&!event_mode&&recent_[selected_index].name==selected_resource.name)::SetWindowTextW(name_,edited.c_str());}
}
void MonitorCenter::rebuild(){if(!ready_)return;const bool events=::SendMessageW(mode_,CB_GETCURSEL,0,0)==1;const int old_selection=ListView_GetNextItem(list_,-1,LVNI_SELECTED);ListView_DeleteAllItems(list_);while(ListView_DeleteColumn(list_,0)){}
    const wchar_t* recent_headers[]={L"Type",L"Name",L"Path"};const wchar_t* event_headers[]={L"PC local time",L"Source",L"Event",L"Count",L"Details"};const int columns=events?5:3;
    for(int i=0;i<columns;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.pszText=const_cast<wchar_t*>(ui::caption(events?event_headers[i]:recent_headers[i]));c.cx=events?(i==0?160:i==4?500:130):(i==0?120:i==1?220:650);ListView_InsertColumn(list_,i,&c);}
    const wchar_t* kinds[]={L"Recording",L"Profile",L"Chart view",L"Session"};const size_t count=events?events_.size():recent_.size();
    for(size_t i=0;i<count;++i){std::vector<std::wstring> row;if(events){const auto& e=events_[events_.size()-1-i];row={format_message_timestamp(e.unix_ms,0,0,MessageClock::Local),e.source,ui::tr(e.kind),std::to_wstring(e.count),ui::tr(e.detail)};}else{const auto& r=recent_[i];row={ui::tr(kinds[static_cast<unsigned>(r.kind)]),r.name,r.path};}LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=static_cast<int>(i);item.pszText=row[0].data();ListView_InsertItem(list_,&item);for(int c=1;c<columns;++c)ListView_SetItemText(list_,item.iItem,c,row[c].data());}
    if(count)ListView_SetItemState(list_,std::clamp(old_selection,0,static_cast<int>(count)-1),LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);selected();
}
void MonitorCenter::selected(){if(!ready_)return;const bool events=::SendMessageW(mode_,CB_GETCURSEL,0,0)==1;const int at=ListView_GetNextItem(list_,-1,LVNI_SELECTED);const bool valid=at>=0&&static_cast<size_t>(at)<(events?events_.size():recent_.size());
    for(int i=0;i<4;++i)::EnableWindow(buttons_[i],!events&&valid);::EnableWindow(name_,!events&&valid);::EnableWindow(buttons_[4],events&&!events_.empty());
    std::wstring text;if(valid){if(events){const auto& e=events_[events_.size()-1-at];text=e.source+L" | "+ui::tr(e.kind)+L"\r\n"+ui::tr(e.detail);}else{::SetWindowTextW(name_,recent_[at].name.c_str());text=recent_[at].path+(recent_[at].related.empty()?L"":L"\r\n"+recent_[at].related)+L"\r\n"+ui::tr(L"Stage opens the existing tool only; no Apply, Connect, Play or TX.");}}::SetWindowTextW(details_,text.c_str());
}
BOOL MonitorCenter::OnCommand(WPARAM w,LPARAM l){const int id=LOWORD(w);if(id==10&&HIWORD(w)==CBN_SELCHANGE){rebuild();return TRUE;}if(id==25){ShowWindow(SW_HIDE);return TRUE;}if(id==11||id==26||(id>=20&&id<=24)){Request r;r.action=id==11?Flash:id==26?Add:static_cast<Action>(id-20);r.flag=::SendMessageW(flash_,BM_GETCHECK,0,0)==BST_CHECKED;const int at=ListView_GetNextItem(list_,-1,LVNI_SELECTED);if(r.action<=Remove){if(::SendMessageW(mode_,CB_GETCURSEL,0,0)!=0||at<0||static_cast<size_t>(at)>=recent_.size())return TRUE;r.resource=recent_[at];r.name=ui::control_value(name_);}GetOwner()->SendMessageW(kRequest,0,reinterpret_cast<LPARAM>(&r));return TRUE;}return CDialog::OnCommand(w,l);}
BOOL MonitorCenter::OnNotify(WPARAM w,LPARAM l,LRESULT* result){auto h=reinterpret_cast<NMHDR*>(l);if(h&&h->hwndFrom==list_&&h->code==LVN_ITEMCHANGED){selected();*result=0;return TRUE;}return CDialog::OnNotify(w,l,result);}
void MonitorCenter::layout(){if(!ready_)return;CRect r;GetClientRect(&r);const int row=std::max(28,control_font_height(mode_)+12);::MoveWindow(mode_,10,10,220,row*8,TRUE);::MoveWindow(flash_,240,10,std::max(100,r.Width()-250),row,TRUE);int x=10,y=std::max(row+30,r.Height()-row*5-60);const int list_bottom=y-10;::MoveWindow(name_,10,y,r.Width()-20,row,TRUE);y+=row+8;for(const auto h:buttons_){const int width=control_text_width(h,24);if(x+width>r.Width()-10){x=10;y+=row+8;}::MoveWindow(h,x,y,width,row,TRUE);x+=width+8;}::MoveWindow(list_,10,row+20,r.Width()-20,std::max(80,list_bottom-row-20),TRUE);y+=row+8;::MoveWindow(details_,10,y,r.Width()-20,std::max(row,r.Height()-y-10),TRUE);}
void MonitorCenter::OnSize(UINT t,int w,int h){CDialog::OnSize(t,w,h);if(t!=SIZE_MINIMIZED)layout();}
void MonitorCenter::OnGetMinMaxInfo(MINMAXINFO* i){CDialog::OnGetMinMaxInfo(i);i->ptMinTrackSize.x=760;i->ptMinTrackSize.y=600;}
LRESULT MonitorCenter::OnDpiChanged(WPARAM,LPARAM l){auto next=make_ui_font(m_hWnd);if(next){auto old=font_;font_=next;::EnumChildWindows(m_hWnd,[](HWND h,LPARAM f)->BOOL{::SendMessageW(h,WM_SETFONT,static_cast<WPARAM>(f),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font_));if(old)DeleteObject(old);}if(l){auto r=reinterpret_cast<RECT*>(l);SetWindowPos(nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout();return 0;}
void MonitorCenter::OnDestroy(){ready_=false;for(auto h=::GetWindow(m_hWnd,GW_CHILD);h;){auto next=::GetWindow(h,GW_HWNDNEXT);::DestroyWindow(h);h=next;}if(font_)DeleteObject(font_);font_=nullptr;CDialog::OnDestroy();}
