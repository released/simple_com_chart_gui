#include "uart_workbench.h"
#include "dpi_support.h"
#include "resource.h"
#include "uart_files.h"
#include "capture_export.h"
#include "capture_filename.h"
#include "button_feedback.h"
#include <commctrl.h>
#include <cwchar>
#include <sstream>

namespace {
std::wstring text(HWND h){std::wstring out(static_cast<size_t>(::GetWindowTextLengthW(h))+1,0);::GetWindowTextW(h,out.data(),static_cast<int>(out.size()));out.pop_back();return out;}
int pick(HWND h){return static_cast<int>(::SendMessageW(h,CB_GETCURSEL,0,0));}
int number(HWND h){const auto value=text(h);if(value.empty()||value.size()>9||value.find_first_not_of(L"0123456789")!=std::wstring::npos)return 0;try{return std::stoi(value);}catch(...){return 0;}}
std::wstring filename_stem(){SYSTEMTIME now{};GetLocalTime(&now);auto name=capture_filename(now);name.resize(name.size()-4);return name;}
std::wstring stamp(int64_t ms){FILETIME ft{};const uint64_t time=static_cast<uint64_t>(std::max<int64_t>(0,ms))*10000+116444736000000000ULL;ft.dwLowDateTime=static_cast<DWORD>(time);ft.dwHighDateTime=static_cast<DWORD>(time>>32);SYSTEMTIME utc{},local{};FileTimeToSystemTime(&ft,&utc);SystemTimeToTzSpecificLocalTime(nullptr,&utc,&local);wchar_t out[40]{};swprintf_s(out,L"%02u:%02u:%02u.%03u",local.wHour,local.wMinute,local.wSecond,local.wMilliseconds);return out;}
std::wstring packet_text(const std::deque<TrafficRecord>& packets,bool full=false){
    std::wstring out;int64_t previous=0;
    for(const auto& r:packets){out+=r.tx?L"TX ":L"RX ";out+=stamp(r.receipt_ms)+L"  dt="+std::to_wstring(previous?std::max<int64_t>(0,r.receipt_ms-previous):0)+L"ms  len="+std::to_wstring(r.bytes.size())+L"  ";previous=r.receipt_ms;
        const auto n=full?r.bytes.size():std::min<size_t>(256,r.bytes.size());out+=uart_wide(uart_hex(std::string_view(r.bytes).substr(0,n)))+L"  |";
        for(size_t i=0;i<n;++i){const unsigned char c=r.bytes[i];out.push_back(c>=32&&c<127?c:L'.');}out+=L"|";if(n<r.bytes.size())out+=L" ... (retained bytes available in raw view/recording)";out+=L"\r\n";
    }return out;
}
}
LRESULT UartPortGroup::WindowProc(UINT message,WPARAM w,LPARAM l){
    if(message==WM_SETFONT){font_=reinterpret_cast<HFONT>(w);if(l)Invalidate();return 0;}
    if(message==WM_GETFONT)return reinterpret_cast<LRESULT>(font_);
    if(message==WM_COMMAND||message==WM_DRAWITEM||(message>=WM_CTLCOLORMSGBOX&&message<=WM_CTLCOLORSTATIC))return ::SendMessageW(::GetParent(m_hWnd),message,w,l);
    if(message==WM_ERASEBKGND){RECT r{};::GetClientRect(m_hWnd,&r);::FillRect(reinterpret_cast<HDC>(w),&r,GetSysColorBrush(COLOR_3DFACE));return TRUE;}
    if(message==WM_PAINT){CPaintDC dc(this);CRect bounds;GetClientRect(&bounds);const auto font=reinterpret_cast<HFONT>(SendMessage(WM_GETFONT));const auto previous=dc.SelectObject(CFont::FromHandle(font?font:reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))));TEXTMETRICW metric{};dc.GetTextMetricsW(&metric);CRect border=bounds;border.top+=metric.tmHeight/2;dc.DrawEdge(border,EDGE_ETCHED,BF_RECT);CString caption;GetWindowTextW(caption);CRect title(9,0,std::max(10,bounds.Width()-9),metric.tmHeight+3);dc.SetBkColor(GetSysColor(COLOR_3DFACE));dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));dc.DrawTextW(caption,&title,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX);dc.SelectObject(previous);return 0;}
    return CWnd::WindowProc(message,w,l);
}
BEGIN_MESSAGE_MAP(UartWorkbench,CDialog)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_GETMINMAXINFO()
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_DRAWITEM()
    ON_MESSAGE(WM_DPICHANGED,&UartWorkbench::OnDpiChanged)
