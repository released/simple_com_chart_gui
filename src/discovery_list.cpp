#include "discovery_list.h"
#include "resource.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include "dpi_support.h"
#include "discovery_geometry.h"
#include <algorithm>
#include <cwctype>
BEGIN_MESSAGE_MAP(DiscoveryListDialog,CDialog)
 ON_WM_SIZE()
 ON_WM_DESTROY()
 ON_WM_GETMINMAXINFO()
 ON_MESSAGE(WM_DPICHANGED,&DiscoveryListDialog::OnDpiChanged)
END_MESSAGE_MAP()
DiscoveryListDialog::DiscoveryListDialog(CWnd* owner,std::wstring title,std::vector<std::wstring> columns,Rows rows,std::wstring summary,std::wstring action,std::wstring alternate):CDialog(IDD_UART_TOOL_FORM,owner),title_(std::move(title)),summary_(std::move(summary)),action_(std::move(action)),alternate_caption_(std::move(alternate)),columns_(std::move(columns)),rows_(std::move(rows)){}
BOOL DiscoveryListDialog::OnInitDialog(){
    CDialog::OnInitDialog();ui::localize_dialog(m_hWnd);SetWindowTextW(title_.c_str());ModifyStyle(0,WS_CLIPCHILDREN);
    font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    const auto make=[&](const wchar_t* cls,const wchar_t* label,DWORD style,int id){return ui::create_window(cls,label,WS_CHILD|WS_VISIBLE|(wcscmp(cls,L"STATIC")?WS_TABSTOP:0)|style,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);};
    search_label_=make(L"STATIC",L"Search",SS_NOPREFIX|SS_CENTERIMAGE,105);
    search_=make(L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,100);::SendMessageW(search_,EM_SETLIMITTEXT,256,0);
    list_=make(WC_LISTVIEWW,L"",WS_BORDER|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,101);ListView_SetExtendedListViewStyle(list_,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);ListView_SetUnicodeFormat(list_,TRUE);
    for(size_t i=0;i<columns_.size();++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.cx=150;c.pszText=const_cast<wchar_t*>(ui::caption(columns_[i].c_str()));ListView_InsertColumn(list_,static_cast<int>(i),&c);}
    details_=make(L"EDIT",L"",WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,102);::SendMessageW(details_,EM_SETLIMITTEXT,65536,0);
    info_=make(L"STATIC",summary_.c_str(),SS_NOPREFIX,103);accept_=make(L"BUTTON",action_.c_str(),BS_DEFPUSHBUTTON,IDOK);
    if(!alternate_caption_.empty())other_=make(L"BUTTON",alternate_caption_.c_str(),BS_PUSHBUTTON,104);
    if(refresh_)refresh_button_=make(L"BUTTON",L"Refresh",BS_PUSHBUTTON,106);
    cancel_=make(L"BUTTON",L"Close",BS_PUSHBUTTON,IDCANCEL);if(action_==L"Close")::ShowWindow(accept_,SW_HIDE);set_child_fonts(m_hWnd,font_);ready_=true;
    const auto owner=GetOwner();const auto work=discovery_work_area(owner?owner->GetSafeHwnd():m_hWnd);
    const auto bounds=fit_discovery_bounds({work.left+60,work.top+60,work.left+1060,work.top+740},work);
    SetWindowPos(nullptr,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOZORDER);layout();rebuild();::SetFocus(search_);return FALSE;
}
void DiscoveryListDialog::rebuild(const std::wstring& selected_id){
    if(!ready_)return;auto query=ui::control_value(search_);std::transform(query.begin(),query.end(),query.begin(),towlower);visible_.clear();ListView_DeleteAllItems(list_);
    for(size_t r=0;r<rows_.size();++r){std::wstring hay;for(const auto& cell:rows_[r])hay+=cell+L" ";std::transform(hay.begin(),hay.end(),hay.begin(),towlower);if(!query.empty()&&hay.find(query)==hay.npos)continue;
        const int index=static_cast<int>(visible_.size());visible_.push_back(r);
        for(size_t c=0;c<columns_.size();++c){auto text=c<rows_[r].size()?rows_[r][c]:L"";for(auto& ch:text)if(ch==L'\r'||ch==L'\n'||ch==L'\t')ch=L' ';if(text.size()>512)text.resize(512);
            if(!c){LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=index;item.pszText=text.data();ListView_InsertItem(list_,&item);}else ListView_SetItemText(list_,index,static_cast<int>(c),text.data());}}
    ::EnableWindow(accept_,!visible_.empty());if(other_)::EnableWindow(other_,!visible_.empty());::SetWindowTextW(details_,L"");
    int selected=selected_id.empty()?0:-1;for(size_t i=0;i<visible_.size();++i)if(!selected_id.empty()&&!rows_[visible_[i]].empty()&&rows_[visible_[i]][0]==selected_id)selected=static_cast<int>(i);
    if(!visible_.empty()&&selected>=0){ListView_SetItemState(list_,selected,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}
    else {::EnableWindow(accept_,FALSE);if(other_)::EnableWindow(other_,FALSE);}
}
void DiscoveryListDialog::OnOK(){if(action_==L"Close"){CDialog::OnOK();return;}const int at=ListView_GetNextItem(list_,-1,LVNI_SELECTED);if(at<0||static_cast<size_t>(at)>=visible_.size())return;selected_=visible_[at];CDialog::OnOK();}
BOOL DiscoveryListDialog::OnCommand(WPARAM w,LPARAM l){
    if(LOWORD(w)==106&&refresh_){
        std::wstring selected;const int at=ListView_GetNextItem(list_,-1,LVNI_SELECTED);
        if(at>=0&&static_cast<size_t>(at)<visible_.size()&&!rows_[visible_[at]].empty())selected=rows_[visible_[at]][0];
        auto next=refresh_();rows_=std::move(next.first);summary_=std::move(next.second);::SetWindowTextW(info_,summary_.c_str());rebuild(selected);layout();return TRUE;
    }
    if(LOWORD(w)==100&&HIWORD(w)==EN_CHANGE){rebuild();return TRUE;}if(LOWORD(w)==104&&HIWORD(w)==BN_CLICKED){alternate_=true;OnOK();return TRUE;}return CDialog::OnCommand(w,l);
}
BOOL DiscoveryListDialog::OnNotify(WPARAM w,LPARAM l,LRESULT* out){const auto h=reinterpret_cast<NMHDR*>(l);if(h&&h->hwndFrom==list_){
    if(h->code==NM_DBLCLK){OnOK();*out=0;return TRUE;}
    if(h->code==LVN_ITEMCHANGED){const int at=ListView_GetNextItem(list_,-1,LVNI_SELECTED);const bool valid=at>=0&&static_cast<size_t>(at)<visible_.size();::EnableWindow(accept_,valid);if(other_)::EnableWindow(other_,valid);if(valid){std::wstring text;const auto& row=rows_[visible_[at]];for(size_t c=0;c<columns_.size();++c)text+=ui::tr(columns_[c])+L": "+(c<row.size()?row[c]:L"")+L"\r\n";::SetWindowTextW(details_,text.c_str());}*out=0;return TRUE;}}
    return CDialog::OnNotify(w,l,out);
}
void DiscoveryListDialog::layout(){if(!ready_)return;CRect r;GetClientRect(&r);const int line=std::max(28,control_font_height(accept_)+12),gap=10;
    const int label=std::max(60,control_text_width(search_label_,8));::MoveWindow(search_label_,gap,gap,label,line,TRUE);::MoveWindow(search_,gap+label+8,gap,r.Width()-2*gap-label-8,line,TRUE);const int info_height=std::max(line,control_wrapped_text_height(info_,r.Width()-2*gap));const int bottom=line*4+info_height+70;
    ::MoveWindow(list_,gap,line+20,r.Width()-2*gap,std::max(50,r.Height()-bottom-line-20),TRUE);
    const int available=r.Width()-2*gap;const auto dc=::GetDC(list_);const auto old=SelectObject(dc,font_);
    SIZE first{};if(!first_column_sample_.empty())GetTextExtentPoint32W(dc,first_column_sample_.c_str(),static_cast<int>(first_column_sample_.size()),&first);
    const int width=first.cx?(available-first.cx-24)/static_cast<int>((columns_.size()>1?columns_.size()-1:1)):available/static_cast<int>(std::max<size_t>(1,columns_.size()));
    for(size_t i=0;i<columns_.size();++i){const auto caption=ui::tr(columns_[i]);SIZE extent{};GetTextExtentPoint32W(dc,caption.c_str(),static_cast<int>(caption.size()),&extent);const int preferred=!i&&first.cx?first.cx+24:width;ListView_SetColumnWidth(list_,static_cast<int>(i),std::max(preferred,static_cast<int>(extent.cx)+24));}SelectObject(dc,old);::ReleaseDC(list_,dc);
    int y=std::max(line+80,r.Height()-bottom);::MoveWindow(details_,gap,y,r.Width()-2*gap,line*3,TRUE);y+=line*3+gap;
    ::MoveWindow(info_,gap,y,r.Width()-2*gap,info_height,TRUE);int x=gap;y=r.Height()-line-gap;
    for(auto h:{accept_,other_,refresh_button_,cancel_})if(h&&!(h==accept_&&action_==L"Close")){const int w=std::max(90,control_text_width(h,26));::MoveWindow(h,x,y,w,line,TRUE);x+=w+8;}
}
void DiscoveryListDialog::OnSize(UINT t,int w,int h){CDialog::OnSize(t,w,h);if(t!=SIZE_MINIMIZED)layout();}
void DiscoveryListDialog::OnGetMinMaxInfo(MINMAXINFO* i){CDialog::OnGetMinMaxInfo(i);const auto work=discovery_work_area(m_hWnd);i->ptMinTrackSize={std::max<LONG>(1,std::min<LONG>(800,work.right-work.left-16)),std::max<LONG>(1,std::min<LONG>(620,work.bottom-work.top-16))};}
LRESULT DiscoveryListDialog::OnDpiChanged(WPARAM,LPARAM l){const auto next=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);if(next){const auto old=font_;font_=next;set_child_fonts(m_hWnd,font_);if(old)DeleteObject(old);}if(l){const auto proposed=reinterpret_cast<RECT*>(l);const auto r=fit_discovery_bounds(*proposed,discovery_work_area(m_hWnd,proposed));SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);}layout();return 0;}
void DiscoveryListDialog::OnDestroy(){ready_=false;for(auto h=::GetWindow(m_hWnd,GW_CHILD);h;){const auto next=::GetWindow(h,GW_HWNDNEXT);::DestroyWindow(h);h=next;}CDialog::OnDestroy();if(font_){DeleteObject(font_);font_=nullptr;}}
