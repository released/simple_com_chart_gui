#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "runtime_serial_fixture.h"
#include "ui_metrics.h"
#include <cmath>
namespace {
struct OptimizationTx {std::mutex mutex;std::string bytes;std::vector<uint64_t> times;bool partial=false;};
class OptimizationTransport:public FakeSerialTransport {
public:
    OptimizationTransport(std::shared_ptr<FakeSerialState> serial,std::shared_ptr<OptimizationTx> tx):FakeSerialTransport(serial),tx_(tx){}
    size_t write_bytes(std::string_view bytes,std::wstring* error) override {std::lock_guard<std::mutex> lock(tx_->mutex);tx_->times.push_back(GetTickCount64());const auto n=tx_->partial?std::min<size_t>(1,bytes.size()):bytes.size();tx_->bytes.append(bytes.substr(0,n));if(tx_->partial&&error) *error=L"Simulated partial write";return n;}
private:std::shared_ptr<OptimizationTx> tx_;
};
}
bool AdvancedTools::verify_small_layout(const std::wstring& folder) {
    std::ofstream output(std::filesystem::path(folder)/L"optimization-layout-result.txt");bool pass=true;
    for(const auto width:{800,1050}) for(int page=0;page<PageCount;++page) {
        select(page);SetWindowPos(nullptr,0,0,width,620,SWP_NOMOVE|SWP_NOZORDER);layout();
        bool fits=true;CRect client;GetClientRect(&client);
        for(auto& f:fields_[page]) {RECT r{};::GetClientRect(f.label,&r);fits=fits&&r.right>=control_text_width(f.label,8);::GetWindowRect(f.control,&r);::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&r),2);fits=fits&&r.right<=client.right&&r.left>=0;}
        OnVScroll(SB_BOTTOM,0,nullptr);RECT r{};::GetWindowRect(status_,&r);::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&r),2);fits=fits&&r.top>=0&&r.bottom<=client.bottom;
        output<<(fits?"PASS: ":"FAIL: ")<<"scrollable page "<<page<<" width "<<width<<" scroll "<<scroll_<<" / "<<scroll_limit_<<'\n';pass=pass&&fits;
        if(width==800) {pass=save_runtime_frame(m_hWnd,std::filesystem::path(folder)/(L"optimization-page-"+std::to_wstring(page)+L"-bottom.png"))&&pass;OnVScroll(SB_TOP,0,nullptr);pass=save_runtime_frame(m_hWnd,std::filesystem::path(folder)/(L"optimization-page-"+std::to_wstring(page)+L"-top.png"))&&pass;}
    }return pass;
}
bool CMainDialog::verify_optimizations() {
    const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"optimization-result.txt");bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';report.flush();pass=pass&&ok;};
    check("main window actually has Per-Monitor V2 awareness",AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(m_hWnd),DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)!=FALSE);
    const auto original_demo=demo_;demo_=false;const auto original_notice=uart_notice_;const auto original_ini=settings_path_;settings_path_=(folder/L"optimization-fixture.ini").wstring();
    const auto pump=[&](const std::function<bool()>& done){const auto end=GetTickCount64()+5000;do{poll_uart_operations();poll_advanced_tools();poll_serial_stop();flush_pending_lines();if(done()) return true;Sleep(2);}while(GetTickCount64()<end);return false;};
    const auto click=[&](int page,int action){::SendMessageW(advanced_tools_.action_hwnd(page,action),BM_CLICK,0,0);};
    show_advanced_tools(AdvancedTools::Extraction);
    const auto parser=AdvancedTools::Extraction;::SendMessageW(advanced_tools_.field_hwnd(parser,0),CB_SETCURSEL,1,0);advanced_tools_.set_field(parser,1,L"time,adc");advanced_tools_.set_field(parser,6,L"time");advanced_tools_.set_field(parser,7,L"1,3300");click(parser,0);
    check("Preview does not apply parser",extractor_.config().mode==ExtractionConfig::KeyValue);
    ::SetWindowTextW(edit_include_,L"[");::SetWindowTextW(edit_limit_,L"0");::SendMessageW(combo_filter_,CB_SETCURSEL,1,0);click(parser,1);
    check("Apply parser persists CSV without changing raw view",extractor_.config().mode==ExtractionConfig::Csv && capture_.rows().empty());
    check("parser Apply retains confirmed filters despite invalid unapplied edits",active_include_.empty()&&!active_regex_&&line_filter_.matches("anything"));
    wchar_t draft[16]{};::GetWindowTextW(edit_limit_,draft,16);check("parser Apply leaves unapplied filter edits untouched",std::wstring(draft)==L"0");
    ::SetWindowTextW(edit_include_,L"");::SetWindowTextW(edit_limit_,L"256");::SendMessageW(combo_filter_,CB_SETCURSEL,0,0);
    message_page_.reset();
    source_lines_.clear_partial();const std::string raw="1,3300\n2,3400\n";
    {std::lock_guard<std::mutex> lock(pending_mutex_);pending_.raw.append(raw);for(const auto& line:source_lines_.feed(raw)) pending_.lines.push_back({line.text,100,line.offset,line.count});pending_.source_end=source_lines_.end();}flush_pending_lines();
    check("CSV values/time feed charts and source mapping",capture_.rows().size()==2 && capture_.rows()[1]->seconds==1 && capture_.rows()[0]->source_offset!=UINT64_MAX);
    const auto writable_ini=settings_path_;settings_path_=(folder/L"missing-parent"/L"cannot-save.ini").wstring();
    ::SendMessageW(advanced_tools_.field_hwnd(parser,0),CB_SETCURSEL,0,0);click(parser,1);
    check("failed parser save preserves active parser, clock and captured rows",extractor_.config().mode==ExtractionConfig::Csv&&capture_.rows().size()==2&&extracted_clock_&&extracted_origin_==1);
    settings_path_=writable_ini;::SendMessageW(advanced_tools_.field_hwnd(parser,0),CB_SETCURSEL,1,0);
    show_advanced_tools(AdvancedTools::Channels);const auto channels=AdvancedTools::Channels;
    advanced_tools_.set_field(channels,0,L"adc");advanced_tools_.set_field(channels,1,L"Voltage");advanced_tools_.set_field(channels,2,L"V");advanced_tools_.set_field(channels,3,L"2");advanced_tools_.set_field(channels,4,L"0.001");click(channels,1);
    double raw_value=0;CaptureStore::value(*capture_.rows()[0],1,raw_value);
    check("Display transform leaves capture/raw unchanged",std::abs(model_.get_series("adc").back().v-3.4)<1e-9 && raw_value==1 && message_page_.buffer().bytes()==raw);
    // Capture keys are deterministic alphabetical: adc first, time second.
    CaptureStore::value(*capture_.rows()[0],0,raw_value);check("raw ADC remains unscaled",raw_value==3300);
    TabCtrl_SetCurSel(tabs_,kDataTab);switch_page();
    const auto data_panel=::FindWindowExW(data_page_.hwnd(),nullptr,L"ChannelPanelWnd",nullptr);
    ::SendMessageW(::GetDlgItem(data_panel,1),BM_CLICK,0,0);
    data_page_.select_field(0,"adc");wchar_t displayed[80]{};ListView_GetItemText(data_page_.table(),0,2,displayed,80);
    check("data table and independent chart apply display preferences",std::wstring(displayed)==L"3.30 V" && std::abs(data_page_.chart_model(0).get_series("adc").front().v-3.3)<1e-9);
    model_.update_from_kv({{"hidden",7}},102);sync_channels();
    ::SendMessageW(::GetDlgItem(channel_panel_.hwnd(),1),BM_CLICK,0,0);
    const auto list=::GetDlgItem(channel_panel_.hwnd(),3);::SetWindowTextW(::GetDlgItem(channel_panel_.hwnd(),4),L"Voltage");
    check("channel search matches alias",ListView_GetItemCount(list)==1);::SendMessageW(::GetDlgItem(channel_panel_.hwnd(),2),BM_CLICK,0,0);
    check("None affects search results only",!model_.is_enabled("adc") && model_.is_enabled("hidden"));::SetWindowTextW(::GetDlgItem(channel_panel_.hwnd(),4),L"");
    check("cleared search restores stable key selection",channel_panel_.get_checkbox_state_map().at("hidden") && !channel_panel_.get_checkbox_state_map().at("adc"));
    TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();OnSourceNavigate(0,reinterpret_cast<LPARAM>(capture_.rows()[0].get()));DWORD selection=0;::SendMessageW(message_page_.view(),EM_GETSEL,reinterpret_cast<WPARAM>(&selection),0);
    check("table navigation selects exact retained RX",selection==0 && !data_page_.follows());message_page_.locate_cursor_source();check("Messages caret locates table row",TabCtrl_GetCurSel(tabs_)==kDataTab);
    const uint64_t second_offset=capture_.rows()[1]->source_offset;OnLocateSource(0,reinterpret_cast<LPARAM>(&second_offset));check("LF-only next-row start is never mapped to previous row",ListView_GetNextItem(data_page_.table(),-1,LVNI_SELECTED)==1);
    TabCtrl_SetCurSel(tabs_,kChartTab);switch_page();plot_view_.return_to_latest();const auto before=plot_view_.view_window();RECT plot{};::GetClientRect(plot_view_.hwnd(),&plot);POINT point{plot.right/2,plot.bottom/2};::ClientToScreen(plot_view_.hwnd(),&point);
    ::SendMessageW(plot_view_.hwnd(),WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),MAKELPARAM(point.x,point.y));check("wheel zoom pauses follow without changing capture",plot_view_.view_window()<before && !plot_view_.following_latest() && capture_.rows().size()==2);
    check("fixed Y accepts valid bounds and rejects reversed",plot_view_.set_fixed_y(true,-1,10) && plot_view_.y_range()==std::make_pair(-1.0,10.0) && !plot_view_.set_fixed_y(true,10,-1));plot_view_.set_fixed_y(false);plot_view_.return_to_latest();check("return to latest restores follow",plot_view_.following_latest());
    update_source_status();wchar_t status[1024]{};::GetWindowTextW(source_status_,status,1024);check("persistent source/provenance/loss state",std::wstring(status).find(L"field seconds")!=std::wstring::npos && std::wstring(status).find(L"Lost:")!=std::wstring::npos);
    auto original_transport=std::move(serial_mgr_);auto serial=std::make_shared<FakeSerialState>();auto tx=std::make_shared<OptimizationTx>();serial_mgr_=std::make_unique<OptimizationTransport>(serial,tx);serial_mgr_->connect(L"fake://optimization",115200,8,0,0,nullptr);
    SerialSettings settings;settings.port={L"COM8",L"fixture",L"USB\\OPTIMIZATION",L"fake://optimization"};connection_.begin(settings,false);connection_.connected(settings.port);start_serial_thread();
    std::atomic<bool> cancel{false};const auto path=folder/L"file-tx.bin";const std::string payload("abc\0def",7);uart_write_file(path,payload,true,cancel);show_advanced_tools(AdvancedTools::FileTx);
    advanced_tools_.set_field(AdvancedTools::FileTx,0,path.wstring());advanced_tools_.set_field(AdvancedTools::FileTx,3,L"2");advanced_tools_.set_field(AdvancedTools::FileTx,4,L"20");
    uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDCANCEL;};click(AdvancedTools::FileTx,1);check("cancelled file confirmation never queues bytes",!file_tx_busy());
    uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};click(AdvancedTools::FileTx,1);click(AdvancedTools::FileTx,2);pump([&]{return !file_tx_job_.busy();});
    {std::lock_guard<std::mutex> lock(tx->mutex);check("file pause before first chunk sends nothing",tx->bytes.empty());}
    click(AdvancedTools::FileTx,3);check("file TX completes through worker",pump([&]{return !file_tx_busy();}));{std::lock_guard<std::mutex> lock(tx->mutex);check("file TX is exact binary",tx->bytes==payload);bool paced=tx->times.size()==4;for(size_t i=1;i<tx->times.size();++i) paced=paced&&tx->times[i]-tx->times[i-1]>=20;check("file TX honors configured chunk pacing",paced);tx->bytes.clear();tx->partial=true;}
    click(AdvancedTools::FileTx,1);check("partial file write stops without retry",pump([&]{return !file_tx_busy();}));{std::lock_guard<std::mutex> lock(tx->mutex);check("partial prefix reported once",tx->bytes.size()==1);tx->bytes.clear();tx->partial=false;}
    message_page_.set_keyboard_available(true,100,true);check("new connection enables keyboard TX by default",message_page_.keyboard_armed());message_page_.arm_keyboard(false);message_page_.set_keyboard_available(true,100,true);check("manual keyboard disable persists within the same connection",!message_page_.keyboard_armed());
    message_page_.set_keyboard_available(false,100,true);message_page_.set_keyboard_available(true,100,true);check("temporary TX suspension does not rearm manually disabled keyboard",!message_page_.keyboard_armed());
    message_page_.set_keyboard_available(true,101,true);check("next connection enables keyboard TX again",message_page_.keyboard_armed());
    message_page_.set_keyboard_available(true,uart_operations_.generation(),true);message_page_.arm_keyboard(false);
    file_tx_active_=true;poll_uart_operations();file_tx_active_=false;poll_uart_operations();
    check("actual file-TX availability gating preserves manual keyboard disable",!message_page_.keyboard_armed());message_page_.arm_keyboard(true);
    check("local TX echo defaults on",message_page_.local_echo());message_page_.echo_tx(std::string("Hi\r\xFF",4));wchar_t echo[256]{};::GetWindowTextW(message_page_.status_control(),echo,256);check("keyboard echo is readable ASCII with escaped control/non-ASCII bytes",std::wstring(echo).find(L"Hi\\r\\xFF")!=std::wstring::npos&&std::wstring(echo).find(L"48 69")==std::wstring::npos);MessagePage::KeyboardTx key{"\x03",false};OnKeyboardTx(uart_operations_.generation(),reinterpret_cast<LPARAM>(&key));
    check("keyboard Ctrl+C byte uses TX worker, not Break",pump([&]{std::lock_guard<std::mutex> lock(tx->mutex);return tx->bytes==std::string("\x03",1);}));
    MessagePage::KeyboardTx paste{"first\rsecond",true};uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDCANCEL;};OnKeyboardTx(uart_operations_.generation(),reinterpret_cast<LPARAM>(&paste));
    {std::lock_guard<std::mutex> lock(tx->mutex);check("cancelled multiline paste sends nothing",tx->bytes.size()==1);}
    uart_notice_=[&](HWND owner,LPCWSTR text,LPCWSTR,UINT flags){check("paste preview warns and has owned cancelable confirmation",owner==m_hWnd && std::wstring(text).find(L"Multiple lines")!=std::wstring::npos && (flags&MB_OKCANCEL)==MB_OKCANCEL);uart_operations_.cancel();uart_operations_.begin();return IDOK;};OnKeyboardTx(uart_operations_.generation(),reinterpret_cast<LPARAM>(&paste));
    {std::lock_guard<std::mutex> lock(tx->mutex);check("paste confirmation revalidates connection generation",tx->bytes.size()==1);}
    const auto stale=uart_operations_.generation();disconnect();check("disconnect drains workers and file TX",pump([&]{return !disconnect_pending_;}) && !file_tx_busy());OnKeyboardTx(stale,reinterpret_cast<LPARAM>(&key));
    {std::lock_guard<std::mutex> lock(tx->mutex);check("stale keyboard generation cannot replay TX",tx->bytes.size()==1);}
    check("all advanced pages reachable at small height",advanced_tools_.verify_small_layout(folder.wstring()));
    if(advanced_tools_.GetSafeHwnd()) advanced_tools_.DestroyWindow();serial_mgr_=std::move(original_transport);connection_.cancel();uart_notice_=original_notice;settings_path_=original_ini;demo_=original_demo;load_settings();
    update_source_status();
    ShowWindow(SW_RESTORE);SetWindowPos(nullptr,0,0,1000,740,SWP_NOMOVE|SWP_NOZORDER);
    bool combo_captions=true;for(const auto combo:{combo_baud_,combo_data_,combo_parity_,combo_stop_,combo_auto_,combo_time_,combo_filter_}) {RECT bounds{};::GetClientRect(combo,&bounds);combo_captions=combo_captions && bounds.right>=control_text_width(combo,32);}
    check("serial/refresh/filter combo captions reserve arrow and text",combo_captions);
    check("main small-width screenshot",save_runtime_frame(m_hWnd,folder/L"optimization-main.png"));return verify_review_pipeline()&&pass;
}
