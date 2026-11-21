#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "json_tree_dialog.h"
#include "binary_inspector.h"
#include "runtime_capture.h"
#include "runtime_serial_fixture.h"
#include <deque>
#include <set>
#include "uart_files.h"
namespace {
std::wstring fd_exception_path;
LONG WINAPI fd_exception(EXCEPTION_POINTERS* exception){
    const auto file=CreateFileW(fd_exception_path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file!=INVALID_HANDLE_VALUE){
        char line[256]{};DWORD written=0;const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto write=[&](const char* label,uintptr_t value){const int n=_snprintf_s(line,sizeof(line),_TRUNCATE,"%s %llx\r\n",label,static_cast<unsigned long long>(value));if(n>0)WriteFile(file,line,n,&written,nullptr);};
        write("exception",exception->ExceptionRecord->ExceptionCode);write("module",base);write("rip-rva",exception->ContextRecord->Rip-base);write("rsp",exception->ContextRecord->Rsp);
        write("thread",GetCurrentThreadId());
        for(DWORD i=0;i<exception->ExceptionRecord->NumberParameters&&i<EXCEPTION_MAXIMUM_PARAMETERS;++i)write("exception-parameter",exception->ExceptionRecord->ExceptionInformation[i]);
        void* frames[48]{};const auto count=CaptureStackBackTrace(0,48,frames,nullptr);for(USHORT i=0;i<count;++i)write("frame-rva",reinterpret_cast<uintptr_t>(frames[i])-base);
        CloseHandle(file);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
struct FdDiagnosticScope {
    LPTOP_LEVEL_EXCEPTION_FILTER previous=nullptr;
    bool enabled=false;
    void start(){enabled=std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--fd-diagnostics")!=std::wstring::npos;if(enabled)previous=SetUnhandledExceptionFilter(fd_exception);}
    ~FdDiagnosticScope(){if(enabled)SetUnhandledExceptionFilter(previous);}
};
thread_local std::deque<std::function<void(HWND)>> callbacks;
thread_local bool callback_failed=false;
thread_local std::set<HWND> visited;
bool fd_dialog(HWND h){wchar_t cls[80]{};GetClassNameW(::GetDlgItem(h,101),cls,80);return ::GetDlgItem(h,4000)||std::wstring(cls)==WC_LISTVIEWW||::GetDlgItem(h,15)||::FindWindowExW(h,nullptr,WC_TREEVIEWW,nullptr);}
VOID CALLBACK visit_fd(HWND h,UINT,UINT_PTR id,DWORD){KillTimer(h,id);if(callbacks.empty())return;if(!::IsWindowVisible(h)){SetTimer(h,id,40,visit_fd);return;}::RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);auto call=std::move(callbacks.front());callbacks.pop_front();try{call(h);}catch(...){callback_failed=true;if(IsWindow(h))::PostMessageW(h,WM_COMMAND,IDCANCEL,0);}}
LRESULT CALLBACK fd_hook(int code,WPARAM w,LPARAM l){if(code==HCBT_DESTROYWND)visited.erase(reinterpret_cast<HWND>(w));if(code==HCBT_ACTIVATE&&!callbacks.empty()){auto h=reinterpret_cast<HWND>(w);DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId()&&fd_dialog(h)&&visited.insert(h).second)SetTimer(h,31987,50,visit_fd);}return CallNextHookEx(nullptr,code,w,l);}
struct Visit {HHOOK hook=nullptr;explicit Visit(std::deque<std::function<void(HWND)>> work){callbacks=std::move(work);callback_failed=false;visited.clear();hook=SetWindowsHookExW(WH_CBT,fd_hook,nullptr,GetCurrentThreadId());if(!hook)throw std::runtime_error("Owned dialog hook failed.");}~Visit(){UnhookWindowsHookEx(hook);callbacks.clear();}bool done()const{return callbacks.empty()&&!callback_failed;}};
bool fits(HWND parent,HWND child){RECT a{},b{};GetClientRect(parent,&a);GetWindowRect(child,&b);MapWindowPoints(nullptr,parent,reinterpret_cast<POINT*>(&b),2);return IsWindowVisible(child)&&b.left>=0&&b.top>=0&&b.right<=a.right&&b.bottom<=a.bottom;}
std::thread failed_launch(std::function<void()>){throw std::runtime_error("Injected start failure.");}
struct FdIo {std::atomic<unsigned> writes{0},controls{0},breaks{0};};
class FdTransport:public FakeSerialTransport {
public:FdTransport(std::shared_ptr<FakeSerialState> state,std::shared_ptr<FdIo> io):FakeSerialTransport(std::move(state)),io_(std::move(io)){}
    size_t write_bytes(std::string_view bytes,std::wstring*)override{++io_->writes;return bytes.size();}
    bool set_controls(const SerialControls&,std::wstring*)override{++io_->controls;return true;}
    bool send_break(std::wstring*)override{++io_->breaks;return true;}
private:std::shared_ptr<FdIo> io_;
};
}
bool CMainDialog::verify_monitor_expansion(){
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"monitor-fd-result.txt");report<<std::unitbuf;bool pass=true;
    fd_exception_path=(folder/L"fd-exception.txt").wstring();
    // Test-process lifetime only. Default runs retain the normal exception path
    // and stack reservation; optional diagnostics must not change stack size.
    static FdDiagnosticScope diagnostics;diagnostics.start();
    report<<"Exit diagnostics: "<<(diagnostics.enabled?"enabled":"normal exception path")<<"; stack guarantee unchanged\n";
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+10000;do{MSG m{};unsigned count=0;while(count++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto saved_scanner=port_scanner_;const auto saved_factory=transport_factory_;const auto saved_notice=uart_notice_;auto state=std::make_shared<FakeSerialState>();auto bstate=std::make_shared<FakeSerialState>();auto io=std::make_shared<FdIo>();
    const SerialPortInfo port{L"COM195",L"FD fake A",L"FD_A",L"fake://fd-a"},bport{L"COM196",L"FD fake B",L"FD_B",L"fake://fd-b"};
    port_scanner_=[port,bport](bool* ok){if(ok)*ok=true;return std::vector<SerialPortInfo>{port,bport};};transport_factory_=[state,io]{return std::make_unique<FdTransport>(state,io);};uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};
    try{
        const auto identity=L"Fixture."+std::to_wstring(std::hash<std::wstring>{}(folder.wstring()));DWORD owner_pid=0;
        ::GetWindowThreadProcessId(::FindWindowW(L"SimpleComChart.SessionActivation.v1",identity.c_str()),&owner_pid);
        check("owned central smoke uses a path-private session identity",owner_pid==GetCurrentProcessId());
        check("normal launch has observation off and legacy 50ms cadence",!observation_only_&&message_page_.cadence().interval==50);
        {Visit visit({[&](HWND h){std::wstring captions;::EnumChildWindows(h,[](HWND child,LPARAM data)->BOOL{*reinterpret_cast<std::wstring*>(data)+=ui::control_value(child)+L"\n";return TRUE;},reinterpret_cast<LPARAM>(&captions));
            check("cadence summary is rendered in selected language",captions.find(ui::active_language()=="tw"?L"\u50c5\u8abf\u6574":L"Update cost excludes deferred painting.")!=std::wstring::npos);
            auto interval=::GetDlgItem(::GetDlgItem(h,4000),4100);::SendMessageW(interval,CB_SETCURSEL,2,0);::SendMessageW(h,WM_COMMAND,4500,0);check("cadence form buttons fit and captured",fits(h,::GetDlgItem(h,IDOK))&&save_runtime_frame(h,folder/L"fd-cadence.png"));::SendMessageW(h,WM_COMMAND,IDOK,0);}});monitor_cadence();check("actual cadence form persists 250 without changing drain timer",visit.done()&&message_page_.cadence().interval==250&&GetPrivateProfileIntW(L"Messages",L"PresentationInterval",0,settings_path_.c_str())==250);}
        auto display=message_page_.options();display.clock=MessageClock::UtcIso;message_page_.configure_display(display);check("UTC choice persists through existing settings owner",message_page_.save_options(settings_path_));display.clock=MessageClock::None;message_page_.configure_display(display);message_page_.load_options(settings_path_);check("UTC and cadence restore without renumbering old clocks",message_page_.options().clock==MessageClock::UtcIso&&message_page_.cadence().interval==250);
        SerialSettings settings;settings.port=port;begin_serial_open(settings,false);check("primary fake connection starts before Workbench is opened",pump([&]{return serial_running_.load();}));
        const std::string early="early:1\r\n";{std::lock_guard<std::mutex> lock(state->mutex);state->raw=early;}check("bounded traffic history is retained before Workbench HWND exists",pump([&]{return primary_traffic_->snapshot().rx>=early.size();})&&!workbench_->GetSafeHwnd());
        show_workbench();workbench_->secondary_.set_factory([bstate,io]{return std::make_unique<FdTransport>(bstate,io);});
        settings.port=bport;std::wstring error;check("injected secondary transport starts",workbench_->secondary_.connect(settings,error));check("both fake sources connect",pump([&]{return workbench_->secondary_connected();}));
        const auto saved_ini=workbench_->ini_;const auto retry_folder=folder/L"b-save-recovery";
        workbench_->ini_=(retry_folder/L"settings.ini").wstring();workbench_->saved_b_.clear();workbench_->b_save_retry_at_=0;
        workbench_->persist_b();check("B settings save failure is visible and schedules bounded retry",!workbench_->b_settings_error_.empty()&&workbench_->b_save_retry_at_>GetTickCount64());
        std::filesystem::create_directory(retry_folder);workbench_->persist_b();check("B settings failed save does not retry every UI tick",!std::filesystem::exists(workbench_->ini_));
        workbench_->b_save_retry_at_=0;workbench_->persist_b();
        wchar_t saved_port[32]{};GetPrivateProfileStringW(L"DualCOM.B",L"Port",L"",saved_port,32,workbench_->ini_.c_str());
        check("B settings save recovery commits candidate and clears its error",workbench_->b_settings_error_.empty()&&std::wstring(saved_port)==bport.device&&GetPrivateProfileIntW(L"DualCOM.B",L"Baud",0,workbench_->ini_.c_str())==settings.baud);
        workbench_->b_settings_error_=L"synthetic old error";workbench_->b_save_retry_at_=GetTickCount64()+5000;workbench_->persist_b();
        check("restoring confirmed B settings clears obsolete save errors",workbench_->b_settings_error_.empty()&&workbench_->b_save_retry_at_==0);
        workbench_->ini_=saved_ini;workbench_->saved_b_.clear();workbench_->b_save_retry_at_=0;
        auto& export_group=workbench_->groups_[1];export_group.raw.append("snapshot-before\r\n");const auto snapshot_expected=workbench_->export_text(1);auto export_snapshot=workbench_->export_snapshot(1);export_group.raw.append("snapshot-after\r\n");check("Workbench export snapshot excludes later RX",export_snapshot()==snapshot_expected);
        {Visit visit({[&](HWND h){::SendMessageW(::GetDlgItem(::GetDlgItem(h,4000),4100),CB_SETCURSEL,1,0);check("observation form capture",save_runtime_frame(h,folder/L"fd-observation.png"));::SendMessageW(h,WM_COMMAND,IDOK,0);}});monitor_feature_command(ID_FD_OBSERVE);check("idle connected sources enter observation through native form",visit.done()&&observation_only_&&uart_operations_.observation()&&workbench_->secondary_.queue().observation());}
        for(const auto action:{UartConsole::Request::Action::Send,UartConsole::Request::Action::Controls,UartConsole::Request::Action::Break}){UartConsole::Request request;request.action=action;request.tx.text=L"must-not-send";OnUartRequest(0,reinterpret_cast<LPARAM>(&request));}
        UartWorkbench::Request raw;raw.action=UartWorkbench::Request::SendRaw;raw.bytes="blocked";raw.generation=uart_operations_.generation();raw.was_active=true;check("primary raw tool entry rejected",OnWorkbenchRequest(0,reinterpret_cast<LPARAM>(&raw))==0);
        uint64_t id=0;check("secondary direct and leased tool entries rejected",!workbench_->secondary_.submit("blocked",id,error)&&!workbench_->acquire(1,UartWorkbench::ToolRun::Command,error));
        const auto definitions=active_include_;check("filter Apply blocked without changing active settings",!apply_filter()&&active_include_==definitions);
        poll_uart_operations();check("observation preserves connected pin semantics and blocks keyboard",serial_mgr_->is_connected()&&workbench_->secondary_connected()&&!message_page_.keyboard_available_&&io->writes==0&&io->controls==0&&io->breaks==0);
        RecordingOptions record;record.base=folder/L"fd-exact";check("explicit RX recording allowed in observation",recorder_.start(record));
        const std::string raw_rx="temperature:12\r\ntemperature:15\r\n";{std::lock_guard<std::mutex> lock(state->mutex);state->raw=raw_rx;}check("RX continues through slower presentation and observation",pump([&]{return uart_statistics_.snapshot(GetTickCount64()).rx_bytes>=raw_rx.size()&&message_page_.buffer().bytes().find(raw_rx)!=std::string::npos;}));
        recorder_.stop();check("recorder drains unchanged raw bytes",pump([&]{return !recorder_.status().active&&!recorder_.status().stopping;})&&uart_read_file(folder/L"fd-exact_1.bin",65536,std::atomic<bool>{false})==raw_rx);
        check("observation exit does not start TX",set_observation(false,error)&&io->writes==0);UartConsole::Request send;send.action=UartConsole::Request::Action::Send;send.tx.text=L"explicit";OnUartRequest(0,reinterpret_cast<LPARAM>(&send));check("explicit normal send still reaches fake driver",pump([&]{return io->writes==1&&!uart_operations_.busy();}));poll_uart_operations();
        primary_traffic_->append("A snapshot",1000,false,1,10);workbench_->secondary_.traffic.append("B snapshot",1000,false,1,20);
        {Visit visit({[&](HWND h){check("merged native list includes A and B with bounded preview",ListView_GetItemCount(::GetDlgItem(h,101))>=2&&fits(h,::GetDlgItem(h,101)));check("timeline shows full measured timestamp column",ListView_GetColumnWidth(::GetDlgItem(h,101),0)>=ListView_GetStringWidth(::GetDlgItem(h,101),L"[2026-11-21T23:59:59.999Z] ")+16);check("timeline native capture",save_runtime_frame(h,folder/L"fd-timeline.png"));::SendMessageW(h,WM_COMMAND,IDCANCEL,0);}});monitor_timeline_view();check("background timeline owner consumes native result",pump([&]{poll_discovery();return !monitor_analysis_.busy();})&&visit.done());}
        discovery_thread_launcher_=failed_launch;monitor_timeline_view();check("analysis start failure leaves retryable idle owner",!monitor_analysis_.busy());discovery_thread_launcher_=nullptr;
        capture_.reset();capture_.append("x",{{"x",1}},1);capture_.append("x",{{"x",2}},2);capture_.append("x",{{"x",3}},3);
        {Visit visit({[&](HWND h){::SendMessageW(h,WM_COMMAND,IDOK,0);},[&](HWND h){check("distribution native table and summary",ListView_GetItemCount(::GetDlgItem(h,101))==32&&ui::control_value(::GetDlgItem(h,103)).find(L"3 / 2 / 3 / 3")!=std::wstring::npos);check("distribution native capture",save_runtime_frame(h,folder/L"fd-distribution.png"));::SendMessageW(h,WM_COMMAND,IDCANCEL,0);}});monitor_distribution_view();check("actual distribution request and result work",pump([&]{poll_discovery();return !monitor_analysis_.busy();})&&visit.done());}
        const auto extraction=encode_extraction(extractor_.config());
        const auto suggestion=[&](const std::string& raw,uint16_t prefix,const std::wstring& expected){
            capture_.reset();data_page_.reset();capture_.append(raw,{{"x",1}},4,0,raw.size(),1,0,prefix);data_page_.refresh(4);
            check("JSON suggestion fixture selects exact retained source",data_page_.select_sequence(capture_.rows().back()->sequence));
            Visit visit({[&](HWND h){check("JSON editor opens with valid optional suggestion",ui::control_value(::GetDlgItem(::GetDlgItem(h,4000),4100))==expected);::SendMessageW(h,WM_COMMAND,IDCANCEL,0);}});
            monitor_json_paths();check("JSON optional suggestion never blocks manual sample entry",visit.done());
        };
        suggestion("binary description",UINT16_MAX,L"");suggestion(std::string("a\0b",3),0,L"");suggestion(std::string("\xff",1),0,L"");
        const auto long_sample=std::string("{\"value\":7,\"padding\":\"")+std::string(5000,'a')+"\"}";
        suggestion("prefix "+long_sample,7,uart_wide(long_sample));
        {Visit visit({[&](HWND h){::SetWindowTextW(::GetDlgItem(::GetDlgItem(h,4000),4100),L"{\"sensor\":{\"value\":23}}");::SendMessageW(h,WM_COMMAND,IDOK,0);},[&](HWND h){auto tree=::FindWindowExW(h,nullptr,WC_TREEVIEWW,nullptr);auto root=TreeView_GetRoot(tree),container=TreeView_GetChild(tree,root),leaf=TreeView_GetChild(tree,container);TreeView_Expand(tree,container,TVE_EXPAND);TreeView_SetCheckState(tree,leaf,TRUE);check("JSON tree buttons fit and native capture",fits(h,::GetDlgItem(h,IDOK))&&save_runtime_frame(h,folder/L"fd-json.png"));::SendMessageW(h,WM_COMMAND,IDOK,0);}});monitor_json_paths();check("JSON numeric selection stages ptr without live Apply",visit.done()&&advanced_tools_.field(AdvancedTools::Extraction,1)==L"ptr:\"value1=/sensor/value\""&&encode_extraction(extractor_.config())==extraction);}
        std::vector<TrafficRecord> frames(2);frames[0].bytes=std::string("\0\x55",2);frames[1].bytes=std::string("\1\x55",2);
        {BinaryInspector inspector(this,frames,{});Visit visit({[&](HWND h){::PostMessageW(h,WM_COMMAND,16,0);},[&](HWND h){::SendMessageW(h,WM_COMMAND,IDOK,0);},[&](HWND h){check("variation table includes every selected byte",ListView_GetItemCount(::GetDlgItem(h,101))==2);check("variation capture",save_runtime_frame(h,folder/L"fd-variation.png"));::SendMessageW(h,WM_COMMAND,IDCANCEL,0);::PostMessageW(inspector.GetSafeHwnd(),WM_COMMAND,IDCANCEL,0);}});inspector.DoModal();check("inspector variation button opens bounded native analysis",visit.done());}
        watch_alarms_.observe({0,"temperature","Above maximum",90,2},GetTickCount64());const auto alarm_id=watch_alarms_.rows().back().id;bool acknowledged_active=false;
        {Visit visit({[&](HWND h){check("alarm actions fit and native capture",fits(h,::GetDlgItem(h,IDOK))&&fits(h,::GetDlgItem(h,104))&&fits(h,::GetDlgItem(h,106))&&save_runtime_frame(h,folder/L"fd-alarms.png"));
            auto list=::GetDlgItem(h,101);wchar_t before[64]{};ListView_GetItemText(list,0,0,before,64);::SetWindowTextW(::GetDlgItem(h,100),L"temperature");
            ::SendMessageW(h,WM_COMMAND,106,0);wchar_t after[64]{};ListView_GetItemText(list,0,0,after,64);
            check("alarm refresh preserves query and selected identity",ui::control_value(::GetDlgItem(h,100))==L"temperature"&&std::wstring(before)==after&&ListView_GetNextItem(list,-1,LVNI_SELECTED)==0);
            watch_alarms_.observe({0,"temperature-other","Above maximum",90,2},GetTickCount64());watch_alarms_.observe({0,"temperature-other","Returned to range",5,2},GetTickCount64());
            ::SendMessageW(h,WM_COMMAND,106,0);wchar_t refreshed[64]{};ListView_GetItemText(list,1,5,refreshed,64);
            check("alarm refresh reads changed model states",ListView_GetItemCount(list)==2&&std::wstring(refreshed)==ui::tr(L"Resolved"));::SendMessageW(h,WM_COMMAND,IDOK,0);} , [&] (HWND h){
            acknowledged_active=std::any_of(watch_alarms_.rows().begin(),watch_alarms_.rows().end(),[&](const auto& alarm){return alarm.id==alarm_id&&alarm.acknowledged&&alarm.active;});
            for(unsigned i=0;i<128;++i)watch_alarms_.observe({0,"evict-"+std::to_string(i),"Above maximum",90,2},GetTickCount64());
            ::SendMessageW(h,WM_COMMAND,106,0);auto list=::GetDlgItem(h,101);
            check("expired alarm selection cannot target another row",ListView_GetNextItem(list,-1,LVNI_SELECTED)==-1&&!::IsWindowEnabled(::GetDlgItem(h,IDOK))&&!::IsWindowEnabled(::GetDlgItem(h,104)));
            ::SendMessageW(h,WM_COMMAND,IDOK,0);check("Enter after eviction retains dialog without an action",::IsWindow(h)!=FALSE);::SendMessageW(h,WM_COMMAND,IDCANCEL,0);
        } });monitor_alarm_view();check("actual alarm acknowledgement retains active condition",visit.done()&&acknowledged_active);}
        check("enter observation before reconnect",set_observation(true,error));disconnect();workbench_->secondary_.disconnect();check("fake sources stop without UI join",pump([&]{return !disconnect_pending_&&workbench_->secondary_.idle();}));
        settings.port=port;begin_serial_open(settings,false);check("new generation retains observation policy",pump([&]{return serial_running_.load();})&&uart_operations_.observation());
        check("background analysis starts before cancellation",monitor_analysis_.start([](const std::atomic<bool>& cancelled){while(!cancelled)Sleep(1);return MonitorAnalysisResult{};}));
        OnCancel();check("close cancels result owner before window teardown",discovery_close_pending_&&IsWindow(m_hWnd));discovery_close_pending_=false;
        check("owned analysis cancellation drains",pump([&]{MonitorAnalysisResult result;std::wstring error;return !monitor_analysis_.busy()||monitor_analysis_.take(result,error);}));discovery_close_pending_=false;
    }catch(const std::exception& e){report<<"FAIL: exception "<<e.what()<<'\n';pass=false;}
    callbacks.clear();discovery_thread_launcher_=nullptr;discovery_modal_=false;discovery_close_pending_=false;monitor_analysis_.cancel();
    disconnect();if(workbench_)workbench_->secondary_.disconnect();check("fake teardown completes",pump([&]{return !disconnect_pending_&&(!workbench_||workbench_->secondary_.idle());}));
    port_scanner_=saved_scanner;transport_factory_=saved_factory;uart_notice_=saved_notice;
    report<<"Scope: native monitor FD1-FD8, isolated desktop/fake transports only; no real COM, clipboard or input desktop.\n";return pass;
}
