#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "uart_workbench.h"
#include "uart_files.h"
#include <deque>
#include <set>
#include <cmath>

namespace {
struct Wire {std::mutex mutex;std::vector<std::string> tx;std::string rx;bool partial=false;size_t count(){std::lock_guard<std::mutex> lock(mutex);return tx.size();}};
class ProtocolFake : public FakeSerialTransport {
public:
    ProtocolFake(std::shared_ptr<FakeSerialState> state,std::shared_ptr<Wire> wire):FakeSerialTransport(state),state_(std::move(state)),wire_(std::move(wire)){}
    size_t write_bytes(std::string_view bytes,std::wstring* error)override {
        {std::lock_guard<std::mutex> lock(wire_->mutex);wire_->tx.emplace_back(bytes);if(wire_->partial){wire_->partial=false;if(error)*error=L"Injected partial write";return 1;}}
        if(bytes.size()!=8)return bytes.size();const auto fc=static_cast<unsigned char>(bytes[1]);const unsigned count=(static_cast<unsigned char>(bytes[4])<<8)|static_cast<unsigned char>(bytes[5]);
        if(fc<1||fc>4||!count)return bytes.size();std::string response;response+=bytes[0];response+=bytes[1];
        if(fc<=2){response+=static_cast<char>((count+7)/8);for(unsigned n=0;n<(count+7)/8;++n)response+='\x55';}
        else{response+=static_cast<char>(count*2);for(unsigned n=0;n<count;++n){response+='\0';response+=static_cast<char>(201+n);}}
        const auto crc=frame_checksum(response,FrameChecksum::Modbus16);response+=static_cast<char>(crc);response+=static_cast<char>(crc>>8);
        {std::lock_guard<std::mutex> lock(wire_->mutex);wire_->rx+=response;}{std::lock_guard<std::mutex> lock(state_->mutex);state_->raw+=response;}return bytes.size();
    }
private:std::shared_ptr<FakeSerialState> state_;std::shared_ptr<Wire> wire_;
};
thread_local std::function<void(HWND)> modal_visit;
thread_local bool modal_done=false,modal_failed=false;
thread_local unsigned modal_wait=0;
VOID CALLBACK visit_modal(HWND h,UINT,UINT_PTR id,DWORD){KillTimer(h,id);if(modal_visit&&!IsWindowVisible(h)){if(++modal_wait<125)SetTimer(h,id,40,visit_modal);else{modal_failed=true;::PostMessageW(h,WM_COMMAND,IDCANCEL,0);}return;}try{if(modal_visit){auto action=std::move(modal_visit);modal_visit={};RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);action(h);modal_done=true;}}catch(...){modal_failed=true;::PostMessageW(h,WM_COMMAND,IDCANCEL,0);}}
LRESULT CALLBACK modal_hook(int code,WPARAM w,LPARAM l){if(code==HCBT_ACTIVATE&&modal_visit){const auto h=reinterpret_cast<HWND>(w);DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId()&&::GetDlgItem(h,4000))SetTimer(h,31911,30,visit_modal);}return CallNextHookEx(nullptr,code,w,l);}
struct ModalVisit {HHOOK hook;explicit ModalVisit(std::function<void(HWND)> f){modal_visit=std::move(f);modal_done=modal_failed=false;modal_wait=0;hook=SetWindowsHookExW(WH_CBT,modal_hook,nullptr,GetCurrentThreadId());if(!hook)throw std::runtime_error("Fixture dialog hook failed");}~ModalVisit(){UnhookWindowsHookEx(hook);modal_visit={};}bool done()const{return modal_done&&!modal_failed;}};
void edit(HWND h,const wchar_t* value){::SendMessageW(h,EM_SETSEL,0,-1);::SendMessageW(h,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(value));}
bool inside(HWND parent,HWND child){RECT bounds{},r{};GetClientRect(parent,&bounds);GetWindowRect(child,&r);MapWindowPoints(nullptr,parent,reinterpret_cast<POINT*>(&r),2);return IsWindowVisible(child)&&r.left>=0&&r.top>=0&&r.right<=bounds.right+1&&r.bottom<=bounds.bottom+1&&r.bottom>r.top;}
bool capture(HWND window,const std::filesystem::path& path){
    // Let native child paints settle inside the owned private desktop only.
    const auto until=GetTickCount64()+40;
    do{MSG message{};unsigned count=0;while(count++<64&&PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(1);}while(GetTickCount64()<until);
    RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    RECT bounds{};GetClientRect(window,&bounds);const int width=bounds.right,height=bounds.bottom;
    HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,width,height);const auto old=SelectObject(memory,bitmap);
    const bool copied=BitBlt(memory,0,0,width,height,dc,0,0,SRCCOPY)!=FALSE;GdiFlush();SelectObject(memory,old);
    UINT count=0,size=0;Gdiplus::GetImageEncodersSize(&count,&size);std::vector<unsigned char> buffer(size);const auto encoders=reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());Gdiplus::GetImageEncoders(count,size,encoders);
    bool saved=false;auto resident=path;resident.replace_filename(path.stem().wstring()+L"-resident.png");
    for(UINT n=0;n<count;++n)if(wcscmp(encoders[n].MimeType,L"image/png")==0){Gdiplus::Bitmap image(bitmap,nullptr);saved=image.Save(resident.c_str(),&encoders[n].Clsid)==Gdiplus::Ok;break;}
    DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
    return copied&&saved&&save_runtime_frame(window,path);
}
}
bool CMainDialog::verify_protocol_workspaces() {
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"protocols-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto done){const auto until=GetTickCount64()+10000;do{MSG m{};unsigned n=0;while(n++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(done())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto tab=[&](int page){TabCtrl_SetCurFocus(tabs_,page);::SendMessageW(tabs_,WM_KEYDOWN,VK_SPACE,0);::SendMessageW(tabs_,WM_KEYUP,VK_SPACE,0);return TabCtrl_GetCurSel(tabs_)==page;};
    const auto click=[](ProtocolPage& page,unsigned id){::SendMessageW(page.control(id),BM_CLICK,0,0);};
    auto a=std::make_shared<FakeSerialState>(),b=std::make_shared<FakeSerialState>();auto aw=std::make_shared<Wire>(),bw=std::make_shared<Wire>();
    const SerialPortInfo pa{L"COM181",L"Protocol fixture A",L"PROTOCOL_A",L"fake://protocol-a"},pb{L"COM182",L"Protocol fixture B",L"PROTOCOL_B",L"fake://protocol-b"};
    const auto old_scanner=port_scanner_;const auto old_factory=transport_factory_;const auto old_notice=uart_notice_;const auto old_connection_notice=connection_notice_;
    port_scanner_=[pa,pb](bool* ok){if(ok)*ok=true;return std::vector<SerialPortInfo>{pa,pb};};transport_factory_=[a,aw]{return std::make_unique<ProtocolFake>(a,aw);};uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};connection_notice_=uart_notice_;
    try {
        const auto language_files=std::filesystem::path(__FILE__).parent_path()/L"i18n";
        bool catalogs_match=true;const std::atomic<bool> no_cancel{false};
        for(const auto leaf:{L"lang_en.ini",L"lang_tw.ini"})catalogs_match=catalogs_match&&uart_read_file(folder/leaf,2*1024*1024,no_cancel)==uart_read_file(language_files/leaf,2*1024*1024,no_cancel);
        check("runtime catalogs exactly match the bound source inputs",catalogs_match);
        check("five real native tabs preserve original three indices",TabCtrl_GetItemCount(tabs_)==5&&kMessagesTab==0&&kChartTab==1&&kDataTab==2);
        check("production pages start empty and no adapter is fabricated",modbus_points_[0].empty()&&!lin_frames_&&!lin_transport_&&a->attempt_count()==0);
        ShowWindow(SW_RESTORE);SetWindowPos(nullptr,0,0,1120,900,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        for(int page=0;page<5;++page){check("native button tab activation selects requested page",tab(page));
            check("tab pages have exclusive expected visibility",(::IsWindowVisible(message_page_.hwnd())!=FALSE)==(page==0)&&(::IsWindowVisible(plot_view_.hwnd())!=FALSE)==(page==1)&&(::IsWindowVisible(data_page_.hwnd())!=FALSE)==(page==2)&&(modbus_page_.IsWindowVisible()!=FALSE)==(page==3)&&(lin_page_.IsWindowVisible()!=FALSE)==(page==4));}
        check("tab navigation never connects or transmits",a->attempt_count()==0&&aw->count()==0);
        check("new page containers retain child clipping",(::GetWindowLongPtrW(modbus_page_.GetSafeHwnd(),GWL_STYLE)&WS_CLIPCHILDREN)&&(::GetWindowLongPtrW(lin_page_.GetSafeHwnd(),GWL_STYLE)&WS_CLIPCHILDREN));
        tab(kModbusTab);
        {ModalVisit visit([&](HWND h){const auto viewport=::GetDlgItem(h,4000);edit(::GetDlgItem(viewport,4100),L"speed");edit(::GetDlgItem(viewport,4101),L"1");::SendMessageW(::GetDlgItem(viewport,4102),CB_SETCURSEL,2,0);::SendMessageW(::GetDlgItem(viewport,4103),CB_SETCURSEL,1,0);edit(::GetDlgItem(viewport,4104),L"40001");edit(::GetDlgItem(viewport,4105),L"1");::SendMessageW(::GetDlgItem(viewport,4106),CB_SETCURSEL,2,0);edit(::GetDlgItem(viewport,4111),L"100");edit(::GetDlgItem(viewport,4112),L"500");::SendMessageW(h,WM_COMMAND,4500,0);
            check("actual editor preview resolves reference address without TX",modbus_points_[0].empty()&&aw->count()==0&&a->attempt_count()==0);
            RECT area{},button{};::GetClientRect(h,&area);::GetWindowRect(::GetDlgItem(h,IDOK),&button);::MapWindowPoints(nullptr,h,reinterpret_cast<POINT*>(&button),2);report<<"BOUNDS: editor client="<<area.right<<","<<area.bottom<<" accept="<<button.left<<","<<button.top<<","<<button.right<<","<<button.bottom<<" visible="<<::IsWindowVisible(::GetDlgItem(h,IDOK))<<'\n';
            check("actual editor is captured",capture(h,folder/L"protocol-modbus-editor.png"));check("editor accept button fits owner work area",inside(h,::GetDlgItem(h,IDOK)));::SendMessageW(h,WM_COMMAND,IDOK,0);});click(modbus_page_,2);check("actual native editor commits validated definition only",visit.done()&&modbus_points_[0].size()==1&&modbus_points_[0][0].request.address==0&&modbus_points_[0][0].name=="speed");}
        ModbusPoint bit;bit.name="flag";bit.type="bit";bit.request.function=1;bit.request.unit=2;bit.request.period_ms=100;bit.request.timeout_ms=500;modbus_points_[0].push_back(bit);refresh_modbus_page();
        check("definitions do not arm polling",!workbench_->rtu_[0].active()&&aw->count()==0);
        SerialSettings settings;settings.port=pa;begin_serial_open(settings,false);check("fake primary opens before Workbench HWND exists",pump([&]{return serial_running_.load();})&&!workbench_->GetSafeHwnd());
        std::wstring recording_error;RecordingOptions recording;recording.base=folder/L"protocol-rx";check("owned continuous recorder starts",recorder_.start(recording));
        click(modbus_page_,7);
        check("headless shared owner starts one multi-block Modbus plan",workbench_->rtu_[0].active()&&workbench_->rtu_[0].plan_size()==2&&!workbench_->GetSafeHwnd());
        check("actual FC03 and FC01 fake responses feed existing charts",pump([&]{double x=0,y=0;return model_.latest_value("A.RTU.u1.f3.speed",x)&&x==201&&model_.latest_value("A.RTU.u2.f1.flag",y)&&y==1;}));
        check("dedicated page and Workbench share the same TX lease",workbench_->tx_reserved(0)&&!workbench_->start_rtu(0,RtuConfig{},recording_error));
        refresh_modbus_page();check("Modbus rows show current observed values",modbus_page_.rows().size()==2&&modbus_page_.rows()[0][8]==L"201");
        RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("actual Modbus page captured",capture(m_hWnd,folder/L"protocol-modbus-page.png"));
        click(modbus_page_,8);check("native Stop releases protocol ownership",!workbench_->rtu_[0].active()&&!workbench_->tx_reserved(0));
        check("queued I/O drains after Stop",pump([&]{return !uart_operations_.busy();}));const auto stopped=aw->count();
        const auto wait_until=GetTickCount64()+180;check("stopped poller never restarts from later ticks",pump([&]{return GetTickCount64()>=wait_until;})&&aw->count()==stopped);
        refresh_modbus_page();check("last good values are visibly stale after Stop",modbus_page_.rows()[0][10]==ui::tr(L"Stale; last successful value"));
        check("all accepted fake response bytes reach raw history",pump([&]{std::lock_guard<std::mutex> lock(aw->mutex);return primary_traffic_->snapshot().rx==aw->rx.size();}));recorder_.stop();check("raw recording drains after display/plan changes",pump([&]{return !recorder_.status().stopping;}));
        {std::lock_guard<std::mutex> lock(aw->mutex);check("continuous raw recording is exact and independent of chart resets",recorder_.status().bytes==aw->rx.size()&&!recorder_.status().failed);}
        show_workbench();workbench_->secondary_.set_factory([b,bw]{return std::make_unique<ProtocolFake>(b,bw);});::SendMessageW(workbench_->groups_[1].port,CB_SETCURSEL,1,0);::SendMessageW(workbench_->groups_[1].connect,BM_CLICK,0,0);
        check("existing second COM connection remains independent",pump([&]{return workbench_->secondary_.connected();}));workbench_->ShowWindow(SW_HIDE);
        ModbusPoint second;second.name="input";second.request.function=4;second.request.period_ms=100;modbus_points_[1]={second};::SendMessageW(modbus_page_.control(50),CB_SETCURSEL,1,0);::SendMessageW(modbus_page_.GetSafeHwnd(),WM_COMMAND,MAKEWPARAM(50,CBN_SELCHANGE),reinterpret_cast<LPARAM>(modbus_page_.control(50)));click(modbus_page_,7);
        check("Port B dedicated page uses existing B receive/model pipeline",pump([&]{double value=0;return model_.latest_value("B.RTU.input",value)&&value==201;}));click(modbus_page_,8);
        disconnect();workbench_->secondary_.disconnect();check("both fake ports stop asynchronously",pump([&]{return !disconnect_pending_&&!serial_mgr_->is_connected()&&!workbench_->secondary_.reserved();}));
        {
            UartWorkbench bounded;auto traffic=std::make_shared<TrafficStore>();bounded.initialize((folder/L"bounded-rtu.ini").wstring(),traffic);
            RtuConfig config;std::string detail,request;bounded.rtu_[0].start(config,123,0,detail);bounded.rtu_[0].next(2,request);bounded.rtu_[0].complete(true,3);
            std::string response("\x01\x03\x02\x00\xC9",5);const auto crc=frame_checksum(response,FrameChecksum::Modbus16);response+=static_cast<char>(crc);response+=static_cast<char>(crc>>8);bounded.rtu_[0].receive(response,4,1);
            for(unsigned n=0;n<256;++n){BinaryFrame queued;queued.bytes="x";bounded.decoded_.push_back({1,std::move(queued)});}bounded.decoded_bytes_=256;
            bounded.poll_rtu(0,5);
            check("RTU queue eviction reports the affected source loss",bounded.decoded_.size()==256&&bounded.numeric_losses()[1]==1);
            check("RTU decoded frames retain original connection generation",bounded.decoded_.back().port==0&&bounded.decoded_.back().frame.generation==123);
            bounded.rtu_[0].stop("Fixture finished");
        }
        tab(kLinTab);LinFrame frame;frame.id=0x10;frame.pid=0x50;frame.data=std::string("\x05\x00",2);frame.checksum_type=LinChecksum::Enhanced;frame.checksum=static_cast<int>(lin_checksum(frame.id,frame.data,frame.checksum_type));
        auto unknown=frame;unknown.seconds=0.1;unknown.pid=unknown.checksum=-1;auto invalid=frame;invalid.seconds=0.2;invalid.checksum^=1;
        lin_frames_=std::make_shared<const std::vector<LinFrame>>(std::vector<LinFrame>{frame,unknown,invalid});LinSignal signal;signal.name="position";signal.id=0x10;signal.length=8;signal.scale=0.5;signal.unit="mm";lin_signals_={signal};refresh_lin_page();
        check("unknown and invalid metadata are distinct in actual native rows",lin_page_.rows().size()==3&&lin_page_.rows()[1][5]==ui::tr(L"Unknown")&&lin_page_.rows()[2][6]==ui::tr(L"Invalid"));
        const auto raw_before=message_page_.buffer().bytes();const bool apply_ready=pump([&]{return ::IsWindowEnabled(lin_page_.control(6))!=FALSE;});check("staged LIN source enables native Apply after owner state update",apply_ready);if(!apply_ready)throw std::runtime_error("LIN Apply unavailable");click(lin_page_,6);
        check("LIN applies verified values through bounded native chart drain",pump([&]{return lin_chart_frames_&&lin_chart_position_==3;})&&capture_.rows().size()==1&&capture_.rows().front()->numeric_source==2&&lin_chart_skipped_==2);
        check("LIN source clock and raw Messages remain separate",lin_offline_chart_&&message_page_.buffer().bytes()==raw_before&&std::abs(now_seconds()-1)<0.001);
        if(!lin_offline_chart_)throw std::runtime_error("LIN source was not applied; connection countercheck cannot run");
        poll_workbench();::SendMessageW(btn_connect_,BM_CLICK,0,0);::SendMessageW(workbench_->groups_[0].connect,BM_CLICK,0,0);
        check("main and Workbench Port A connection routes reject applied offline LIN",a->attempt_count()==1&&lin_offline_chart_&&capture_.rows().size()==1);
        const auto choices=channel_panel_.get_checkbox_state_map();check("new LIN signals retain explicit unchecked chart default",choices.count("LIN.10.position")&&!choices.at("LIN.10.position"));
        check("LIN values preserve conversion and source identity",!capture_.rows().empty()&&capture_.keys()==std::vector<std::string>{"LIN.10.position"}&&capture_.rows().front()->cells[0].second==2.5);
        ::SendMessageW(lin_page_.control(51),BM_SETCHECK,BST_CHECKED,0);click(lin_page_,6);check("explicit unknown permission admits unknown but never invalid frames",pump([&]{return lin_chart_position_==3;})&&capture_.rows().size()==2&&lin_chart_skipped_==1);
        const auto path=folder/L"immutable-lin.csv";check("LIN trace snapshot export starts",begin_protocol_export(path,true,true));click(lin_page_,8);
        check("export remains immutable after Clear",pump([&]{return !exporting_;})&&decode_lin_trace(uart_read_file(path,2*1024*1024,std::atomic<bool>{false})).size()==3&&!lin_frames_);
        const auto trace=encode_lin_trace({frame,unknown,invalid});const std::atomic<bool> cancel{false};uart_write_file(folder/L"lin-input.csv",trace,false,cancel);
        begin_protocol_file(folder/L"lin-input.csv",1);check("background trace import stages without changing capture",pump([&]{return !protocol_file_.busy();})&&lin_frames_&&lin_frames_->size()==3);
        const auto retained=lin_frames_;uart_write_file(folder/L"invalid.csv","invalid",false,cancel);begin_protocol_file(folder/L"invalid.csv",1);
        check("malformed file preserves last good source",pump([&]{return !protocol_file_.busy();})&&lin_frames_==retained);
        begin_protocol_file(folder/L"lin-input.csv",1);++protocol_epoch_;check("late-generation file result is discarded",pump([&]{return !protocol_file_.busy();})&&lin_frames_==retained);
        begin_protocol_file(folder/L"lin-input.csv",1);protocol_file_.cancel();check("cancel retains owner until finished and preserves source",pump([&]{return !protocol_file_.busy();})&&lin_frames_==retained);
        const std::string ldf="LIN_description_file; LIN_protocol_version=\"2.2\"; LIN_language_version=\"2.2\"; LIN_speed=19.2 kbps; Nodes { Master: Host,5 ms,0.1 ms; Slaves: Sensor; } Signals { position:8,0,Sensor,Host; } Frames { State:0x10,Sensor,2 { position,0; } } Schedule_tables { Normal { State delay 10 ms; } }";
        uart_write_file(folder/L"signals.ldf",ldf,false,cancel);begin_protocol_file(folder/L"signals.ldf",3);check("actual LDF file job stages signals and schedule",pump([&]{return !protocol_file_.busy();})&&lin_signals_.size()==1&&lin_database_.schedules.size()==1);
        {ModalVisit visit([&](HWND h){
            const auto viewport=::GetDlgItem(h,4000),cycles=::GetDlgItem(viewport,4101);RECT rect{};::GetWindowRect(cycles,&rect);::MapWindowPoints(nullptr,viewport,reinterpret_cast<POINT*>(&rect),2);const POINT center{(rect.left+rect.right)/2,(rect.top+rect.bottom)/2};
            report<<"BOUNDS: schedule cycles="<<rect.left<<","<<rect.top<<","<<rect.right<<","<<rect.bottom<<" visible="<<::IsWindowVisible(cycles)<<" direct_hit="<<::GetDlgCtrlID(::ChildWindowFromPointEx(viewport,center,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED))<<" style="<<std::hex<<::GetWindowLongPtrW(cycles,GWL_STYLE)<<std::dec<<'\n';
            check("schedule cycle input is visible and unobstructed",inside(viewport,cycles)&&::ChildWindowFromPointEx(viewport,center,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED)==cycles);
            check("hardware Start is unavailable without an adapter",!::IsWindowEnabled(::GetDlgItem(h,IDOK)));::SendMessageW(h,WM_COMMAND,4500,0);::RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("schedule preview remains available and captured",capture(h,folder/L"protocol-lin-schedule.png"));::SendMessageW(h,WM_COMMAND,IDOK,0);check("disabled Start cannot be bypassed with IDOK",::IsWindow(h));::PostMessageW(h,WM_COMMAND,IDCANCEL,0);});click(lin_page_,9);check("actual schedule dialog never transmits or arms a missing adapter",visit.done()&&!lin_scheduler_.active()&&aw->count()==stopped);}
        RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("actual LIN page captured",capture(m_hWnd,folder/L"protocol-lin-page.png"));
        SetWindowPos(nullptr,0,0,820,650,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);::SetFocus(lin_page_.control(11));lin_page_.reveal_focus();check("compact LIN toolbar can reveal keyboard focus",inside(lin_page_.GetSafeHwnd(),lin_page_.control(11)));
        SetWindowPos(nullptr,0,0,1120,900,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);tab(kChartTab);check("original Chart remains usable after protocol pages",::IsWindowVisible(plot_view_.hwnd())&&!lin_page_.IsWindowVisible());
        check("all operations used injected transports only",a->attempt_count()==1&&b->attempt_count()==1);
    }catch(const std::exception& e){report<<"FAIL: exception "<<e.what()<<'\n';pass=false;}
    if(workbench_){workbench_->stop_tool(0);workbench_->stop_tool(1);workbench_->secondary_.disconnect();}disconnect();protocol_file_.cancel();
    check("fixture workers finish before owner teardown",pump([&]{return !disconnect_pending_&&!protocol_file_.busy()&&(!workbench_||!workbench_->secondary_.reserved());}));
    port_scanner_=old_scanner;transport_factory_=old_factory;uart_notice_=old_notice;connection_notice_=old_connection_notice;return pass;
}
