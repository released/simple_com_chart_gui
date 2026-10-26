#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "runtime_serial_fixture.h"
#include "ui_metrics.h"
#include <cmath>
namespace {
struct AdvancedTxState { std::mutex mutex; std::vector<std::string> writes; DWORD thread=0; };
class AdvancedTransport : public FakeSerialTransport {
public:
    AdvancedTransport(std::shared_ptr<FakeSerialState> serial,std::shared_ptr<AdvancedTxState> tx):FakeSerialTransport(std::move(serial)),tx_(std::move(tx)){}
    size_t write_bytes(std::string_view bytes,std::wstring*) override { std::lock_guard<std::mutex> lock(tx_->mutex); tx_->writes.emplace_back(bytes); tx_->thread=GetCurrentThreadId(); return bytes.size(); }
private: std::shared_ptr<AdvancedTxState> tx_;
};
}
bool AdvancedTools::verify_layout(const std::wstring& folder) {
    std::ofstream report(std::filesystem::path(folder)/L"advanced-layout-result.txt"); bool pass=true;
    const auto check=[&](const char* name,bool ok) { report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n'; pass=pass&&ok; };
    for (const auto width:{1050,1400}) for (int page=0;page<4;++page) {
        select(page); SetWindowPos(nullptr,0,0,width,900,SWP_NOMOVE|SWP_NOZORDER); layout(); CRect client; GetClientRect(&client); bool fits=true,captions=true;
        bool visible=page_==page && TabCtrl_GetCurSel(tabs_)==page && TabCtrl_GetCurFocus(tabs_)==page;
        for (const auto& field:fields_[page]) visible=visible&&::IsWindowVisible(field.label)&&::IsWindowVisible(field.control);
        for (const auto button:actions_[page]) visible=visible&&::IsWindowVisible(button);
        check("selected page fields/buttons really visible and tab focus agrees",visible);
        for (auto child=::GetWindow(m_hWnd,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT)) if (::IsWindowVisible(child)) {
            RECT r{}; ::GetWindowRect(child,&r); ::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&r),2);
            fits=fits&&r.left>=0&&r.top>=0&&r.right<=client.right&&r.bottom<=client.bottom;
        }
        for (const auto& field:fields_[page]) { RECT r{}; ::GetClientRect(field.label,&r); captions=captions&&r.right>=control_text_width(field.label,8); }
        for (const auto button:actions_[page]) { RECT r{}; ::GetClientRect(button,&r); captions=captions&&r.right>=control_text_width(button,16); }
        check("page controls fit tested width",fits); check("page captions have measured space",captions);
        if (width==1400) check("page screenshot",save_runtime_frame(m_hWnd,std::filesystem::path(folder)/(L"advanced-page-"+std::to_wstring(page)+L".png")));
    }
    return pass;
}
bool CMainDialog::verify_advanced_tools() {
    const auto folder=std::filesystem::path(settings_path_).parent_path(); std::ofstream report(folder/L"advanced-result.txt"); bool pass=true;
    const auto check=[&](const char* name,bool ok) { report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n'; report.flush(); pass=pass&&ok; };
    const auto original_path=settings_path_; const bool original_demo=demo_; const auto original_notice=uart_notice_;
    demo_=false; settings_path_=(folder/L"advanced-fixture.ini").wstring();
    uart_notice_=[&](HWND owner,LPCWSTR,LPCWSTR,UINT flags) { check("owned cancelable advanced confirmation",owner==advanced_tools_.GetSafeHwnd() && (flags&MB_OKCANCEL)==MB_OKCANCEL); return IDOK; };
    const auto pump=[&](const std::function<bool()>& done) {
        const auto deadline=GetTickCount64()+4000;
        do { poll_serial_stop(); poll_uart_operations(); poll_advanced_tools(); flush_pending_lines(); if (done()) return true; Sleep(2); } while (GetTickCount64()<deadline); return false;
    };
    const auto click=[&](int page,size_t action) { ::SendMessageW(advanced_tools_.action_hwnd(page,action),BM_CLICK,0,0); };
    ::SetWindowTextW(edit_include_,L""); ::SetWindowTextW(edit_exclude_,L""); ::SendMessageW(combo_filter_,CB_SETCURSEL,0,0); apply_filter(false);
    show_advanced_tools(AdvancedTools::Replay); std::atomic<bool> cancel{false};
    const auto input=folder/L"offline-fixture.log"; uart_write_file(input,"fan:1\nfan:2\nfan:3",true,cancel);
    advanced_tools_.set_field(0,0,input.wstring()); advanced_tools_.set_field(0,1,L"100"); advanced_tools_.set_field(0,2,L"100"); click(0,1);
    check("actual offline Load runs in background and remains paused",pump([&]{return !replay_load_.busy();}) && replay_.data() && !replay_.playing());
    click(0,2); check("actual Play feeds parser, capture and raw Messages",pump([&]{return !replay_.playing() && !replay_io_.busy();}) && capture_.rows().size()==3 && message_page_.buffer().bytes()=="fan:1\nfan:2\nfan:3");
    OnChannelChanged(0,0); check("offline channel actions keep virtual rather than wall clock",std::abs(now_seconds()-1.2)<1e-8 && model_.get_series("fan").size()==3);
    advanced_tools_.set_field(0,3,L"1"); click(0,4); check("actual Seek resets view and pauses at requested record",capture_.rows().empty() && replay_.position()==1 && !replay_.playing());
    click(0,2); check("play after seek contains only selected suffix",pump([&]{return !replay_.playing() && !replay_io_.busy();}) && capture_.rows().size()==2 && message_page_.buffer().bytes()=="fan:2\nfan:3");
    auto display=message_page_.options(); display.clock=MessageClock::Local; message_page_.configure_display(display);
    check("synthetic offline export suppresses fabricated PC receipt clocks",message_page_.export_text()=="fan:2\r\nfan:3");
    click(0,5); check("Unload clears offline view without opening hardware",!replay_.data() && capture_.rows().empty() && advanced_idle());
    show_advanced_tools(AdvancedTools::Profiles); advanced_tools_.set_field(1,0,L"fixture-"+std::to_wstring(GetCurrentProcessId())); click(1,0);
    check("actual Save named writes portable profile without opening port",pump([&]{return !profile_job_.busy();}) && has_staged_profile_ && advanced_idle());
    auto profile=current_profile(); profile.font_points=14; profile.charts[0]="fan"; profile.include=L"fan:"; profile.startup_tab=2; profile.serial.port.device.clear(); profile.serial.port.instance_id.clear(); profile.serial.baud=230400;
    const auto profile_path=folder/L"profiles"/(advanced_tools_.field(1,0)+L".sccprofile"); uart_write_file(profile_path,encode_profile(profile),true,cancel); click(1,1);
    check("actual Load named only stages validated settings",pump([&]{return !profile_job_.busy();}) && staged_profile_.font_points==14 && message_page_.font_points()!=14);
    workflow_grid_present_=[](WorkflowGrid&)->INT_PTR{return IDCANCEL;}; click(1,4);
    check("cancelled profile confirmation leaves live preferences unchanged",!profile_job_.busy() && message_page_.font_points()!=14);
    uart_notice_=[&](HWND owner,LPCWSTR,LPCWSTR,UINT flags) { check("owned cancelable advanced confirmation",owner==advanced_tools_.GetSafeHwnd() && (flags&MB_OKCANCEL)==MB_OKCANCEL); return IDOK; };
    workflow_grid_present_=[](WorkflowGrid&)->INT_PTR{return IDOK;};
    click(1,4); check("profile commit locks settings but main close/cancel remains enabled",profile_job_.busy() && ::IsWindowEnabled(m_hWnd) && !::IsWindowEnabled(btn_apply_));
    check("actual Apply atomically persists settings and stays disconnected",pump([&]{return !profile_job_.busy();}) && ::IsWindowEnabled(m_hWnd) && active_include_==L"fan:" && message_page_.font_points()==14 && data_page_.selected_field(0)=="fan" && remembered_serial_.baud==230400 && advanced_idle());
    capture_.reset(); for (int i=0;i<3;++i) capture_.append("fan",{{"fan",i+1.0}},10+i);
    show_advanced_tools(AdvancedTools::Analysis); for (size_t i=0;i<6;++i) advanced_tools_.set_field(2,i,i==0?L"fan":i==1?L"0":i==2||i==3?L"2":i==4?L"3":L"2"); click(2,0);
    check("analysis snapshot renders A/B and reference line",save_runtime_frame(advanced_tools_.GetSafeHwnd(),folder/L"advanced-analysis.png") && advanced_tools_.analysis_plot().analysis_lines()==3 && capture_.rows().size()==3);
    RECT chart_bounds{}; ::GetClientRect(advanced_tools_.analysis_plot().hwnd(),&chart_bounds);
    ::SendMessageW(advanced_tools_.analysis_plot().hwnd(),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(chart_bounds.right/2,chart_bounds.bottom/2));
    check("actual plot mouse message moves cursor and recalculates statistics",advanced_tools_.field(2,1)!=L"0" && capture_.rows().size()==3);
    const auto cursor_before_drag=advanced_tools_.field(2,1);::SendMessageW(advanced_tools_.analysis_plot().hwnd(),WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(chart_bounds.right*3/4,chart_bounds.bottom/2));::SendMessageW(advanced_tools_.analysis_plot().hwnd(),WM_LBUTTONUP,0,MAKELPARAM(chart_bounds.right*3/4,chart_bounds.bottom/2));
    check("analysis cursor keeps dragging against the same snapshot",advanced_tools_.field(2,1)!=cursor_before_drag && advanced_tools_.analysis_snapshot().rows.size()==3);
    click(2,1); save_runtime_frame(advanced_tools_.GetSafeHwnd(),folder/L"advanced-analysis-clear.png"); check("clear cursors removes analysis overlay only",advanced_tools_.analysis_plot().analysis_lines()==0 && capture_.rows().size()==3);
    auto original_transport=std::move(serial_mgr_); auto serial=std::make_shared<FakeSerialState>(); auto tx=std::make_shared<AdvancedTxState>();
    serial_mgr_=std::make_unique<AdvancedTransport>(serial,tx); serial_mgr_->connect(L"fake://advanced",115200,8,0,0,nullptr);
    SerialSettings settings; settings.port={L"COM4",L"fixture",L"USB\\ADVANCED",L"fake://advanced"}; connection_.begin(settings,false); connection_.connected(settings.port); start_serial_thread();
    show_advanced_tools(AdvancedTools::Automation); advanced_tools_.set_field(3,4,L"100"); advanced_tools_.set_field(3,5,L"2"); advanced_tools_.set_field(3,10,L"ping"); click(3,0);
    check("actual Start runs finite periodic TX on the worker",pump([&]{return !automation_.active();}));
    { std::lock_guard<std::mutex> lock(tx->mutex); check("two exact periodic writes and no UI-thread transmission",tx->writes==std::vector<std::string>{"ping","ping"} && tx->thread!=GetCurrentThreadId()); }
    uart_notice_=[&](HWND,LPCWSTR,LPCWSTR,UINT){uart_operations_.cancel(); uart_operations_.begin(); return IDOK;}; click(3,0);
    check("automation confirmation revalidates connection generation",!automation_.active() && !automation_id_);
    uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};
    const auto live_rows=capture_.rows().size(); AdvancedTools::Request unload{AdvancedTools::Replay,5,{}}; OnAdvancedRequest(0,reinterpret_cast<LPARAM>(&unload));
    check("Unload with no offline source never clears live capture",capture_.rows().size()==live_rows);
    ::SendMessageW(advanced_tools_.field_hwnd(3,0),CB_SETCURSEL,2,0); advanced_tools_.set_field(3,5,L"1"); advanced_tools_.set_field(3,9,L"READY"); advanced_tools_.set_field(3,10,L"pong"); click(3,0);
    { std::lock_guard<std::mutex> lock(tx->mutex); check("auto reply arming alone sends nothing",tx->writes.size()==2); }
    { std::lock_guard<std::mutex> lock(serial->mutex); serial->raw="READY"; }
    check("raw worker RX triggers generic finite auto reply",pump([&]{return !automation_.active();}));
    { std::lock_guard<std::mutex> lock(tx->mutex); check("one exact auto reply",tx->writes.size()==3 && tx->writes.back()=="pong"); }
    ::SendMessageW(advanced_tools_.field_hwnd(3,0),CB_SETCURSEL,0,0); advanced_tools_.set_field(3,5,L"100"); advanced_tools_.set_field(3,4,L"1000"); click(3,0); disconnect();
    check("disconnect stops automation and cancels queued work",pump([&]{return !disconnect_pending_;}) && !automation_.active() && !automation_id_);
    const bool layout=advanced_tools_.verify_layout(folder.wstring());
    connection_.cancel(); serial_mgr_=std::move(original_transport); uart_notice_=original_notice; settings_path_=original_path; demo_=original_demo;
    if (advanced_tools_.GetSafeHwnd()) advanced_tools_.DestroyWindow(); load_settings(); update_connection_ui(); return pass&&layout;
}
