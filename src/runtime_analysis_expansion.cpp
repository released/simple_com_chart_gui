#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "binary_inspector.h"
#include "settings_fields.h"

namespace {
struct TestWrites {std::mutex mutex;std::vector<std::string> bytes;bool partial=false;size_t size(){std::lock_guard<std::mutex> lock(mutex);return bytes.size();}};
class AnalysisTransport:public FakeSerialTransport {
public:AnalysisTransport(std::shared_ptr<FakeSerialState> state,std::shared_ptr<TestWrites> writes):FakeSerialTransport(state),state_(std::move(state)),writes_(std::move(writes)){}
    size_t write_bytes(std::string_view bytes,std::wstring* error)override{bool partial=false;{std::lock_guard<std::mutex> lock(writes_->mutex);writes_->bytes.emplace_back(bytes);partial=writes_->partial;writes_->partial=false;}
        if(partial){if(error)*error=L"Fixture partial write";return std::min<size_t>(1,bytes.size());}if(bytes=="Q"){std::lock_guard<std::mutex> lock(state_->mutex);state_->raw+="rpm:201\n";}return bytes.size();}
private:std::shared_ptr<FakeSerialState> state_;std::shared_ptr<TestWrites> writes_;
};
thread_local std::function<void(HWND)>* modal_visit=nullptr;
thread_local std::wstring modal_title;
LRESULT CALLBACK modal_hook(int code,WPARAM w,LPARAM l){if(code==HCBT_ACTIVATE&&modal_visit){HWND h=reinterpret_cast<HWND>(w);DWORD process=0;GetWindowThreadProcessId(h,&process);wchar_t caption[256]{};GetWindowTextW(h,caption,256);if(process==GetCurrentProcessId()&&modal_title==caption){auto visit=modal_visit;modal_visit=nullptr;(*visit)(h);PostMessageW(h,WM_COMMAND,IDCANCEL,0);}}return CallNextHookEx(nullptr,code,w,l);}
}
bool CMainDialog::verify_analysis_expansion(){
    const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"analysis-expansion-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+5000;do{MSG message{};unsigned count=0;while(count++<64&&PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto caption=[](HWND h){std::wstring value(static_cast<size_t>(::GetWindowTextLengthW(h))+1,0);::GetWindowTextW(h,value.data(),static_cast<int>(value.size()));value.pop_back();return value;};
    const auto modal=[&](HWND owner,int command,const wchar_t* title){bool visited=false,fonts=true;std::function<void(HWND)> visit=[&](HWND h){visited=true;::EnumChildWindows(h,[](HWND child,LPARAM flag)->BOOL{const auto font=reinterpret_cast<HFONT>(::SendMessageW(child,WM_GETFONT,0,0));if(font&&GetObjectType(font)!=OBJ_FONT)*reinterpret_cast<bool*>(flag)=false;return TRUE;},reinterpret_cast<LPARAM>(&fonts));};modal_title=title;modal_visit=&visit;const auto hook=SetWindowsHookExW(WH_CBT,modal_hook,nullptr,GetCurrentThreadId());::SendMessageW(owner,WM_COMMAND,command,0);if(hook)UnhookWindowsHookEx(hook);modal_visit=nullptr;return visited&&fonts;};
    SendMessage(WM_COMMAND,ID_UART_WORKBENCH);auto& wb=*workbench_;check("new workbench opens without COM access",!serial_mgr_->is_connected()&&!wb.secondary_.connected());
    check("derived options actual menu opens/cancels with live fonts",modal(m_hWnd,ID_UART_DERIVED,L"Derived channels"));
    check("binary replay options actual menu opens/cancels",modal(m_hWnd,ID_UART_BINARY_REPLAY,L"Offline binary replay schema"));
    check("B text options actual menu opens/cancels",modal(wb.m_hWnd,ID_WORKBENCH_TEXT_B,L"Port B text charts"));
    check("forwarding options cancel never arms TX",modal(wb.m_hWnd,ID_WORKBENCH_FORWARD,L"Opt-in A/B forwarding")&&!wb.forwarder_.active());
    std::wstring error;check("B text configuration applies to fixture INI",wb.apply_text({L"Enabled",L"fan",L"bad",L"Text",L"Key/value",L"",L"",L"1",L"2"},error));
    std::string detail;check("derived formula loads from fixture settings",WritePrivateProfileStringW(L"Derived",L"Configuration",uart_wide(uart_hex("power = voltage*current\nspeed = B.rpm*2")).c_str(),settings_path_.c_str())!=FALSE);load_derived();
    auto a=std::make_shared<FakeSerialState>(),b=std::make_shared<FakeSerialState>();auto aw=std::make_shared<TestWrites>(),bw=std::make_shared<TestWrites>();
    const std::vector<SerialPortInfo> ports={{L"COM81",L"Analysis A",L"ANALYSIS_A",L"fake-A"},{L"COM82",L"Analysis B",L"ANALYSIS_B",L"fake-B"}};
    unsigned warnings=0;uart_notice_=[&](HWND,LPCWSTR text,LPCWSTR,UINT){++warnings;report<<"NOTICE: "<<message_utf8(text)<<'\n';return IDOK;};wb.notice_=uart_notice_;
    port_scanner_=[ports](bool* ok){if(ok)*ok=true;return ports;};transport_factory_=[a,aw]{return std::make_unique<AnalysisTransport>(a,aw);};wb.secondary_.set_factory([b,bw]{return std::make_unique<AnalysisTransport>(b,bw);});wb.set_scanner(port_scanner_);scan_ports();wb.scan();
    const auto click=[&](size_t port,int control){wb.SetActiveWindow();::SendMessageW(::GetDlgItem(wb.groups_[port].box,wb.id(port,control)),BM_CLICK,0,0);};
    ::SendMessageW(wb.groups_[0].port,CB_SETCURSEL,0,0);::SendMessageW(wb.groups_[1].port,CB_SETCURSEL,1,0);click(0,UartWorkbench::Connect);click(1,UartWorkbench::Connect);
    const bool connected=pump([&]{return serial_mgr_->is_connected()&&wb.secondary_.connected();});check("both fake transports connect through actual controls",connected&&warnings==0);if(!connected){wb.shutdown();disconnect();pump([&]{return wb.shutdown_complete()&&!disconnect_pending_;});return false;}
    {std::lock_guard<std::mutex> lock(a->mutex);a->raw="voltage:12,current:2\n";}{std::lock_guard<std::mutex> lock(b->mutex);b->raw="fan,rpm:201,temp:-2.5\nfan,bad,rpm:999\n";}
    check("A raw and derived plus independent B text feed shared choices",pump([&]{double value=0;return model_.latest_value("B.rpm",value)&&value==201&&model_.latest_value("calc.power",value)&&value==24&&model_.latest_value("B.calc.speed",value)&&value==402;}));
    const auto temperature=model_.get_series("B.temp");check("B filtering retains valid signed value and discovery stays none",model_.get_enabled_count()==0&&model_.get_series("B.rpm").size()==1&&!temperature.empty()&&temperature.back().v==-2.5);
    check("A/B raw views remain separate",caption(wb.groups_[0].log).find(L"fan,rpm")==std::wstring::npos&&caption(wb.groups_[1].log).find(L"voltage")==std::wstring::npos);
    const auto snapshot=current_profile();check("complete workspace captures B text and formulas without private paths",snapshot.workspace&&snapshot.workspace_settings.count("Workbench.Text.B/Configuration")&&snapshot.workspace_settings.count("Derived/Configuration")&&snapshot.workspace_settings.count("Private/History")==0);
    check("profile Apply refuses a live dual-port session",!apply_profile(snapshot));
    ForwardOptions forward;forward.seconds=10;forward.byte_limit=3;check("forwarding reserves both connected TX queues",wb.start_forwarding(forward,error)&&wb.tx_reserved(0)&&wb.tx_reserved(1));
    check("competing command blocked by forwarding lease",!wb.command_send(0,"Q",error));const auto writes=bw->size();{std::lock_guard<std::mutex> lock(a->mutex);a->raw="X\r\n";}
    check("actual worker forwards exact RX to B and releases finite run",pump([&]{return !wb.forwarder_.active()&&bw->size()==writes+1;}));
    {std::lock_guard<std::mutex> lock(bw->mutex);check("forwarded driver bytes are unmodified",!bw->bytes.empty()&&bw->bytes.back()=="X\r\n");}
    check("both ports continue monitoring after forwarding completes",serial_mgr_->is_connected()&&wb.secondary_.connected()&&!wb.tx_reserved(0)&&!wb.tx_reserved(1));
    forward.byte_limit=100;check("second forwarding requires explicit Start",wb.start_forwarding(forward,error));{std::lock_guard<std::mutex> lock(bw->mutex);bw->partial=true;}{std::lock_guard<std::mutex> lock(a->mutex);a->raw="ABC\n";}
    check("partial B write stops both forwarding leases",pump([&]{return !wb.forwarder_.active()&&!wb.tx_reserved(0)&&!wb.tx_reserved(1);}));
    ScriptStep step;step.command.group="Fixture";step.command.name="Read RPM";step.command.body="Q";step.numeric_key="rpm";step.minimum=200;step.maximum=210;
    check("numeric script starts explicitly through Workbench",wb.start_script(1,{step},error));check("actual queued script validates device-reported range",pump([&]{return !wb.tx_reserved(1);})&&!wb.runs_[1].script.results().empty()&&wb.runs_[1].script.results()[0].find("PASS")==0);check("numeric CSV report includes observed value",wb.runs_[1].script.csv().find("201")!=std::string::npos);
    BinaryFrameConfig config;config.header="H";config.fixed_bytes=3;config.fields={{"v","u16le",1,1,0}};TrafficRecord packet;packet.bytes=std::string("H\xC9\0",3);BinaryInspector inspector(&wb,{packet},config);check("native read-only inspector creates controls",inspector.Create(IDD_UART_TOOL_FORM,&wb)!=FALSE);inspector.ShowWindow(SW_SHOW);
    check("inspector displays decoded field span/value",inspector.rows_.size()==1&&inspector.rows_[0].valid&&inspector.rows_[0].offset==1&&inspector.rows_[0].value=="201");ListView_SetItemState(inspector.list_,0,LVIS_SELECTED,LVIS_SELECTED);DWORD first=0,last=0;::SendMessageW(inspector.hex_,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));check("selecting actual field highlights exact HEX bytes without edit focus",last>first&&(::GetWindowLongPtrW(inspector.hex_,GWL_STYLE)&ES_NOHIDESEL));
    inspector.SetActiveWindow();const auto paint_started=GetTickCount64();
    check("inspector initial paint settles before capture",pump([&]{return GetTickCount64()-paint_started>=150;}));
    inspector.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    check("inspector screenshot",save_runtime_frame(inspector.m_hWnd,folder/L"binary-inspector.png"));inspector.DestroyWindow();
    wb.SendMessage(WM_COMMAND,ID_WORKBENCH_SIGNALS);auto& signals=wb.signals_;::SetWindowTextW(signals.x_,L"voltage");::SetWindowTextW(signals.y_,L"current");signals.SendMessage(WM_COMMAND,MAKEWPARAM(7001,CBN_KILLFOCUS),reinterpret_cast<LPARAM>(signals.x_));check("native live XY uses same-row pairs",!signals.points_[0].empty()&&signals.points_[0].back()==std::make_pair(12.0,2.0));check("XY screenshot",save_runtime_frame(signals.m_hWnd,folder/L"live-xy.png"));
    std::atomic<bool> cancel{false};const auto run1=folder/L"compare-1.csv",run2=folder/L"compare-2.csv";uart_write_file(run1,"Row,Time (s),rpm\r\n1,0,201\r\n2,1,202\r\n",true,cancel);uart_write_file(run2,"Row,Time (s),rpm\r\n1,0,201\r\n2,1,204\r\n",true,cancel);
    ::SendMessageW(signals.mode_,CB_SETCURSEL,1,0);signals.refresh();check("comparison background run 1 import starts",signals.begin_load(0,run1));check("run 1 background import completes",pump([&]{return !signals.busy();}));check("comparison background run 2 import starts",signals.begin_load(1,run2));check("run 2 background import completes",pump([&]{return !signals.busy();}));::SetWindowTextW(signals.x_,L"rpm");signals.refresh(false);check("native comparison plots both runs with numeric verdicts",signals.comparison_.matches.size()==2&&signals.comparison_.passed==1&&signals.points_[0].size()==2&&signals.points_[1].size()==2);check("comparison screenshot",save_runtime_frame(signals.m_hWnd,folder/L"run-comparison.png"));signals.ShowWindow(SW_HIDE);
    click(1,UartWorkbench::Connect);disconnect();const bool idle=pump([&]{return advanced_idle();});check("fake ports drain before offline/profile work",idle);
    if(!idle)report<<"STATE: A connected="<<serial_mgr_->is_connected()<<" running="<<serial_running_<<" stopping="<<disconnect_pending_<<" mode="<<static_cast<int>(connection_.mode())<<" B connected="<<wb.secondary_.connected()<<" reserved="<<wb.secondary_.reserved()<<" analysis busy="<<wb.analysis_busy()<<'\n';
    const auto encoded=encode_profile(snapshot);check("complete workspace round trip validates",decode_profile(encoded).workspace);
    replay_binary_enabled_=true;replay_binary_config_=config;reset_binary_replay();const auto recording=folder/L"binary_1.bin",index=folder/L"binary_1.idx";uart_write_file(recording,std::string("H\xC9\0H\xE8\x03",6),true,cancel);uart_write_file(index,"SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,1,1700000000000,RX\r\n1,2,1700000000001,RX\r\n3,0,1700000000002,Unexpected disconnect\r\n3,3,1700000000003,RX\r\n",true,cancel);
    replay_.load(load_replay(recording,100,cancel));reset_offline_view();message_page_.set_offline_source(true,false);show_advanced_tools(AdvancedTools::Replay);replay_.play(GetTickCount64(),1);
    check("offline binary recording decodes across reads without live ports",pump([&]{return capture_.rows().size()==2&&replay_frames_.empty();})&&model_.get_series("v").size()==2&&model_.get_series("v").back().v==1000);
    check("offline source remains unchanged and never reopens COM",uart_read_file(recording,100,cancel)==std::string("H\xC9\0H\xE8\x03",6)&&a->attempt_count()==1&&b->attempt_count()==1);
    replay_.pause(GetTickCount64());reset_offline_view();replay_.seek(0,GetTickCount64());check("seek clears binary partials and numeric view",capture_.rows().empty()&&replay_binary_state_->decoder.pending_bytes()==0);replay_.load({});reset_offline_view();replay_binary_enabled_=false;
    check("idle workspace Apply starts atomic save without opening hardware",apply_profile(snapshot));check("workspace Apply completes and leaves both ports closed",pump([&]{return !profile_job_.busy();})&&!serial_mgr_->is_connected()&&!wb.secondary_.connected());
    wb.shutdown();check("new file jobs and owned fake workers drain on shutdown",pump([&]{return wb.shutdown_complete();}));advanced_tools_.ShowWindow(SW_HIDE);
    report<<(pass?"PASS":"FAIL")<<": analysis expansion; isolated controls/files/fake transports only\n";return pass;
}
