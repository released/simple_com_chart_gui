#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "recording_settings.h"
#include "session_manifest.h"
#include "input_framing.h"
#include "build_version.h"
#include <cmath>
namespace {
struct SessionWrites {std::mutex mutex;std::vector<std::string> packets;size_t count(){std::lock_guard<std::mutex> lock(mutex);return packets.size();}};
class SessionTransport:public FakeSerialTransport {
public:SessionTransport(std::shared_ptr<FakeSerialState> s,std::shared_ptr<SessionWrites> w):FakeSerialTransport(std::move(s)),writes(std::move(w)){}
    size_t write_bytes(std::string_view bytes,std::wstring*)override{std::lock_guard<std::mutex> lock(writes->mutex);writes->packets.emplace_back(bytes);return bytes.size();}
private:std::shared_ptr<SessionWrites> writes;
};
thread_local std::function<void(HWND)>* record_visit=nullptr;
LRESULT CALLBACK record_hook(int code,WPARAM w,LPARAM l){if(code==HCBT_ACTIVATE&&record_visit){const auto h=reinterpret_cast<HWND>(w);DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId()&&GetDlgItem(h,IDC_RECORD_TOTAL)&&GetDlgItem(h,IDC_RECORD_SECONDS)&&GetDlgItem(h,IDC_RECORD_FREE)){const auto visit=record_visit;record_visit=nullptr;(*visit)(h);PostMessageW(h,WM_COMMAND,IDOK,0);}}return CallNextHookEx(nullptr,code,w,l);}
}
bool CMainDialog::verify_uart_session(){
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"uart-session-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* text,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<text<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+7000;do{MSG m{};unsigned count=0;while(count++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto click=[&](int page,int action){lab_.select(page);::SendMessageW(lab_.actions_[page][action],BM_CLICK,0,0);};
    try {
        SendMessage(WM_COMMAND,ID_UART_LAB);lab_.SetWindowPos(nullptr,0,0,700,520,SWP_NOMOVE|SWP_NOZORDER);
        check("Laboratory adds six native pages without COM access",lab_.GetSafeHwnd()&&UartLabWindow::PageCount==13&&!serial_mgr_->is_connected());
        for(int page=UartLabWindow::Sessions;page<UartLabWindow::PageCount;++page){lab_.select(page);CRect area;lab_.viewport_.GetClientRect(&area);bool valid=true;CClientDC dc(&lab_);const auto old=dc.SelectObject(CFont::FromHandle(lab_.font_));for(const auto& f:lab_.fields_[page]){CRect r,label;::GetWindowRect(f.input,&r);::GetWindowRect(f.label,&label);lab_.viewport_.ScreenToClient(&r);valid=valid&&::IsWindowVisible(f.input)&&r.left>=0&&r.right<=area.right&&label.Width()>=dc.GetTextExtent(f.name.c_str()).cx&&reinterpret_cast<HFONT>(::SendMessageW(f.input,WM_GETFONT,0,0))==lab_.font_;}dc.SelectObject(old);lab_.OnVScroll(SB_BOTTOM,0,nullptr);check(("native new page "+std::to_string(page)+" has unclipped labels/live fonts/scroll at minimum size").c_str(),valid);}
        lab_.SetWindowPos(nullptr,0,0,1040,820,SWP_NOMOVE|SWP_NOZORDER);
        device_scanner_=[](const std::atomic<bool>&){DeviceDetails d;d.port=L"COM171";d.name=L"Session fixture adapter";d.hardware=L"USB\\VID_1234&PID_5678";return std::vector<DeviceDetails>{d};};
        lab_.field(UartLabWindow::Devices,0,L"VID_1234");click(UartLabWindow::Devices,0);check("device page filters fake catalog in background without opening COM",pump([&]{return !lab_file_.busy();})&&lab_.reports_[UartLabWindow::Devices].find(L"COM171")!=std::wstring::npos&&!serial_mgr_->is_connected());
        lab_.field(UartLabWindow::Input,1,L"Custom delimiter");lab_.field(UartLabWindow::Input,2,L"3B");click(UartLabWindow::Input,1);
        check("actual framing Apply persists without opening hardware",saved_text_framing(settings_path_,0).delimiter==";"&&!serial_mgr_->is_connected());
        show_advanced_tools(AdvancedTools::Extraction);::SendMessageW(advanced_tools_.field_hwnd(AdvancedTools::Extraction,0),CB_SETCURSEL,3,0);advanced_tools_.set_field(AdvancedTools::Extraction,1,L"ptr:rpm=/fan/rpm");advanced_tools_.set_field(AdvancedTools::Extraction,7,L"{\"fan\":{\"rpm\":201}}");
        ::SendMessageW(advanced_tools_.action_hwnd(AdvancedTools::Extraction,0),BM_CLICK,0,0);check("JSON Pointer preview does not apply",extractor_.config().mode==ExtractionConfig::KeyValue);
        ::SendMessageW(advanced_tools_.action_hwnd(AdvancedTools::Extraction,1),BM_CLICK,0,0);check("explicit JSON parser Apply saves compatible preferences",extractor_.config().mode==ExtractionConfig::Json&&extractor_.config().columns=="ptr:rpm=/fan/rpm");advanced_tools_.ShowWindow(SW_HIDE);
        auto state=std::make_shared<FakeSerialState>();auto writes=std::make_shared<SessionWrites>();const std::vector<SerialPortInfo> ports{{L"COM171",L"Session fixture A",L"SESSION_FAKE_A",L"fake-session-A"}};
        port_scanner_=[ports](bool* ok){if(ok)*ok=true;return ports;};transport_factory_=[state,writes]{return std::make_unique<SessionTransport>(state,writes);};uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};scan_ports();::SendMessageW(combo_port_,CB_SETCURSEL,0,0);on_connect_toggle();
        check("fake A connects through the real UI path",pump([&]{return serial_mgr_->is_connected();}));
        {std::lock_guard<std::mutex> lock(state->mutex);state->raw="{\"fan\":{\"rpm\":201}};";}
        check("actual RX worker charts delimiter-terminated nested JSON without LF",pump([&]{double v=0;return model_.latest_value("rpm",v)&&v==201;})&&model_.get_enabled_count()==0&&message_page_.buffer().bytes().find(";")!=std::string::npos);
        lab_.field(UartLabWindow::Checksums,0,L"8:07:0:0:0:0");lab_.field(UartLabWindow::Checksums,1,L"31 32 33 34 35 36 37 38 39");click(UartLabWindow::Checksums,0);
        check("native custom CRC preview matches F4 and sends nothing",lab_.reports_[UartLabWindow::Checksums].find(L"F4")!=std::wstring::npos&&writes->count()==0);
        click(UartLabWindow::Checksums,1);lab_.field(UartLabWindow::Packet,2,L"31 32 33 34 35 36 37 38 39");click(UartLabWindow::Packet,0);
        check("CRC handoff to packet is preview only",lab_.reports_[UartLabWindow::Packet].find(L"313233343536373839F4")!=std::wstring::npos&&writes->count()==0);
        click(UartLabWindow::Packet,1);check("explicit confirmed custom packet uses existing finite TX ownership",pump([&]{return writes->count()==1&&workbench_&&!workbench_->tx_reserved(0);}));
        {std::lock_guard<std::mutex> lock(writes->mutex);check("driver receives exact previewed custom CRC bytes",writes->packets.size()==1&&uart_hex(writes->packets[0])=="313233343536373839F4");}
        bool visited=false;std::function<void(HWND)> visit=[&](HWND h){visited=true;::SetDlgItemInt(h,IDC_RECORD_TOTAL,1,FALSE);::SetDlgItemInt(h,IDC_RECORD_SECONDS,60,FALSE);::SetDlgItemInt(h,IDC_RECORD_FREE,0,FALSE);};record_visit=&visit;const auto hook=SetWindowsHookExW(WH_CBT,record_hook,nullptr,GetCurrentThreadId());
        if(!hook)throw std::runtime_error("Cannot install owned recording-dialog fixture hook.");recording_options();UnhookWindowsHookEx(hook);record_visit=nullptr;
        auto options=saved_recording_options(settings_path_,folder/L"session-live");options.metadata=encode_capture_metadata(current_profile(),CHART_VERSION_STR,"A");check("actual recording-options dialog saves output/time/free limits",visited&&options.limits.bytes==1048576&&options.limits.seconds==60&&options.limits.free_bytes==0);
        check("live recorder starts with sanitized start metadata",recorder_.start(options));
        bool chunks=true;for(unsigned i=0;i<32&&recorder_.status().active;++i){const auto before=recorder_.status().bytes;{std::lock_guard<std::mutex> lock(state->mutex);state->raw=std::string(32000,'X')+';';}chunks=chunks&&pump([&]{return recorder_.status().bytes>before||!recorder_.status().active;});}
        check("complete bounded UART read chunks are recorded",chunks&&recorder_.status().bytes>=32001);
        check("recording limit stops writer while COM monitoring stays connected",!recorder_.status().active&&recorder_.status().limit_reached&&recorder_.status().output_bytes<=1048576&&serial_mgr_->is_connected());
        disconnect();check("fake UART drains before manifest/profile work",pump([&]{return !disconnect_pending_&&!serial_mgr_->is_connected();}));
        lab_.field(UartLabWindow::Sessions,0,(folder/L"session-live_1.bin").wstring());lab_.field(UartLabWindow::Sessions,1,(folder/L"session.sccsession").wstring());click(UartLabWindow::Sessions,3);
        check("native session creation hashes references without raw duplication",pump([&]{return !lab_file_.busy();})&&std::filesystem::exists(folder/L"session.sccsession"));
        click(UartLabWindow::Sessions,4);check("native verification checks listed bytes/SHA256 and segment set",pump([&]{return !lab_file_.busy();})&&lab_.reports_[UartLabWindow::Sessions].find(L"All listed files match")!=std::wstring::npos);
        WritePrivateProfileStringW(L"Private",L"Keep",L"fixture sentinel",settings_path_.c_str());click(UartLabWindow::Sessions,5);
        check("session settings stage explicitly without connecting or TX replay",pump([&]{return !lab_file_.busy();})&&has_staged_profile_&&staged_profile_.serial.port.device.empty()&&staged_profile_.serial.port.instance_id.empty()&&state->attempt_count()==1&&writes->count()==1&&advanced_tools_.field(AdvancedTools::Replay,0)==(folder/L"session-live_1.bin").wstring());
        ::SendMessageW(advanced_tools_.action_hwnd(AdvancedTools::Profiles,4),BM_CLICK,0,0);check("staged session Apply restores parser/framing while hardware stays closed",pump([&]{return !profile_job_.busy();})&&extractor_.config().columns=="ptr:rpm=/fan/rpm"&&numeric_framing_[0].delimiter==";"&&!serial_mgr_->is_connected());
        wchar_t kept[64]{};GetPrivateProfileStringW(L"Private",L"Keep",L"",kept,64,settings_path_.c_str());check("session Apply preserves unrelated private INI values",std::wstring(kept)==L"fixture sentinel");advanced_tools_.ShowWindow(SW_HIDE);
        lab_.select(UartLabWindow::Sessions);lab_.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("session page evidence",save_runtime_frame(lab_.m_hWnd,folder/L"session-manifest.png"));
        capture_.reset();model_.reset();accept_numeric("v:2",{{"v",2}},100,UINT64_MAX,0,0,1);accept_numeric("v:4",{{"v",4}},102,UINT64_MAX,0,0,1);sync_channels();lab_.field(UartLabWindow::Calculus,1,L"v");lab_.field(UartLabWindow::Calculus,3,L"3");click(UartLabWindow::Calculus,0);
        check("native calculus job reads retained raw values independent of chart choices",pump([&]{return !lab_file_.busy();})&&calculus_result_&&calculus_result_->points.back().value==1&&model_.get_enabled_count()==0);
        const auto paint=GetTickCount64();pump([&]{return GetTickCount64()-paint>=120;});lab_.OnVScroll(SB_LINEDOWN,0,nullptr);lab_.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("calculus child uses live font and paints result",reinterpret_cast<HFONT>(lab_.calculus_.SendMessage(WM_GETFONT))==lab_.font_&&save_runtime_frame(lab_.m_hWnd,folder/L"session-calculus.png"));
        primary_traffic_->clear();const auto t=traffic_monotonic_seconds();primary_traffic_->append(std::string("Q\x01\n",3),1000,true,t,8);primary_traffic_->append(std::string("R\x01\n",3),1020,false,t+.02,8);lab_.field(UartLabWindow::Transactions,5,L"51");lab_.field(UartLabWindow::Transactions,6,L"52");click(UartLabWindow::Transactions,0);
        check("native passive transaction snapshot pairs rule IDs without TX",pump([&]{return !lab_file_.busy();})&&transaction_result_&&transaction_result_->rows.size()==1&&transaction_result_->rows[0].status=="RULE MATCH"&&writes->count()==1);
        click(UartLabWindow::Transactions,1);check("request inspect navigates exact retained frame bytes",lab_.reports_[UartLabWindow::Transactions].find(L"51010A")!=std::wstring::npos);click(UartLabWindow::Transactions,2);check("reply inspect navigates exact retained frame bytes",lab_.reports_[UartLabWindow::Transactions].find(L"52010A")!=std::wstring::npos);
        start_lab_file(UartLabWindow::Sessions,4,[](const std::atomic<bool>& cancel)->LabFileResult{while(!cancel)Sleep(1);throw std::runtime_error("Intentional cancelled session fixture.");});check("session Cancel remains enabled during file work",::IsWindowEnabled(lab_.actions_[UartLabWindow::Sessions][7]));click(UartLabWindow::Sessions,7);check("session cancel drains owned background work",pump([&]{return !lab_file_.busy();}));
        lab_.ShowWindow(SW_HIDE);if(workbench_){workbench_->ShowWindow(SW_HIDE);workbench_->shutdown();check("all owned workers drain before session fixture exits",pump([&]{return workbench_->shutdown_complete()&&!lab_file_.busy();}));}
    }catch(const std::exception& e){check(e.what(),false);record_visit=nullptr;lab_file_.cancel();recorder_.stop();disconnect();pump([&]{return !lab_file_.busy()&&!recorder_.status().active&&!disconnect_pending_;});if(workbench_){workbench_->shutdown();pump([&]{return workbench_->shutdown_complete();});}}
    return pass;
}
