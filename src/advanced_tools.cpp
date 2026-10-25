#include "advanced_tools.h"
#include "resource.h"
#include "build_version.h"
#include "uart_files.h"
#include <afxdlgs.h>
#include <commctrl.h>
#include <algorithm>
#include "dpi_support.h"

namespace {
LRESULT CALLBACK viewport_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_COMMAND || message==WM_NOTIFY || message==PlotView::kCursorMoved) return ::SendMessageW(::GetParent(window),message,w,l);
    return DefWindowProcW(window,message,w,l);
}
std::wstring text(HWND window) { std::wstring result(static_cast<size_t>(::GetWindowTextLengthW(window))+1,0); ::GetWindowTextW(window,result.data(),static_cast<int>(result.size())); result.pop_back(); return result; }
const wchar_t* help[] = {
    L"Offline only: disconnect first. Load completed .bin + adjacent .idx for PC receipt timing; other files use the synthetic interval. Seek starts a new view at the selected 0-based record. No file is sent to UART.",
    L"Profiles save complete A/B workspaces: serial choices, parsers, pacing, framing, instruments, formulas, chart fields and display. Legacy profiles remain readable. No history, commands, paths or recording data. Load/Import only stages; Apply is explicit and requires idle state. Names: A-Z, a-z, 0-9, _ or - (1..48).",
    L"Retained capture snapshot: left-click sets A, right-click sets B. A/B and View end use capture seconds; values snap to nearest samples. Statistics use inclusive A..B, skipping blanks. Press Analyze to refresh; View end/Window navigate without changing live charts.",
    L"Explicit, finite automation only. One command per line; Periodic/Auto reply require one line. Reply/Trigger use the selected Text/HEX bytes, literal and case-sensitive. Checksum is appended before Ending. Stop/close/disconnect stops; reconnect never resumes.",
    L"Extraction after Include/Exclude. CSV/Tab: comma-separated column names. JSON: flat numeric fields. Regex: groups 1..8 for name/value/time (0 = receipt). Optional time field is seconds; raw clocks/bytes stay unchanged. Preview never applies. Apply starts a new capture.",
    L"Display only: raw RX and capture/export values stay unchanged. Choose/search a channel; set alias, unit, decimals (-1 = automatic), scale and offset. Apply saves immediately.",
    L"Explicit file TX over the current connection. Binary is unchanged; UTF-8 text has explicit newline conversion. One bounded pending chunk, pacing, pause/cancel. No retry or reconnect replay. Driver acceptance is not target ACK."
};
}
BEGIN_MESSAGE_MAP(AdvancedTools, CDialog)
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(PlotView::kCursorMoved, &AdvancedTools::OnPlotCursor)
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_DPICHANGED, &AdvancedTools::OnDpiChanged)
END_MESSAGE_MAP()
bool AdvancedTools::show(CWnd* owner, HFONT font, int page) {
    if (!GetSafeHwnd()) { font_ = font; if (!Create(IDD_ADVANCED_TOOLS,owner)) return false; }
    select(page); ShowWindow(SW_SHOW); if (IsIconic()) ShowWindow(SW_RESTORE); SetForegroundWindow(); return true;
}
BOOL AdvancedTools::OnInitDialog() {
    CDialog::OnInitDialog(); SetWindowTextW(L"Advanced UART tools | " CHART_VERSION_W);
    ModifyStyle(0,WS_CLIPCHILDREN|WS_VSCROLL);
    owned_font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    if (owned_font_) font_=owned_font_;
    WNDCLASSW viewport{};viewport.lpfnWndProc=viewport_proc;viewport.hInstance=GetModuleHandleW(nullptr);viewport.lpszClassName=L"SCC.AdvancedViewport";viewport.hbrBackground=GetSysColorBrush(COLOR_BTNFACE);RegisterClassW(&viewport);
    content_=::CreateWindowExW(WS_EX_CONTROLPARENT,viewport.lpszClassName,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,0,0,1,1,m_hWnd,nullptr,viewport.hInstance,nullptr);
    const auto make = [&](const wchar_t* type, const wchar_t* value, DWORD style, int id) { return ::CreateWindowW(type,value,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,0,0,1,1,id==100?m_hWnd:content_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr); };
    tabs_ = make(WC_TABCONTROLW,L"",WS_TABSTOP,100);
    const wchar_t* page_names[]={L"Replay",L"Profiles",L"Analysis",L"Automation",L"Extraction",L"Channels",L"File TX"};
    for (int p=0;p<PageCount;++p) { TCITEMW item{}; item.mask=TCIF_TEXT; item.pszText=const_cast<wchar_t*>(page_names[p]); TabCtrl_InsertItem(tabs_,p,&item); }
    help_ = make(L"STATIC",L"",SS_NOPREFIX,0); status_ = make(L"EDIT",L"",ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,101);
    fields_[Replay] = {{L"File / rotation segment",L""},{L"Synthetic interval (ms)",L"100"},{L"Speed (0.1..100 x)",L"1"},{L"Seek record (0-based)",L"0"},{L"Find bytes (from current record)",L""},{L"Find format",L"Text UTF-8",{L"Text UTF-8",L"HEX"}},{L"Seek time (s)",L"0"}};
    fields_[Profiles] = {{L"Named profile",L"default"}};
    fields_[Analysis] = {{L"Channel (type to search)",L"",{L""}},{L"Cursor A (s)",L"0"},{L"Cursor B (s)",L"1"},{L"View end (s)",L"1"},{L"Window (s)",L"30"},{L"Reference Y (empty = off)",L""}};
    fields_[Automation] = {{L"Mode",L"Periodic",{L"Periodic",L"Batch",L"Auto reply"}}, {L"Format",L"Text UTF-8",{L"Text UTF-8",L"HEX"}},
        {L"Ending",L"None",{L"None",L"CR",L"LF",L"CRLF"}}, {L"Checksum",L"None",{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus (LE)"}},
        {L"Interval / cooldown (ms)",L"1000"},{L"Cycles / maximum replies",L"1"},{L"Wait reply (empty = off)",L""},{L"Reply timeout (ms)",L"1000"},{L"Timeout retries (0..3)",L"0"},
        {L"Auto-reply trigger",L""},{L"Commands (one per line)",L"",{},true}};
    fields_[Extraction]={{L"Parser",L"Key/value",{L"Key/value",L"CSV",L"Tab",L"JSON",L"Regex groups"}},{L"Column names (comma separated)",L""},{L"Regex",L"([A-Za-z_][A-Za-z_0-9]*):([+-]?[0-9.]+)"},{L"Name group",L"1"},{L"Value group",L"2"},{L"Time group (0 = receipt)",L"0"},{L"Time field (empty = receipt)",L""},{L"Preview input",L"",{},true}};
    fields_[Channels]={{L"Channel (type to search)",L"",{L""}},{L"Alias",L""},{L"Unit",L""},{L"Decimals (-1 = automatic)",L"-1"},{L"Scale",L"1"},{L"Offset",L"0"}};
    fields_[FileTx]={{L"File",L""},{L"Mode",L"Binary",{L"Binary",L"UTF-8 text"}},{L"Text newline",L"Unchanged",{L"Unchanged",L"LF",L"CRLF",L"CR"}},{L"Chunk bytes (1..4096)",L"256"},{L"Delay between chunks (ms)",L"20"}};
    const std::array<std::vector<const wchar_t*>,PageCount> captions{{
        {L"Browse...",L"Load",L"Play",L"Pause",L"Seek",L"Unload / cancel",L"Find next",L"Seek time"},
        {L"Save named",L"Load named",L"Import...",L"Export staged...",L"Apply staged"},
        {L"Analyze / refresh",L"Clear cursors"}, {L"Start...",L"Stop"}, {L"Preview",L"Apply parser"}, {L"Load selected",L"Apply display",L"Reset selected"}, {L"Browse...",L"Start...",L"Pause",L"Resume",L"Cancel"}}};
    for (int p=0;p<PageCount;++p) {
        for (size_t i=0;i<fields_[p].size();++i) {
            auto& f = fields_[p][i]; f.label=make(L"STATIC",f.name.c_str(),SS_CENTERIMAGE|SS_NOPREFIX,0);
            const bool searchable=(p==Analysis||p==Channels)&&i==0;
            f.control=make(f.choices.empty()?L"EDIT":L"COMBOBOX",f.initial.c_str(),WS_TABSTOP | (f.choices.empty() ? (f.multiline?ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL:ES_AUTOHSCROLL) : (searchable?CBS_DROPDOWN|CBS_AUTOHSCROLL:CBS_DROPDOWNLIST)|WS_VSCROLL),2000+p*32+static_cast<int>(i));
            if (f.choices.empty()) ::SendMessageW(f.control,EM_SETLIMITTEXT,f.multiline?65536:1024,0);
            else { for (const auto& choice:f.choices) ::SendMessageW(f.control,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(choice.c_str())); ::SendMessageW(f.control,CB_SETCURSEL,0,0); }
        }
        for (size_t i=0;i<captions[p].size();++i) actions_[p].push_back(make(L"BUTTON",captions[p][i],BS_PUSHBUTTON|WS_TABSTOP,4000+p*16+static_cast<int>(i)));
    }
    analysis_plot_.create(content_,0,0,100,100,5000); analysis_plot_.set_model(&analysis_model_); analysis_plot_.set_overlay_enabled(true);
    for (HWND child=::GetWindow(m_hWnd,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT)) ::SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font_),TRUE);
    set_child_fonts(content_,font_);
    MONITORINFO monitor{sizeof(monitor)}; GetMonitorInfoW(MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTONEAREST),&monitor);
    SetWindowPos(nullptr,monitor.rcWork.left+20,monitor.rcWork.top+20,std::min(1100L,monitor.rcWork.right-monitor.rcWork.left-40),std::min(760L,monitor.rcWork.bottom-monitor.rcWork.top-40),SWP_NOZORDER);
    select(page_); return TRUE;
}
void AdvancedTools::select(int page) {
    if (selecting_) return; selecting_=true;
    page_=std::clamp(page,0,static_cast<int>(PageCount)-1); scroll_=0; ::SetFocus(tabs_); TabCtrl_SetCurSel(tabs_,page_); TabCtrl_SetCurFocus(tabs_,page_);
    for (int p=0;p<PageCount;++p) { for (auto& f:fields_[p]) { ::ShowWindow(f.label,p==page_?SW_SHOW:SW_HIDE); ::ShowWindow(f.control,p==page_?SW_SHOW:SW_HIDE); } for (auto button:actions_[p]) ::ShowWindow(button,p==page_?SW_SHOW:SW_HIDE); }
    ::ShowWindow(analysis_plot_.hwnd(),page_==Analysis?SW_SHOW:SW_HIDE); ::SetWindowTextW(help_,help[page_]);
    ::SetWindowTextW(status_,(states_[page_]+L"\r\n"+logs_[page_]).c_str()); layout(); selecting_=false;
    RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
}
void AdvancedTools::layout() {
    if (!tabs_) return; CRect client; GetClientRect(&client);
    HDC dc=::GetDC(m_hWnd); const auto old=SelectObject(dc,font_); TEXTMETRICW tm{}; GetTextMetricsW(dc,&tm);
    const int line=tm.tmHeight+8, gap=8; int label_width=0;
    for (const auto& f:fields_[page_]) { SIZE size{}; GetTextExtentPoint32W(dc,f.name.c_str(),static_cast<int>(f.name.size()),&size); label_width=std::max(label_width,static_cast<int>(size.cx)+16); }
    ::MoveWindow(tabs_,10,8,client.Width()-20,line+6,TRUE);
    const int viewport_height=std::max(40,client.Height()-line-18);
    ::MoveWindow(content_,0,line+18,client.Width(),viewport_height,TRUE);
    RECT paragraph{0,0,client.Width()-28,0}; DrawTextW(dc,help[page_],-1,&paragraph,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
    int y=4-scroll_; ::MoveWindow(help_,14,y,client.Width()-28,paragraph.bottom+8,TRUE); y+=paragraph.bottom+16;
    for (auto& f:fields_[page_]) {
        const int height=f.multiline?line*3:line;
        ::MoveWindow(f.label,14,y,label_width,line,TRUE); ::MoveWindow(f.control,14+label_width,y,client.Width()-28-label_width,f.choices.empty()?height:line+200,TRUE); y+=height+gap;
    }
    int x=14;
    for (const auto button:actions_[page_]) { const auto value=text(button); SIZE size{}; GetTextExtentPoint32W(dc,value.c_str(),static_cast<int>(value.size()),&size); const int width=static_cast<int>(size.cx)+24;
        if (x+width>client.Width()-14) { x=14; y+=line+gap; } ::MoveWindow(button,x,y,width,line,TRUE); x+=width+gap; }
    y+=line+12;
    if (page_==Analysis) { const int chart_height=std::max(160,viewport_height-(y+scroll_)-150); ::MoveWindow(analysis_plot_.hwnd(),14,y,client.Width()-28,chart_height,TRUE); y+=chart_height+8; }
    const int virtual_bottom=y+scroll_+110;
    scroll_limit_=std::max(0,virtual_bottom-viewport_height);
    if (scroll_>scroll_limit_) { scroll_=scroll_limit_; SelectObject(dc,old); ::ReleaseDC(m_hWnd,dc); layout(); return; }
    ::MoveWindow(status_,14,y,client.Width()-28,std::max(90,viewport_height-y-14),TRUE);
    SCROLLINFO bar{sizeof(bar),SIF_RANGE|SIF_PAGE|SIF_POS,0,virtual_bottom,static_cast<UINT>(viewport_height),scroll_,0}; SetScrollInfo(SB_VERT,&bar,TRUE);
    ::SetWindowPos(tabs_,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    SelectObject(dc,old); ::ReleaseDC(m_hWnd,dc);
}
void AdvancedTools::OnSize(UINT type,int cx,int cy) { CDialog::OnSize(type,cx,cy); layout(); }
void AdvancedTools::OnGetMinMaxInfo(MINMAXINFO* info) { CDialog::OnGetMinMaxInfo(info); info->ptMinTrackSize={700,480}; }
void AdvancedTools::OnVScroll(UINT code,UINT,CScrollBar*) {
    SCROLLINFO info{sizeof(info),SIF_ALL}; GetScrollInfo(SB_VERT,&info);
    const int step=40;
    if(code==SB_LINEUP) scroll_-=step; else if(code==SB_LINEDOWN) scroll_+=step;
    else if(code==SB_PAGEUP) scroll_-=static_cast<int>(info.nPage)*3/4; else if(code==SB_PAGEDOWN) scroll_+=static_cast<int>(info.nPage)*3/4;
    else if(code==SB_THUMBTRACK || code==SB_THUMBPOSITION) scroll_=info.nTrackPos;
    else if(code==SB_TOP) scroll_=0; else if(code==SB_BOTTOM) scroll_=scroll_limit_;
    scroll_=std::clamp(scroll_,0,scroll_limit_); layout(); RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}
BOOL AdvancedTools::OnMouseWheel(UINT,short delta,CPoint) { OnVScroll(delta>0?SB_LINEUP:SB_LINEDOWN,0,nullptr); return TRUE; }
LRESULT AdvancedTools::OnDpiChanged(WPARAM,LPARAM value) {
    const auto font=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    if (font) { const auto old=owned_font_; owned_font_=font; font_=font; set_child_fonts(m_hWnd,font);set_child_fonts(content_,font); if(old) DeleteObject(old); }
    if (value) { const auto& r=*reinterpret_cast<const RECT*>(value); SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE); }
    layout(); return 0;
}
void AdvancedTools::OnDestroy() { CDialog::OnDestroy(); if(owned_font_) { DeleteObject(owned_font_); owned_font_=nullptr; font_=nullptr; } }
BOOL AdvancedTools::OnNotify(WPARAM w,LPARAM l,LRESULT* result) { if (reinterpret_cast<NMHDR*>(l)->hwndFrom==tabs_ && reinterpret_cast<NMHDR*>(l)->code==TCN_SELCHANGE) { select(TabCtrl_GetCurSel(tabs_)); return TRUE; } return CDialog::OnNotify(w,l,result); }
BOOL AdvancedTools::OnCommand(WPARAM w,LPARAM l) {
    if(!searching_ && HIWORD(w)==CBN_EDITCHANGE) {
        for(const auto page:{Analysis,Channels}) if(reinterpret_cast<HWND>(l)==field_hwnd(page,0)) {
            auto& field=fields_[page][0];const auto window=field.control;const auto value=text(window);auto query=value;std::transform(query.begin(),query.end(),query.begin(),towlower);
            const auto selected=::SendMessageW(window,CB_GETEDITSEL,0,0);searching_=true;::SendMessageW(window,CB_RESETCONTENT,0,0);
            for(const auto& item:field.choices){auto name=item;std::transform(name.begin(),name.end(),name.begin(),towlower);if(name.find(query)!=name.npos) ::SendMessageW(window,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.c_str()));}
            ::SetWindowTextW(window,value.c_str());::SendMessageW(window,CB_SETEDITSEL,0,selected);searching_=false;return TRUE;
        }
    }
    const int id=LOWORD(w), p=(id-4000)/16, action=(id-4000)%16;
    if (id>=4000 && p<PageCount && action<static_cast<int>(actions_[p].size()) && HIWORD(w)==BN_CLICKED) {
        if ((p==Replay||p==FileTx) && action==0) { const auto window=m_hWnd; CFileDialog pick(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR,L"All files (*.*)|*.*||",this); if (pick.DoModal()==IDOK && ::IsWindow(window) && CWnd::FromHandlePermanent(window)==this) set_field(p,0,pick.GetPathName().GetString()); return TRUE; }
        Request request{p,action,{}}; for (size_t i=0;i<fields_[p].size();++i) request.fields.push_back(field(p,i));
        const auto owner=GetOwner()->GetSafeHwnd(); ::SendMessageW(owner,kRequest,0,reinterpret_cast<LPARAM>(&request)); return TRUE;
    }
    return CDialog::OnCommand(w,l);
}
BOOL AdvancedTools::PreTranslateMessage(MSG* message) {
    if (message && message->message==WM_KEYDOWN && GetKeyState(VK_CONTROL)<0 && message->wParam=='A') { wchar_t name[32]{}; ::GetClassNameW(message->hwnd,name,32); if (_wcsicmp(name,L"EDIT")==0) { ::SendMessageW(message->hwnd,EM_SETSEL,0,-1); return TRUE; } }
    const BOOL handled=CDialog::PreTranslateMessage(message);
    const auto focus=::GetFocus();
    if(message && message->message==WM_KEYDOWN && message->wParam==VK_TAB && focus && focus!=tabs_ && ::IsChild(m_hWnd,focus)) {
        RECT control{},client{},tab{};::GetWindowRect(focus,&control);::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&control),2);GetClientRect(&client);::GetWindowRect(tabs_,&tab);::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&tab),2);
        int delta=control.top<tab.bottom+8?control.top-tab.bottom-8:control.bottom>client.bottom-8?control.bottom-client.bottom+8:0;
        if(delta) {scroll_=std::clamp(scroll_+delta,0,scroll_limit_);layout();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);}
    }
    return handled;
}
void AdvancedTools::OnCancel() { Request request{-1,0,{}}; ::SendMessageW(GetOwner()->GetSafeHwnd(),kRequest,0,reinterpret_cast<LPARAM>(&request)); }
LRESULT AdvancedTools::OnPlotCursor(WPARAM second,LPARAM value) {
    if (page_!=Analysis || !value) return 0; wchar_t text_value[64]{}; swprintf_s(text_value,L"%.10g",*reinterpret_cast<const double*>(value));
    set_field(Analysis,second?2:1,text_value);Request request{Analysis,2,{}};for(size_t i=0;i<fields_[Analysis].size();++i) request.fields.push_back(field(Analysis,i));::SendMessageW(GetOwner()->GetSafeHwnd(),kRequest,0,reinterpret_cast<LPARAM>(&request));return 0;
}
HWND AdvancedTools::field_hwnd(int page,size_t i) const { return page>=0 && page<PageCount && i<fields_[page].size()?fields_[page][i].control:nullptr; }
HWND AdvancedTools::action_hwnd(int page,size_t i) const { return page>=0 && page<PageCount && i<actions_[page].size()?actions_[page][i]:nullptr; }
std::wstring AdvancedTools::field(int page,size_t i) const { const auto window=field_hwnd(page,i); return window?text(window):L""; }
void AdvancedTools::set_field(int page,size_t i,const std::wstring& value) { if (const auto window=field_hwnd(page,i)) ::SetWindowTextW(window,value.c_str()); }
void AdvancedTools::notice(int page,const std::wstring& value) { if (page<0 || page>=PageCount) return; auto& log=logs_[page]; log+=value+L"\r\n"; if (log.size()>24000) log.erase(0,log.size()-16000); if (page_==page && status_) ::SetWindowTextW(status_,(states_[page]+L"\r\n"+log).c_str()); }
void AdvancedTools::set_choices(int page,size_t index,const std::vector<std::wstring>& values) {const auto window=field_hwnd(page,index);if(!window) return;fields_[page][index].choices=values;const auto selected=text(window);searching_=true;::SendMessageW(window,CB_RESETCONTENT,0,0);for(const auto& value:values) ::SendMessageW(window,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value.c_str()));::SetWindowTextW(window,selected.c_str());searching_=false;}
void AdvancedTools::state(int page,const std::wstring& value) { if (states_[page]==value) return; states_[page]=value; if (page_==page && status_) ::SetWindowTextW(status_,(value+L"\r\n"+logs_[page]).c_str()); }
void AdvancedTools::show_analysis(const CaptureSnapshot& data,size_t column,double a,double b,double end,double window,bool reference,double threshold) {
    analysis_data_=data;
    analysis_model_.reset(); analysis_model_.set_time_window(window);
    for (const auto& row:data.rows) { double value=0; if (row->seconds>=end-window && row->seconds<=end && CaptureStore::value(*row,column,value)) analysis_model_.update_from_kv({{data.keys[column],value}},row->seconds); }
    analysis_plot_.reset_visual(); analysis_plot_.set_absolute_time_axis(true); analysis_plot_.set_time_window(window); analysis_plot_.update_from_model(end); analysis_plot_.set_analysis(a,b,reference,threshold);
}
