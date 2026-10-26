#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "packet_builder.h"
#include "settings_fields.h"

namespace {
struct Writes {std::mutex mutex;std::vector<std::string> bytes;bool partial=false;size_t count(){std::lock_guard<std::mutex> lock(mutex);return bytes.size();}};
class LabTransport:public FakeSerialTransport {
public:
    LabTransport(std::shared_ptr<FakeSerialState> state,std::shared_ptr<Writes> writes):FakeSerialTransport(std::move(state)),writes_(std::move(writes)){}
    size_t write_bytes(std::string_view bytes,std::wstring* error)override{std::lock_guard<std::mutex> lock(writes_->mutex);writes_->bytes.emplace_back(bytes);if(writes_->partial){writes_->partial=false;if(error)*error=L"Fixture partial write";return bytes.size()-1;}return bytes.size();}
private:std::shared_ptr<Writes> writes_;
};
}
bool CMainDialog::verify_uart_lab() {
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"uart-lab-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+5000;do{MSG m{};unsigned count=0;while(count++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto click=[&](int page,int action){lab_.select(page);::SendMessageW(lab_.actions_[page][action],BM_CLICK,0,0);};
    SendMessage(WM_COMMAND,ID_UART_LAB);check("actual UART menu creates empty laboratory without opening COM",lab_.GetSafeHwnd()&&!serial_mgr_->is_connected());
    lab_.SetWindowPos(nullptr,0,0,700,520,SWP_NOMOVE|SWP_NOZORDER);
    for(int p=0;p<UartLabWindow::PageCount;++p){lab_.select(p);bool visible=true,widths=true;CRect area;lab_.viewport_.GetClientRect(&area);
        for(const auto& field:lab_.fields_[p]){visible=visible&&::IsWindowVisible(field.input)&&reinterpret_cast<HFONT>(::SendMessageW(field.input,WM_GETFONT,0,0))==lab_.font_&&reinterpret_cast<HFONT>(::SendMessageW(field.label,WM_GETFONT,0,0))==lab_.font_;CRect r;::GetWindowRect(field.input,&r);lab_.viewport_.ScreenToClient(&r);widths=widths&&r.left>=0&&r.right<=area.right;}
        lab_.OnVScroll(SB_BOTTOM,0,nullptr);check(("page "+std::to_string(p)+" has live inputs, bounded widths and scroll access").c_str(),visible&&widths&&lab_.scroll_>=0);
    }
    lab_.SetWindowPos(nullptr,0,0,1040,820,SWP_NOMOVE|SWP_NOZORDER);
    lab_.field(UartLabWindow::Packet,2,L"AA {address:u8} {target:u16le} {length:u8}");lab_.field(UartLabWindow::Packet,3,L"address,1,0,255\r\ntarget,201,0,65535");click(UartLabWindow::Packet,0);
    check("actual Preview computes independent parameters without TX",lab_.reports_[UartLabWindow::Packet].find(L"AA01C90005")!=std::wstring::npos&&!serial_mgr_->is_connected());
    check("packet page screenshot",save_runtime_frame(lab_.m_hWnd,folder/L"lab-packet.png"));
    click(UartLabWindow::Parser,0);const auto before=encode_extraction(extractor_.config());lab_.field(UartLabWindow::Parser,7,L"Input,Expected,Seconds\r\nx:1,x:1,\r\nx:2,x:3,\r\n");click(UartLabWindow::Parser,1);
    check("actual parser cases run in background",pump([&]{return !lab_file_.busy();})&&lab_.reports_[UartLabWindow::Parser].find(L"Passed 1, failed 1")!=std::wstring::npos&&encode_extraction(extractor_.config())==before);
    auto state=std::make_shared<FakeSerialState>();auto writes=std::make_shared<Writes>();const std::vector<SerialPortInfo> ports{{L"COM91",L"Lab fake A",L"LAB_A",L"fake-A"}};
    port_scanner_=[ports](bool* ok){if(ok)*ok=true;return ports;};transport_factory_=[state,writes]{return std::make_unique<LabTransport>(state,writes);};
    uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};scan_ports();::SendMessageW(combo_port_,CB_SETCURSEL,0,0);on_connect_toggle();check("fake A connects without a native COM open",pump([&]{return serial_mgr_->is_connected();}));
    ::SetWindowTextW(edit_include_,L"keep");::SetWindowTextW(edit_exclude_,L"skip");check("fixture filtering applies",apply_filter(false));
    lab_.field(UartLabWindow::Quality,1,L"cnt");lab_.field(UartLabWindow::Quality,2,L"256");click(UartLabWindow::Quality,0);
    {std::lock_guard<std::mutex> lock(state->mutex);state->raw="keep,cnt:254\nkeep,cnt:255\nkeep,cnt:0\nkeep,skip,cnt:8\nother,cnt:9\nkeep heartbeat\n";}
    check("actual worker feeds counters and sequence wrap with default-none charts",pump([&]{return sequences_[0].stats().samples==3;})&&sequences_[0].stats().wraps==1&&model_.get_enabled_count()==0);
    click(UartLabWindow::Diagnostics,0);const auto diag=lab_.reports_[UartLabWindow::Diagnostics];check("mode-aware diagnostic report distinguishes exclusion/include/no-values",diag.find(L"Excluded: 1")!=diag.npos&&diag.find(L"Include did not match: 1")!=diag.npos&&diag.find(L"No numeric fields: 1")!=diag.npos);
    show_workbench();auto& wb=*workbench_;std::wstring error;
    const std::vector<std::wstring> config{L"Enabled",L"AA",L"4",L"0",L"1",L"Little",L"0",L"None",L"0",L"Little",L"@codec,COBS\n@id,1,1,Little\n@case,1,value,2,u16le",L""};
    check("extended native schema applies and persists in fixture INI",wb.apply_binary(0,config,error));
    const auto wire=wire_encode(std::string("\xAA\x01\xC9\0",4),WireCodec::Cobs);{std::lock_guard<std::mutex> lock(state->mutex);state->raw=wire;}
    check("real receive dispatch decodes COBS/message ID into numeric channels",pump([&]{double v=0;return model_.latest_value("A.value",v)&&v==201;}));
    check("wire Messages still contains original encoded bytes",message_page_.buffer().bytes().find(wire)!=std::string::npos);
    const auto profile=current_profile();check("profiles contain schema and sequence preferences, exclude packet bodies",profile.workspace_settings.count("Lab.Sequence.A/Configuration")&&profile.workspace_settings.at("Workbench.Binary.A/Configuration").find("AA {address")==std::string::npos);
    click(UartLabWindow::Packet,1);check("confirmed packet reserves existing TX path",pump([&]{return writes->count()==1&&!wb.tx_reserved(0);}));
    {std::lock_guard<std::mutex> lock(writes->mutex);check("actual target receives exact immutable assembled bytes",writes->bytes.size()==1&&uart_hex(writes->bytes[0])=="AA01C90005");writes->partial=true;}
    click(UartLabWindow::Packet,1);check("partial packet write stops visibly without retry",pump([&]{return writes->count()==2&&!wb.tx_reserved(0);})&&wb.runs_[0].status.find(L"failed/partial")!=std::wstring::npos);
    uart_notice_=[&](HWND,LPCWSTR,LPCWSTR,UINT){disconnect();return IDOK;};click(UartLabWindow::Packet,1);check("connection change during confirmation prevents TX",pump([&]{return !disconnect_pending_;})&&writes->count()==2);
    std::atomic<bool> cancel{false};const auto raw=folder/L"lab-recording.bin",idx=folder/L"lab-recording.idx",slice=folder/L"lab-slice.bin";
    uart_write_file(raw,"ABCD",false,cancel);uart_write_file(idx,"SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,2,1000,RX\r\n2,2,1010,RX\r\n",false,cancel);
    lab_.field(UartLabWindow::Recording,0,raw.wstring());lab_.field(UartLabWindow::Recording,3,L"1");lab_.field(UartLabWindow::Recording,4,L"2");lab_.field(UartLabWindow::Recording,5,slice.wstring());click(UartLabWindow::Recording,2);
    check("native range export completes in background",pump([&]{return !lab_file_.busy();})&&uart_read_file(slice,100,cancel)=="CD");
    click(UartLabWindow::Recording,1);check("native recording check reports matching structure",pump([&]{return !lab_file_.busy();})&&lab_.reports_[UartLabWindow::Recording].find(L"complete/matching")!=std::wstring::npos);
    model_.reset();capture_.reset();accept_numeric("flags:0",{{"flags",0}},10,UINT64_MAX,0,0,1);accept_numeric("flags:1",{{"flags",1}},11,UINT64_MAX,0,0,1);accept_numeric("flags:0",{{"flags",0}},12,UINT64_MAX,0,0,2);accept_numeric("flags:1",{{"flags",1}},13,UINT64_MAX,0,0,2);
    sync_channels();lab_.field(UartLabWindow::Digital,0,L"Ready,flags,0,1");click(UartLabWindow::Digital,0);const auto history=digital_history(capture_.snapshot(),digital_lanes_[0],30,2);
    check("digital history preserves reconnect gap and selection independence",history.segments.size()==2&&history.segments[0].last==1&&history.segments[1].first==2&&model_.get_enabled_count()==0);
    const auto paint_started=GetTickCount64();check("digital child paint settles before evidence capture",pump([&]{return GetTickCount64()-paint_started>=120;}));lab_.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    check("digital history screenshot",save_runtime_frame(lab_.m_hWnd,folder/L"lab-digital.png"));click(UartLabWindow::Digital,2);check("Reset to none persists empty lanes",digital_lanes_.empty());
    wb.ShowWindow(SW_HIDE);lab_.ShowWindow(SW_HIDE);wb.shutdown();check("owned workers drain before fixture exit",pump([&]{return wb.shutdown_complete()&&!lab_file_.busy();}));return pass;
}