END_MESSAGE_MAP()
bool UartWorkbench::show(CWnd* owner,const std::wstring& ini,HFONT font,std::shared_ptr<TrafficStore> primary){
    if(!GetSafeHwnd()){initialize(ini,std::move(primary));font_=font;if(!Create(IDD_UART_WORKBENCH,owner))return false;}
    ShowWindow(SW_SHOW);if(IsIconic())ShowWindow(SW_RESTORE);SetForegroundWindow();return true;
}
HWND UartWorkbench::child(const wchar_t* cls,const wchar_t* caption,DWORD style,int control){
    return ::CreateWindowExW(0,cls,caption,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,0,0,1,1,building_parent_?building_parent_:m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(control)),nullptr,nullptr);
}
HWND UartWorkbench::combo(size_t group,int control,std::initializer_list<const wchar_t*> choices,bool editable){
    auto h=child(L"COMBOBOX",L"",WS_TABSTOP|WS_VSCROLL|(editable?CBS_DROPDOWN|CBS_AUTOHSCROLL:CBS_DROPDOWNLIST),id(group,control));
    for(const auto value:choices)::SendMessageW(h,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(value));
    if(choices.size())::SendMessageW(h,CB_SETCURSEL,0,0);return h;
}
void UartWorkbench::build_group(size_t i){
    auto& g=groups_[i];
    const auto label=[&](const wchar_t* s){return child(L"STATIC",s,SS_CENTERIMAGE|SS_NOPREFIX);};
    const auto button=[&](const wchar_t* s,int c,bool check=false){return child(L"BUTTON",s,WS_TABSTOP|(check?BS_AUTOCHECKBOX:c==Connect?BS_OWNERDRAW:BS_PUSHBUTTON),id(i,c));};
    const auto cls=AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW,LoadCursorW(nullptr,IDC_ARROW),GetSysColorBrush(COLOR_3DFACE));
    g.panel.CreateEx(WS_EX_CONTROLPARENT,cls,i==0?L"Port A - Main connection / Chart":L"Port B - Independent monitor",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CRect(0,0,1,1),this,3000+static_cast<UINT>(i));g.box=g.panel.GetSafeHwnd();building_parent_=g.box;
    g.port_label=label(L"COM:");g.port=combo(i,Port,{});g.scan=button(L"Scan",Scan);g.connect=button(L"Connect",Connect);
    g.baud_label=label(L"Baud:");g.baud=combo(i,Baud,{L"115200",L"9600",L"19200",L"38400",L"57600",L"230400",L"460800",L"921600"},true);
    g.data_label=label(L"Data:");g.data=combo(i,Data,{L"8",L"7",L"6",L"5"});
    g.parity_label=label(L"Parity:");g.parity=combo(i,Parity,{L"NONE",L"ODD",L"EVEN"});
    g.stop_label=label(L"Stop:");g.stop=combo(i,Stop,{L"1",L"1.5",L"2"});
    g.view=combo(i,View,{L"ASCII",L"HEX",L"RX/TX inspect",L"Terminal (VT100)"});g.follow=button(L"Auto-scroll",Follow,true);::SendMessageW(g.follow,BM_SETCHECK,BST_CHECKED,0);
    g.clear=button(L"Clear",Clear);g.save=button(L"Save As...",Save);g.record=button(L"Record RX...",Record);
    g.input=child(L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,id(i,Input));::SendMessageW(g.input,EM_SETLIMITTEXT,16384,0);
    g.hex=button(L"HEX TX",Hex,true);g.send=button(L"Send",Send);
    g.log=child(L"EDIT",L"",WS_TABSTOP|WS_BORDER|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|ES_AUTOHSCROLL);::SendMessageW(g.log,EM_SETLIMITTEXT,2*1024*1024,0);
    g.status=label(L"Disconnected | RX 0 | TX 0 | retained display only");
    building_parent_=nullptr;
}
BOOL UartWorkbench::OnInitDialog(){
    CDialog::OnInitDialog();owned_font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);if(owned_font_)font_=owned_font_;
    if(!font_)font_=reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));black_=CreateSolidBrush(RGB(0,0,0));
    LOGFONTW lf{};GetObjectW(font_,sizeof(lf),&lf);wcscpy_s(lf.lfFaceName,L"Consolas");log_font_=CreateFontIndirectW(&lf);
    hint_=child(L"STATIC",L"Separate A/B logs. Tools: framing, binary parser, commands, transfers, RTU and displays. PC receipt times are not wire timing.",SS_CENTERIMAGE|SS_NOPREFIX);
    build_group(0);build_group(1);
    ::EnumChildWindows(m_hWnd,[](HWND h,LPARAM font)->BOOL{::SendMessageW(h,WM_SETFONT,static_cast<WPARAM>(font),FALSE);return TRUE;},reinterpret_cast<LPARAM>(font_));
    for(auto& g:groups_)::SendMessageW(g.log,WM_SETFONT,reinterpret_cast<WPARAM>(log_font_?log_font_:font_),FALSE);
    const auto saved=GetPrivateProfileIntW(L"DualCOM",L"SplitPercent",50,ini_.c_str());ratio_=std::clamp(saved/100.0,0.25,0.75);
    CMenu menu;if(menu.LoadMenuW(IDR_WORKBENCH_MENU)){SetMenu(&menu);menu.Detach();}
    ready_=true;scan();load_b();load_views();set_settings(0,primary_status_.settings);SetTimer(2,1000,nullptr);
    CRect area;SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);const int w=std::min<int>(1260,area.Width()-24),h=std::min<int>(900,area.Height()-30);SetWindowPos(nullptr,area.left+12,area.top+12,w,h,SWP_NOZORDER);layout();return TRUE;
}
void UartWorkbench::scan(){bool ok=false;const auto fresh=scanner_(&ok);if(!ok)return;if(fresh.size()==ports_.size()&&std::equal(fresh.begin(),fresh.end(),ports_.begin(),[](const auto& a,const auto& b){return a.device==b.device&&a.description==b.description&&a.instance_id==b.instance_id&&a.open_path==b.open_path;}))return;SerialPortInfo hint[2];for(size_t i=0;i<2;++i){const auto index=pick(groups_[i].port);if(index>=0&&index<static_cast<int>(ports_.size()))hint[i]=ports_[index];}ports_=fresh;fill_ports(0,primary_status_.active?primary_status_.settings.port:hint[0]);fill_ports(1,secondary_.reserved()?secondary_.settings().port:hint[1]);}
void UartWorkbench::fill_ports(size_t i,const SerialPortInfo& hint){
    auto h=groups_[i].port;::SendMessageW(h,CB_RESETCONTENT,0,0);for(const auto& p:ports_){const auto s=p.device+L" - "+p.description;::SendMessageW(h,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));}
    int index=find_serial_port(ports_,hint);if(hint.device.empty()&&hint.instance_id.empty()&&!ports_.empty())index=0;::SendMessageW(h,CB_SETCURSEL,index,0);
}
void UartWorkbench::set_settings(size_t i,const SerialSettings& s){auto& g=groups_[i];fill_ports(i,s.port);::SetWindowTextW(g.baud,std::to_wstring(s.baud).c_str());::SendMessageW(g.data,CB_SETCURSEL,8-s.data_bits,0);::SendMessageW(g.parity,CB_SETCURSEL,s.parity,0);::SendMessageW(g.stop,CB_SETCURSEL,s.stop_bits,0);}
SerialSettings UartWorkbench::chosen(size_t i,std::wstring& error)const{
    SerialSettings s;const auto& g=groups_[i];const auto index=pick(g.port);if(index<0||index>=static_cast<int>(ports_.size())){error=L"Select an available COM port.";return s;}
    s.port=ports_[index];s.baud=number(g.baud);s.data_bits=8-pick(g.data);s.parity=pick(g.parity);s.stop_bits=pick(g.stop);if(i==0)s.controls=primary_status_.settings.controls;
    if(!valid_serial_settings(s))error=L"Invalid baud/data/parity/stop settings.";return s;
}
void UartWorkbench::update_primary(const Primary& p){
    const auto& old=primary_status_.settings;const bool changed=primary_status_.active!=p.active||old.port.device!=p.settings.port.device||old.baud!=p.settings.baud||old.data_bits!=p.settings.data_bits||old.parity!=p.settings.parity||old.stop_bits!=p.settings.stop_bits;
    primary_status_=p;if(ready_&&changed)set_settings(0,p.settings);
}
bool UartWorkbench::secondary_owns(const SerialPortInfo& p)const{return secondary_.reserved()&&serial_ports_conflict(secondary_.settings().port,p);}
LRESULT UartWorkbench::request(Request value){value.generation=primary_status_.generation;value.was_active=primary_status_.active;const auto owner=GetOwner();if(owner&&::IsWindow(owner->GetSafeHwnd()))return ::SendMessageW(owner->GetSafeHwnd(),kRequest,0,reinterpret_cast<LPARAM>(&value));return 0;}
void UartWorkbench::warn(const std::wstring& value){const auto notify=notice_;notify(m_hWnd,value.c_str(),L"UART Workbench",MB_OK|MB_ICONWARNING);}
void UartWorkbench::load_b(){
    wchar_t port[32]{},instance[1024]{};GetPrivateProfileStringW(L"DualCOM.B",L"Port",L"",port,32,ini_.c_str());GetPrivateProfileStringW(L"DualCOM.B",L"InstanceId",L"",instance,1024,ini_.c_str());SerialSettings s;s.port.device=port;s.port.instance_id=instance;
    s.baud=GetPrivateProfileIntW(L"DualCOM.B",L"Baud",115200,ini_.c_str());s.data_bits=GetPrivateProfileIntW(L"DualCOM.B",L"Data",8,ini_.c_str());s.parity=GetPrivateProfileIntW(L"DualCOM.B",L"Parity",0,ini_.c_str());s.stop_bits=GetPrivateProfileIntW(L"DualCOM.B",L"Stop",0,ini_.c_str());
    if(!valid_serial_settings(s)){s=SerialSettings{};s.port.device=port;s.port.instance_id=instance;}set_settings(1,s);
}
void UartWorkbench::persist_b(){const auto& s=secondary_.settings();const auto key=s.port.device+L"|"+s.port.instance_id+L"|"+std::to_wstring(s.baud)+L"|"+std::to_wstring(s.data_bits)+L"|"+std::to_wstring(s.parity)+L"|"+std::to_wstring(s.stop_bits);if(saved_b_==key)return;
    const auto put=[&](const wchar_t* key,const std::wstring& value){return WritePrivateProfileStringW(L"DualCOM.B",key,value.c_str(),ini_.c_str())!=FALSE;};
    if(put(L"Port",s.port.device)&&put(L"InstanceId",s.port.instance_id)&&put(L"Baud",std::to_wstring(s.baud))&&put(L"Data",std::to_wstring(s.data_bits))&&put(L"Parity",std::to_wstring(s.parity))&&put(L"Stop",std::to_wstring(s.stop_bits)))saved_b_=key;else groups_[1].save_error=L"Cannot save B settings; use a writable tool folder.";
}
void UartWorkbench::OnTimer(UINT_PTR timer){
    if(!ready_)return;
    if(timer==2){if(!shutdown_)scan();return;}
}
void UartWorkbench::tick(){
    const auto* reserve=primary_status_.active?&primary_status_.settings.port:nullptr;secondary_.poll(ports_,reserve);if(secondary_.connected()){persist_b();if(!had_b_)set_settings(1,secondary_.settings());}had_b_=secondary_.connected();
    for(size_t i=0;i<2;++i){int saved=0;std::wstring error;if(save_jobs_[i].take(saved,error))groups_[i].save_error=error.empty()?L"Saved.":error;}
    if(auto connection_error=secondary_.take_error();!connection_error.empty()&&!shutdown_){warn(connection_error+L"\r\n\r\nCheck the COM selection, USB/driver and baud settings. Disconnect this port in Tera Term or another application before retrying.");if(!GetSafeHwnd())return;}
    for(const auto& result:secondary_.results())operation_result(1,result);poll_tools();commands_.poll();signals_.poll();
    decode_binary();decode_text();if(ready_&&IsWindowVisible()){refresh_group(0);refresh_group(1);}
}
void UartWorkbench::refresh_group(size_t i){
    auto& g=groups_[i];auto& store=i==0?*primary_:secondary_.traffic;const auto batch=store.after(g.sequence,32);
    if(g.revision!=batch.revision){g.raw.reset();g.terminal.reset();g.framer.reset();g.packets.clear();g.packet_bytes=0;g.sequence=0;g.revision=batch.revision;g.dirty=true;}
    // Re-read after a reset so the old sequence cannot skip fresh records.
    const auto actual=g.sequence==0?store.after(0,32):batch;
    if(g.sequence&&!actual.records.empty()&&actual.records.front().sequence>g.sequence+1){g.raw.reset();g.terminal.reset();g.framer.reset();g.packets.clear();g.packet_bytes=0;g.dirty=true;}
    const auto append=[&](std::vector<TrafficRecord> frames){for(auto& frame:frames){g.packet_bytes+=frame.bytes.size();const auto at=std::upper_bound(g.packets.begin(),g.packets.end(),frame.sample_seconds,[](double time,const TrafficRecord& r){return time<r.sample_seconds;});g.packets.insert(at,std::move(frame));g.dirty=true;while(g.packet_bytes>256*1024||g.packets.size()>512){g.packet_bytes-=g.packets.front().bytes.size();g.packets.pop_front();}}};
    for(const auto& r:actual.records){if(!r.tx){g.raw.append(r.bytes,0,r.receipt_ms);if(g.terminal_generation!=r.generation){g.terminal.reset();g.terminal_generation=r.generation;}g.terminal.feed(r.bytes);}append(g.framer.feed(r));g.sequence=r.sequence;g.dirty=true;}append(g.framer.drain(traffic_monotonic_seconds()));
    if(g.dirty)update_view(i);
    const bool active=i==0?primary_status_.active:secondary_.reserved();const bool connected=i==0?primary_status_.connected:secondary_.connected();
    const auto enable=[](HWND h,bool value){if((::IsWindowEnabled(h)!=FALSE)!=value)::EnableWindow(h,value);};
    const auto caption=[](HWND h,const std::wstring& value){if(text(h)!=value)::SetWindowTextW(h,value.c_str());};
    for(auto h:{g.port,g.scan,g.baud,g.data,g.parity,g.stop})enable(h,!active&&!shutdown_);
    enable(g.connect,!shutdown_);caption(g.connect,active?L"Disconnect":L"Connect");enable(g.send,connected&&!shutdown_&&!tx_reserved(i));enable(g.save,!save_jobs_[i].busy());
    const auto rec=i==0?primary_status_.recording:secondary_.recorder.status();caption(g.record,rec.active?L"Stop record":L"Record RX...");
    const auto binary=binary_[i].enabled?L" | binary "+std::to_wstring(binary_[i].frames)+L" / CRC errors "+std::to_wstring(binary_[i].decoder.checksum_errors())+L" / lost "+std::to_wstring(binary_[i].lost):L"";
    const auto operation=tx_reserved(i)?runs_[i].status.substr(0,runs_[i].status.find(L"\r\n"))+L" | ":L"";
    const auto textStatus=i==1&&text_b_.config().enabled?L" | text B skipped "+std::to_wstring(text_b_.skipped())+L" / gaps "+std::to_wstring(text_lost_):L"";
    const auto framing=textStatus+L" | framing "+std::to_wstring(g.framer.config().mode)+L" / discarded "+std::to_wstring(g.framer.discarded())+L" B";
    const auto status=operation+(g.save_error.empty()?L"":g.save_error+L" | ")+rec.detail+L" | "+(i==0?primary_status_.status:secondary_.status())+L" | RX "+std::to_wstring(actual.rx)+L" | TX "+std::to_wstring(actual.tx)+L" | history expired "+std::to_wstring(actual.expired)+L" B"+(g.mode==2?framing:L"")+binary;caption(g.status,status);
}
void UartWorkbench::update_view(size_t i){auto& g=groups_[i];const auto view=g.mode==0?g.raw.ascii():g.mode==1?g.raw.hex():g.mode==3?g.terminal.text():packet_text(g.packets);const int first=static_cast<int>(::SendMessageW(g.log,EM_GETFIRSTVISIBLELINE,0,0));DWORD start=0,end=0;::SendMessageW(g.log,EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));
    if(view==g.rendered){g.dirty=false;return;}
    size_t common=0;while(common<g.rendered.size()&&common<view.size()&&g.rendered[common]==view[common])++common;
    ::SendMessageW(g.log,WM_SETREDRAW,FALSE,0);::SendMessageW(g.log,EM_SETSEL,common,g.rendered.size());::SendMessageW(g.log,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(view.c_str()+common));g.rendered=view;if(::SendMessageW(g.follow,BM_GETCHECK,0,0)==BST_CHECKED){::SendMessageW(g.log,EM_SETSEL,view.size(),view.size());::SendMessageW(g.log,EM_SCROLLCARET,0,0);}else{::SendMessageW(g.log,EM_SETSEL,start,end);::SendMessageW(g.log,EM_LINESCROLL,0,first-static_cast<int>(::SendMessageW(g.log,EM_GETFIRSTVISIBLELINE,0,0)));}::SendMessageW(g.log,WM_SETREDRAW,TRUE,0);::InvalidateRect(g.log,nullptr,FALSE);g.dirty=false;
}
std::string UartWorkbench::export_text(size_t i)const{
    const auto& g=groups_[i];const auto wide=g.mode==2?packet_text(g.packets,true):g.mode==3?g.terminal.text():g.mode==1?g.raw.hex():g.raw.ascii();return message_utf8(wide);
}
bool UartWorkbench::begin_save(size_t i,const std::filesystem::path& path){if(save_jobs_[i].busy())return false;auto data=export_text(i);if(data.empty())return false;groups_[i].save_error.clear();return save_jobs_[i].start([data=std::move(data),path](const std::atomic<bool>&){export_message_text(data,path);return 1;});}
void UartWorkbench::save_group(size_t i){if(save_jobs_[i].busy())return;if(export_text(i).empty()){warn(L"This port has no retained data to save.");return;}
    const auto name=filename_stem()+L"_"+(i==0?L"A":L"B")+L".log";CFileDialog dialog(FALSE,L"log",name.c_str(),OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,L"Log (*.log)|*.log|Text (*.txt)|*.txt||",this);if(dialog.DoModal()!=IDOK)return;const std::filesystem::path path(dialog.GetPathName().GetString());begin_save(i,path);
}
void UartWorkbench::record_group(size_t i){if(i==0){Request r;r.action=Request::RecordA;request(std::move(r));return;}if(secondary_.recorder.status().active){secondary_.recorder.stop();return;}const auto name=filename_stem()+L"_B";CFileDialog dialog(FALSE,L"log",name.c_str(),OFN_HIDEREADONLY,L"Recording prefix (*.log)|*.log||",this);if(dialog.DoModal()!=IDOK)return;RecordingOptions options;options.base=std::filesystem::path(dialog.GetPathName().GetString());options.base.replace_extension();if(!secondary_.recorder.start(options))warn(L"Cannot start B recording; check the destination and existing prefix.");}
BOOL UartWorkbench::OnCommand(WPARAM w,LPARAM l){const int control=LOWORD(w);if(ui_locked_)return TRUE;if(control==ID_WORKBENCH_FORWARD){forward_dialog();return TRUE;}if(control==ID_WORKBENCH_SIGNALS){signals_.show(this,ini_);return TRUE;}if(control==ID_WORKBENCH_INSPECT_A||control==ID_WORKBENCH_INSPECT_B){inspect_dialog(control==ID_WORKBENCH_INSPECT_B?1:0);return TRUE;}if(control==ID_WORKBENCH_TEXT_B){text_dialog();return TRUE;}if(control==ID_WORKBENCH_FRAMING_A||control==ID_WORKBENCH_FRAMING_B){framing_dialog(control==ID_WORKBENCH_FRAMING_B?1:0);return TRUE;}if(control==ID_WORKBENCH_INSTRUMENTS){instruments_.show(this,ini_);return TRUE;}if(control==ID_WORKBENCH_RTU_A||control==ID_WORKBENCH_RTU_B){rtu_dialog(control==ID_WORKBENCH_RTU_B?1:0);return TRUE;}if(control==ID_WORKBENCH_TERMINAL_A||control==ID_WORKBENCH_TERMINAL_B){terminal_dialog(control==ID_WORKBENCH_TERMINAL_B?1:0);return TRUE;}if(control==ID_WORKBENCH_TRANSFER_A||control==ID_WORKBENCH_TRANSFER_B){transfer_dialog(control==ID_WORKBENCH_TRANSFER_B?1:0);return TRUE;}if(control==ID_WORKBENCH_STOP_A||control==ID_WORKBENCH_STOP_B){stop_tool(control==ID_WORKBENCH_STOP_B?1:0);return TRUE;}if(control==ID_WORKBENCH_COMMANDS){show_commands();return TRUE;}if(control==ID_WORKBENCH_PACING_A||control==ID_WORKBENCH_PACING_B){pacing_dialog(control==ID_WORKBENCH_PACING_B?1:0);return TRUE;}if(control==ID_WORKBENCH_BINARY_A||control==ID_WORKBENCH_BINARY_B){binary_dialog(control==ID_WORKBENCH_BINARY_A?0:1);return TRUE;}if(control>=2000&&control<2200){const size_t i=control>=2100?1:0;const int action=control-2000-static_cast<int>(i)*100;auto& g=groups_[i];
    if(action==View&&HIWORD(w)==CBN_SELCHANGE){g.mode=pick(g.view);persist_view(i);g.dirty=true;update_view(i);return TRUE;}if(HIWORD(w)!=BN_CLICKED)return CDialog::OnCommand(w,l);
    if(action==Scan){scan();return TRUE;}if(action==Clear){const auto snapshot=(i==0?*primary_:secondary_.traffic).after(UINT64_MAX,1);g.raw.reset();g.terminal.reset();g.framer.reset();g.packets.clear();g.packet_bytes=0;g.sequence=snapshot.newest;g.revision=snapshot.revision;g.dirty=true;update_view(i);return TRUE;}if(action==Follow){persist_view(i);g.dirty=true;update_view(i);return TRUE;}if(action==Save){save_group(i);return TRUE;}if(action==Record){record_group(i);return TRUE;}
    if(action==Connect){if(i==0){Request r;r.action=Request::ConnectA;if(!primary_status_.active){std::wstring e;r.settings=chosen(0,e);if(!e.empty()){warn(e);return TRUE;}}request(std::move(r));}else if(secondary_.reserved())secondary_.disconnect();else{Request allowed;allowed.action=Request::CheckLive;if(!request(allowed)){warn(L"Finish profile/offline work before opening Port B.");return TRUE;}Request refresh;refresh.action=Request::RefreshA;request(std::move(refresh));std::wstring e;auto settings=chosen(1,e);if(e.empty()&&primary_status_.active&&serial_ports_conflict(settings.port,primary_status_.settings.port))e=L"Port A already owns this COM device. Select a different port for B.";if(e.empty())secondary_.connect(settings,e);if(!e.empty())warn(e);}return TRUE;}
    if(action==Send){if(tx_reserved(i)){warn(L"Stop the active tool operation before manual TX.");return TRUE;}TxEntry tx{text(g.input),::SendMessageW(g.hex,BM_GETCHECK,0,0)==BST_CHECKED,TxEnding::None};std::string bytes;std::wstring error;if(!make_tx_payload(tx,bytes,error)){warn(error);return TRUE;}if(i==0){Request r;r.action=Request::SendA;r.tx=std::move(tx);request(std::move(r));}else if(!secondary_.send(std::move(bytes),error))warn(error);return TRUE;}
    }return CDialog::OnCommand(w,l);
}
void UartWorkbench::shutdown(){if(shutdown_)return;shutdown_=true;const auto rec=secondary_.recorder.status();close_recording_=rec.active||rec.stopping;commands_.shutdown();signals_.shutdown();stop_tool(0);stop_tool(1);secondary_.disconnect();secondary_.recorder.stop();}
std::wstring UartWorkbench::close_recording_error(){if(!close_recording_||!shutdown_complete())return {};close_recording_=false;const auto rec=secondary_.recorder.status();return rec.lost||rec.detail.find(L"ERROR")!=std::wstring::npos?L"Port B recording did not finish cleanly. Partial files were preserved.\r\n\r\n"+rec.detail+L"\r\n\r\nReview the files before closing.":L"";}
bool UartWorkbench::shutdown_complete()const{const auto rec=secondary_.recorder.status();return secondary_.idle()&&!rec.active&&!rec.stopping&&!save_jobs_[0].busy()&&!save_jobs_[1].busy()&&!commands_.busy()&&!signals_.busy()&&!transfers_[0].busy()&&!transfers_[1].busy();}
HBRUSH UartWorkbench::OnCtlColor(CDC* dc,CWnd* wnd,UINT type){if(wnd&&(wnd->GetSafeHwnd()==groups_[0].log||wnd->GetSafeHwnd()==groups_[1].log)){dc->SetTextColor(RGB(0,255,0));dc->SetBkColor(RGB(0,0,0));return black_;}if(type==CTLCOLOR_STATIC||type==CTLCOLOR_BTN||type==CTLCOLOR_DLG){dc->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));dc->SetBkColor(GetSysColor(COLOR_3DFACE));return GetSysColorBrush(COLOR_3DFACE);}return CDialog::OnCtlColor(dc,wnd,type);}
BOOL UartWorkbench::OnEraseBkgnd(CDC* dc){CRect bounds;GetClientRect(&bounds);dc->FillSolidRect(bounds,GetSysColor(COLOR_3DFACE));return TRUE;}
void UartWorkbench::OnDrawItem(int id,LPDRAWITEMSTRUCT item){if(!item||(id!=this->id(0,Connect)&&id!=this->id(1,Connect))){CDialog::OnDrawItem(id,item);return;}const size_t i=id==this->id(1,Connect)?1:0;CDC dc;dc.Attach(item->hDC);CRect r=item->rcItem;const auto colors=button_colors((item->itemState&ODS_SELECTED)!=0,i==0?primary_status_.active:secondary_.reserved(),(item->itemState&ODS_DISABLED)!=0,false);dc.FillSolidRect(r,colors.background);dc.DrawEdge(r,(item->itemState&ODS_SELECTED)?EDGE_SUNKEN:EDGE_RAISED,BF_RECT);const auto old=dc.SelectObject(CFont::FromHandle(font_));dc.SetTextColor(colors.text);dc.SetBkMode(TRANSPARENT);const auto caption=text(item->hwndItem);dc.DrawTextW(caption.c_str(),static_cast<int>(caption.size()),&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);if(item->itemState&ODS_FOCUS){r.DeflateRect(3,3);dc.DrawFocusRect(r);}dc.SelectObject(old);dc.Detach();}
void UartWorkbench::OnDestroy(){ready_=false;KillTimer(1);KillTimer(2);secondary_.disconnect();if(commands_.GetSafeHwnd())commands_.DestroyWindow();if(instruments_.GetSafeHwnd())instruments_.DestroyWindow();if(signals_.GetSafeHwnd())signals_.DestroyWindow();for(auto h=::GetWindow(m_hWnd,GW_CHILD);h;){const auto next=::GetWindow(h,GW_HWNDNEXT);::DestroyWindow(h);h=next;}if(black_){DeleteObject(black_);black_=nullptr;}CDialog::OnDestroy();if(log_font_){DeleteObject(log_font_);log_font_=nullptr;}if(owned_font_){DeleteObject(owned_font_);owned_font_=nullptr;}}
