#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "session_metadata.h"
#include "build_version.h"
#include <psapi.h>
#include <commctrl.h>
#include <richedit.h>
#pragma comment(lib,"psapi.lib")
namespace {
struct SoakWindowCost {
    struct Cost {UINT message=0;uint64_t count=0,total=0,maximum=0;};
    HWND window=nullptr;std::vector<Cost> costs;
    explicit SoakWindowCost(HWND h):window(h){for(const auto message:{WM_TIMER,WM_PAINT,WM_ERASEBKGND,WM_SETTEXT,WM_GETTEXT,WM_GETTEXTLENGTH,WM_VSCROLL,EM_REPLACESEL,EM_STREAMIN,EM_SETCHARFORMAT,EM_SCROLLCARET,EM_SETSEL,EM_SETTARGETDEVICE})costs.push_back({static_cast<UINT>(message)});SetWindowSubclass(window,observe,0x53434F53,reinterpret_cast<DWORD_PTR>(this));}
    ~SoakWindowCost(){if(IsWindow(window))RemoveWindowSubclass(window,observe,0x53434F53);}
    static LRESULT CALLBACK observe(HWND h,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data){auto& self=*reinterpret_cast<SoakWindowCost*>(data);auto found=std::find_if(self.costs.begin(),self.costs.end(),[&](const auto& cost){return cost.message==message;});if(found==self.costs.end())return DefSubclassProc(h,message,w,l);const auto began=GetTickCount64();const auto result=DefSubclassProc(h,message,w,l);const auto elapsed=GetTickCount64()-began;++found->count;found->total+=elapsed;found->maximum=std::max(found->maximum,elapsed);return result;}
    void report(std::ostream& out,const char* label)const{for(const auto& cost:costs)if(cost.count)out<<label<<"_msg_"<<cost.message<<"_count="<<cost.count<<" total_ms="<<cost.total<<" max_ms="<<cost.maximum<<'\n';}
};
uint64_t cpu_ticks(){FILETIME created{},exited{},kernel{},user{};GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user);ULARGE_INTEGER k{},u{};k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;return k.QuadPart+u.QuadPart;}
uint64_t private_bytes(){PROCESS_MEMORY_COUNTERS_EX value{};value.cb=sizeof(value);return GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&value),sizeof(value))?value.PrivateUsage:0;}
uint64_t hash_bytes(uint64_t hash,std::string_view bytes){for(const unsigned char c:bytes){hash^=c;hash*=1099511628211ULL;}return hash;}
}
bool CMainDialog::verify_receive_soak() {
    unsigned seconds=60;const std::wstring command=AfxGetApp()->m_lpCmdLine;const auto at=command.find(L"--soak-seconds=");if(at!=command.npos){const auto parsed=wcstoul(command.c_str()+at+15,nullptr,10);if(parsed>=60 && parsed<=86400) seconds=static_cast<unsigned>(parsed);}
    const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"soak-result.txt");bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';report.flush();pass=pass&&ok;};
    demo_=false;auto original_transport=std::move(serial_mgr_);const auto scanner=port_scanner_;
    auto serial=std::make_shared<FakeSerialState>();serial_mgr_=std::make_unique<FakeSerialTransport>(serial);serial_mgr_->connect(L"fake://soak",115200,8,0,0,nullptr);
    SerialSettings settings;settings.port={L"COM7",L"soak fixture",L"USB\\SOAK",L"fake://soak"};connection_.begin(settings,false);connection_.connected(settings.port);port_scanner_=[settings](bool* ok){if(ok) *ok=true;return std::vector<SerialPortInfo>{settings.port};};
    ::SetWindowTextW(edit_include_,L"");::SetWindowTextW(edit_exclude_,L"");::SendMessageW(combo_filter_,CB_SETCURSEL,0,0);apply_filter(false);extractor_=FieldExtractor{};message_page_.reset();
    TabCtrl_SetCurSel(tabs_,kMessagesTab);TabCtrl_SetCurFocus(tabs_,kMessagesTab);switch_page();uart_statistics_.reset();pending_line_loss_=0;
    const bool monitor=command.find(L"--monitor-soak")!=command.npos;bool monitor_chart_selected=false;uint64_t monitor_plot_at=0,monitor_chart_updates=0;
    if(monitor){std::vector<KeywordRule> rules;std::wstring error;const std::wstring definition=L"cyan,word,1,a:1\nyellow,word,1,c:3";check("monitor soak validates keyword rules",parse_keyword_rules(definition,rules,error));message_page_.set_keywords(definition,rules);WritePrivateProfileStringW(L"Messages",L"KeywordRules",monitor_hex_text(definition).c_str(),settings_path_.c_str());SetWindowPos(nullptr,0,0,1500,1050,SWP_NOMOVE|SWP_NOZORDER);TabCtrl_SetCurSel(tabs_,kChartTab);TabCtrl_SetCurFocus(tabs_,kChartTab);if(!split_logs_)SendMessage(WM_COMMAND,ID_MONITOR_SPLIT);switch_page();check("monitor soak renders split chart and Messages",split_actual_&&::IsWindowVisible(message_page_.hwnd())&&::IsWindowVisible(plot_view_.hwnd()));}
    RecordingOptions options;options.base=folder/(L"soak-"+std::to_wstring(GetCurrentProcessId()));options.rotate_bytes=1024*1024;options.rotate_daily=false;options.limits={64ULL*1024*1024+seconds*512ULL*1024,seconds+30,1024*1024};options.metadata=encode_capture_metadata(current_profile(),CHART_VERSION_STR,"A");check("start isolated RX recorder with all limits and capture metadata",recorder_.start(options));start_serial_thread();
    const bool waterfall=command.find(L"--waterfall-soak")!=command.npos;if(waterfall){show_uart_lab();lab_.field(UartLabWindow::Waterfall,0,L"a");lab_.field(UartLabWindow::Waterfall,1,L"256");lab_.field(UartLabWindow::Waterfall,2,L"400");lab_.field(UartLabWindow::Waterfall,3,L"64");lab_.select(UartLabWindow::Waterfall);lab_waterfall_request({UartLabWindow::Waterfall,0,lab_.values(UartLabWindow::Waterfall)});}
    SoakWindowCost main_cost(m_hWnd),message_cost(message_page_.view()),plot_cost(plot_view_.hwnd());
    std::string line="a:1,b:2,c:3";line.resize(255,' ');line+='\n';std::string chunk;for(int i=0;i<16;++i) chunk+=line;
    std::atomic<bool> stop{false};uint64_t produced=0,expected_hash=1469598103934665603ULL;bool producer_overflow=false;
    std::thread producer([&]{while(!stop){ {std::lock_guard<std::mutex> lock(serial->mutex);if(serial->raw.size()>4*1024*1024){producer_overflow=true;break;}serial->raw+=chunk;}produced+=chunk.size();expected_hash=hash_bytes(expected_hash,chunk);Sleep(40);}});
    const auto start=GetTickCount64(),cpu_start=cpu_ticks();uint64_t previous=start,max_gap=0,warm_memory=0,peak=0;const auto until=start+seconds*1000ULL;
    const auto pump=[&]{MSG message{};const auto began=GetTickCount64();unsigned dispatched=0;while(dispatched++<32 && GetTickCount64()-began<8 && PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){if(message.message==WM_CLOSE){stop=true;continue;}TranslateMessage(&message);DispatchMessageW(&message);}};
    while(GetTickCount64()<until && !stop){const auto now=GetTickCount64();max_gap=std::max(max_gap,now-previous);previous=now;pump();if(monitor&&!monitor_chart_selected){const auto keys=model_.get_keys();if(std::find(keys.begin(),keys.end(),"a")!=keys.end()){model_.set_enabled("a",true);channel_panel_.set_checkbox_state_map({{"a",true}});monitor_chart_selected=true;}}
        if(monitor&&monitor_chart_selected&&now>=monitor_plot_at){plot_view_.update_from_model(now_seconds());monitor_plot_at=now+100;++monitor_chart_updates;}
        const auto memory=private_bytes();peak=std::max(peak,memory);if(!warm_memory && now-start>=5000) warm_memory=memory;Sleep(5);}
    const auto waterfall_rows=waterfall?lab_.waterfall_.history().rows().size():0;waterfall_running_=false;
    stop=true;producer.join();const auto drain=GetTickCount64()+5000;
    while(uart_statistics_.snapshot(GetTickCount64()).rx_bytes<produced && GetTickCount64()<drain){pump();Sleep(5);}pump();recorder_.stop();
    const auto saved=GetTickCount64()+5000;while(recorder_.status().active && GetTickCount64()<saved){pump();Sleep(5);}
    while(GetTickCount64()<saved){size_t pending=0;{std::lock_guard<std::mutex> lock(pending_mutex_);pending=pending_.lines.size();}if(!pending) break;pump();Sleep(5);}
    const auto status=recorder_.status();uint64_t actual_hash=1469598103934665603ULL,total=0;std::atomic<bool> cancel{false};
    for(uint64_t i=1;i<=status.segments;++i){ReplayFile file(options.base.wstring()+L"_"+std::to_wstring(i)+L".bin");for(uint64_t offset=0;offset<file.size();){const auto bytes=file.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,file.size()-offset)),cancel);actual_hash=hash_bytes(actual_hash,bytes);total+=bytes.size();offset+=bytes.size();}}
    const auto stats=uart_statistics_.snapshot(GetTickCount64());const auto elapsed=GetTickCount64()-start;
    report<<"duration_ms="<<elapsed<<"\nproduced_bytes="<<produced<<"\nrx_bytes="<<stats.rx_bytes<<"\nrecorded_bytes="<<status.bytes<<"\nactual_output_bytes="<<status.output_bytes<<"\nsegments="<<status.segments<<"\nmax_ui_pump_gap_ms="<<max_gap<<"\nprivate_after_5s="<<warm_memory<<"\npeak_private_bytes="<<peak<<"\none_core_cpu_percent="<<static_cast<double>(cpu_ticks()-cpu_start)/10000/std::max<uint64_t>(1,elapsed)*100<<"\nraw_expired="<<message_page_.buffer().offset()<<"\nrows_expired="<<capture_.dropped()<<"\nraw_lost="<<message_page_.buffer().skipped()<<"\nlines_lost="<<pending_line_loss_<<'\n';
    main_cost.report(report,"main");message_cost.report(report,"messages");plot_cost.report(report,"plot");
    if(monitor){report<<"monitor_soak=1\nkeyword_rules=2\nmonitor_chart_updates="<<monitor_chart_updates<<'\n';check("monitor chart stayed selected and split after sustained RX",monitor_chart_selected&&split_actual_&&::IsWindowVisible(plot_view_.hwnd())&&::IsWindowVisible(message_page_.hwnd()));}
    check("configured-duration independent producer sustained RX",elapsed>=seconds*1000ULL && !producer_overflow && produced>=4*1024*1024);
    check("all produced bytes reached RX and raw recording",stats.rx_bytes==produced && status.bytes==produced && total==produced && !status.lost && !status.active);
    check("rotation segments contain exact ordered bytes",actual_hash==expected_hash && status.segments>=4);
    check("recording guards preserve accepted output without premature stop",!status.limit_reached&&status.output_bytes<=options.limits.bytes&&status.output_bytes>status.bytes);
    check("retention expires visibly without pending input loss",message_page_.buffer().offset()>0 && capture_.dropped()>0 && !message_page_.buffer().skipped() && !pending_line_loss_);
    check("bounded memory growth after warmup",warm_memory && peak>=warm_memory && peak-warm_memory<64*1024*1024);
    check("UI pump remains responsive in simulated load",max_gap<1000);
    if(waterfall){report<<"waterfall_rows="<<waterfall_rows<<"\n";check("bounded waterfall runs concurrently with sustained RX",waterfall_rows>=10&&waterfall_rows<=64);check("waterfall worker drains after receive measurement",!lab_file_.busy());check("waterfall receive screenshot",save_runtime_frame(lab_.m_hWnd,folder/L"soak-waterfall.png"));}
    check("persistent receive status screenshot",save_runtime_frame(m_hWnd,folder/L"soak-final.png"));
    disconnect();const auto stopped=GetTickCount64()+5000;while(disconnect_pending_ && GetTickCount64()<stopped){poll_serial_stop();Sleep(2);}check("soak workers stop safely",!disconnect_pending_);
    serial_mgr_=std::move(original_transport);port_scanner_=scanner;connection_.cancel();return pass;
}
