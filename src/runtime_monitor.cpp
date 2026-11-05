#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "uart_workbench.h"
#include "uart_files.h"
#include "ui_metrics.h"
#include <richedit.h>
#include <set>
namespace {
thread_local std::function<void(HWND)>* visit_form=nullptr;
thread_local bool visit_failed=false;
VOID CALLBACK visit_timer(HWND h,UINT,UINT_PTR id,DWORD){KillTimer(h,id);if(visit_form){auto callback=visit_form;visit_form=nullptr;try{(*callback)(h);}catch(...){visit_failed=true;}}if(IsWindow(h))PostMessageW(h,WM_COMMAND,IDCANCEL,0);}
LRESULT CALLBACK visit_hook(int code,WPARAM w,LPARAM l){if(code==HCBT_ACTIVATE&&visit_form){auto h=reinterpret_cast<HWND>(w);DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId()&&GetDlgItem(h,4000))SetTimer(h,32677,50,visit_timer);}return CallNextHookEx(nullptr,code,w,l);}
std::wstring value(HWND h){return ui::control_value(h);}
bool contains(HWND outer,HWND inner){RECT a{},b{};GetClientRect(outer,&a);GetWindowRect(inner,&b);MapWindowPoints(nullptr,outer,reinterpret_cast<POINT*>(&b),2);return b.left>=0&&b.top>=0&&b.right<=a.right+1&&b.bottom<=a.bottom+1&&b.bottom>b.top;}
struct IniAttributes {std::wstring path;DWORD original=INVALID_FILE_ATTRIBUTES;explicit IniAttributes(std::wstring p):path(std::move(p)),original(GetFileAttributesW(path.c_str())){}~IniAttributes(){if(original!=INVALID_FILE_ATTRIBUTES)SetFileAttributesW(path.c_str(),original);}};
}
bool CMainDialog::verify_monitor_workflows(){
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"monitor-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool result){report<<(result?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&result;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+8000;do{MSG m{};unsigned n=0;while(n++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    LRESULT modal_result=0;
    const auto modal=[&](WPARAM action,std::function<void(HWND)> visit){visit_failed=false;visit_form=&visit;const auto hook=SetWindowsHookExW(WH_CBT,visit_hook,nullptr,GetCurrentThreadId());if(!hook){visit_form=nullptr;return false;}modal_result=OnMessageFeature(action,0);UnhookWindowsHookEx(hook);const bool visited=visit_form==nullptr;visit_form=nullptr;return visited&&!visit_failed;};
    try{
        TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();message_page_.reset();message_page_.append(std::string("A\r\r\n\0Z",6));message_page_.refresh();
        const auto raw=message_page_.buffer().bytes();const auto rules=L"red,line,0,error\r\nyellow,word,1,WARN";
        check("actual keyword dialog applies validated rules",modal(MessagePage::EditKeywords,[&](HWND h){auto input=::GetDlgItem(::GetDlgItem(h,4000),4100);::SetWindowTextW(input,rules);::SendMessageW(h,WM_COMMAND,IDOK,0);}));
        check("rule Apply persists exact rules without changing RX",message_page_.keyword_rules_.size()==2&&message_page_.buffer().bytes()==raw);
        check("invalid actual Apply remains open and keeps rules",modal(MessagePage::EditKeywords,[&](HWND h){auto input=::GetDlgItem(::GetDlgItem(h,4000),4100);::SetWindowTextW(input,L"invalid");::SendMessageW(h,WM_COMMAND,IDOK,0);check("invalid form still exists",::IsWindow(h)&&message_page_.keyword_rules_.size()==2);}));
        {IniAttributes attributes(settings_path_);check("owned INI read-only failure fixture",attributes.original!=INVALID_FILE_ATTRIBUTES&&SetFileAttributesW(settings_path_.c_str(),attributes.original|FILE_ATTRIBUTE_READONLY));
            check("rule save failure leaves actual editor open",modal(MessagePage::EditKeywords,[&](HWND h){auto input=::GetDlgItem(::GetDlgItem(h,4000),4100);::SetWindowTextW(input,L"cyan,word,1,changed");::SendMessageW(h,WM_COMMAND,IDOK,0);check("failed save keeps active rules",::IsWindow(h)&&message_page_.keyword_definition()==rules);}));
            const auto smart=message_page_.smart_scroll_;::SendMessageW(message_page_.hwnd(),WM_COMMAND,152,0);check("failed smart-scroll save retains active choice",message_page_.smart_scroll_==smart);
            const auto split=split_logs_;SendMessage(WM_COMMAND,ID_MONITOR_SPLIT);check("failed split save retains active layout",split_logs_==split);
        }
        check("inspector uses immutable raw snapshot",modal(MessagePage::InspectBytes,[&](HWND h){message_page_.append("later");::SendMessageW(h,WM_COMMAND,4500,0);bool exact=false;::EnumChildWindows(h,[](HWND child,LPARAM out)->BOOL{if((::GetWindowLongPtrW(child,GWL_STYLE)&ES_READONLY)&&value(child).find(L"41 0D 0D 0A 00 5A")!=std::wstring::npos&&value(child).find(L"6C 61 74 65 72")==std::wstring::npos)*reinterpret_cast<bool*>(out)=true;return TRUE;},reinterpret_cast<LPARAM>(&exact));check("actual inspector preview shows exact NUL and CRCRLF",exact);}));
        check("inspector Close never requires a valid edited range",modal(MessagePage::InspectBytes,[&](HWND h){auto input=::GetDlgItem(::GetDlgItem(h,4000),4101);::SetWindowTextW(input,L"0");::SendMessageW(h,WM_COMMAND,4500,0);::SendMessageW(h,WM_COMMAND,IDOK,0);})&&modal_result==IDOK);
        message_page_.reset();std::string rows;for(int i=0;i<1200;++i)rows+="value:"+std::to_string(i)+"\r\n";message_page_.append(rows);message_page_.refresh();
        ::SetFocus(btn_scan_);::SendMessageW(message_page_.hwnd(),WM_COMMAND,152,0);::SendMessageW(message_page_.view(),WM_VSCROLL,SB_TOP,0);
        check("smart user scroll suspends follow without focus change",!message_page_.follows()&&message_page_.smart_suspended_&&::GetFocus()==btn_scan_);
        message_page_.append("value:1200\r\n");message_page_.refresh();check("browsing reports new bytes",value(message_page_.inspection_info_).find(L"12")!=std::wstring::npos&&::IsWindowEnabled(message_page_.latest_));
        ::SendMessageW(message_page_.view(),WM_VSCROLL,SB_BOTTOM,0);check("manual bottom resumes smart suspension",message_page_.follows()&&!message_page_.smart_suspended_);
        ::SetFocus(message_page_.view());::SendMessageW(message_page_.view(),WM_VSCROLL,SB_LINEUP,0);RECT wheel_area{};::GetWindowRect(message_page_.view(),&wheel_area);for(int n=0;n<4&&!message_page_.follows();++n)::SendMessageW(message_page_.view(),WM_MOUSEWHEEL,MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA)),MAKELPARAM(wheel_area.left+5,wheel_area.top+5));pump([&]{return message_page_.follows();});SCROLLINFO wheel_info{sizeof(wheel_info),SIF_ALL};::GetScrollInfo(message_page_.view(),SB_VERT,&wheel_info);report<<"wheel_probe: pos="<<wheel_info.nPos<<" max="<<wheel_info.nMax<<" page="<<wheel_info.nPage<<" first_line="<<::SendMessageW(message_page_.view(),EM_GETFIRSTVISIBLELINE,0,0)<<'\n';check("native wheel down resumes at bottom with actual body font",message_page_.follows());
        ::SendMessageW(message_page_.follow_,BM_SETCHECK,BST_UNCHECKED,0);::SendMessageW(message_page_.hwnd(),WM_COMMAND,MAKEWPARAM(2,BN_CLICKED),reinterpret_cast<LPARAM>(message_page_.follow_));::SendMessageW(message_page_.view(),WM_VSCROLL,SB_BOTTOM,0);check("explicit follow off does not auto resume",!message_page_.follows());
        ::SendMessageW(message_page_.hwnd(),WM_COMMAND,153,0);check("Return to latest clears backlog",message_page_.follows()&&message_page_.seen_total_==message_page_.received_total_);
        ::SendMessageW(message_page_.hwnd(),WM_COMMAND,122,0);message_page_.append("paused\r\n");check("paused receive updates backlog without rendering",value(message_page_.inspection_info_).find(L"8")!=std::wstring::npos);::SendMessageW(message_page_.hwnd(),WM_COMMAND,153,0);check("Return to latest resumes paused display",!message_page_.paused_&&message_page_.rendered_text_.find(L"paused")!=std::wstring::npos);
        std::vector<KeywordRule> parsed;std::wstring error;parse_keyword_rules(rules,parsed,error);message_page_.set_keywords(rules,parsed);message_page_.reset();message_page_.append("ERROR detected\r\nWARN here\r\n");message_page_.refresh();
        CHARFORMAT2W color{};color.cbSize=sizeof(color);::SendMessageW(message_page_.view(),EM_SETSEL,0,5);::SendMessageW(message_page_.view(),EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&color));check("actual RichEdit applies rule color",color.crTextColor==RGB(255,90,90));
        ::SetWindowTextW(message_page_.search_,L"ERROR");::SendMessageW(message_page_.highlight_,BM_SETCHECK,BST_CHECKED,0);message_page_.options_command(MessagePage::kHighlight);::SendMessageW(message_page_.view(),EM_SETSEL,0,5);::SendMessageW(message_page_.view(),EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&color));check("Find highlight wins over keyword colors after streaming",color.crTextColor==RGB(0,0,0)&&color.crBackColor==RGB(230,210,70));::SetWindowTextW(message_page_.search_,L"");::SendMessageW(message_page_.highlight_,BM_SETCHECK,BST_UNCHECKED,0);message_page_.options_command(MessagePage::kHighlight);
        const auto captured=message_page_.buffer().bytes();SetWindowPos(nullptr,0,0,1500,1050,SWP_NOMOVE|SWP_NOZORDER);TabCtrl_SetCurSel(tabs_,kChartTab);switch_page();SendMessage(WM_COMMAND,ID_MONITOR_SPLIT);
        check("split shows actual chart and Messages together",split_logs_&&split_actual_&&::IsWindowVisible(plot_view_.hwnd())&&::IsWindowVisible(message_page_.hwnd()));
        check("split children fit parent",contains(m_hWnd,plot_view_.hwnd())&&contains(m_hWnd,message_page_.hwnd())&&contains(message_page_.hwnd(),message_page_.view()));
        for(const int id:{ID_MONITOR_SPLIT_SMALL,ID_MONITOR_SPLIT_HALF,ID_MONITOR_SPLIT_LARGE}){SendMessage(WM_COMMAND,id);check("split ratio keeps visible pane bounds",split_actual_&&contains(message_page_.hwnd(),message_page_.view()));}
        TabCtrl_SetCurSel(tabs_,kDataTab);switch_page();check("4 Charts and raw log coexist",::IsWindowVisible(data_page_.hwnd())&&::IsWindowVisible(message_page_.hwnd()));TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();check("Messages tab remains full view",!split_actual_&&::IsWindowVisible(message_page_.hwnd())&&!::IsWindowVisible(plot_view_.hwnd()));
        check("layout never clears RX",message_page_.buffer().bytes()==captured);
        show_monitor_center();check("actual recent/event window creates",::IsWindow(monitor_center_.m_hWnd));
        const auto file=folder/L"monitor_source.bin";{std::ofstream out(file,std::ios::binary);out<<"test:1\n";}remember_monitor_resource({ResourceKind::Recording,L"測試 UART",file.wstring(),(folder/L"chosen.log").wstring()});
        auto resource=monitor_recent_.front();MonitorCenter::Request request;request.resource=resource;request.action=MonitorCenter::Stage;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));check("recent recording only stages path",advanced_tools_.field(AdvancedTools::Replay,0)==file.wstring()&&!replay_.data()&&!replay_load_.busy()&&!serial_mgr_->is_connected());
        request.action=MonitorCenter::Rename;request.name=L"Board A";OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));check("rename persists display label",monitor_recent_.front().name==L"Board A");load_monitor_preferences();check("recent list survives INI reload",monitor_recent_.front().name==L"Board A"&&monitor_recent_.front().related==(folder/L"chosen.log").wstring());
        request.action=MonitorCenter::Rename;request.name=L"";check("invalid label does not replace saved list",OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request))==0&&monitor_recent_.front().name==L"Board A");
        const auto retained=message_page_.buffer().bytes();
        for(const auto kind:{ResourceKind::Profile,ResourceKind::View,ResourceKind::Session}){
            const auto path=folder/(kind==ResourceKind::Profile?L"monitor-profile.sccprofile":kind==ResourceKind::View?L"monitor-view.sccview":L"monitor-session.sccsession");
            {std::ofstream out(path,std::ios::binary);if(kind==ResourceKind::Profile){UartProfile fixture;fixture.serial.port.device=L"COM123";out<<encode_profile(fixture);}else if(kind==ResourceKind::View)out<<encode_view(current_view());else out<<"Not read until explicit Verify";}
            RecentResource recent{kind,L"Fixture resource",path.wstring()};remember_monitor_resource(recent);MonitorCenter::Request stage;stage.action=MonitorCenter::Stage;stage.resource=recent;
            OnMonitorRequest(0,reinterpret_cast<LPARAM>(&stage));check("recent staging background work completes",pump([&]{return !profile_job_.busy()&&!lab_file_.busy();}));
            if(kind==ResourceKind::Profile)check("recent profile stages without Apply",has_staged_profile_&&!serial_mgr_->is_connected());
            if(kind==ResourceKind::View)check("recent view stages without applying selections",staged_view_!=nullptr);
            if(kind==ResourceKind::Session)check("recent session only fills Verify input",lab_.values(UartLabWindow::Sessions)[1]==path.wstring()&&!replay_load_.busy());
            check("recent staging preserves raw Messages",message_page_.buffer().bytes()==retained);
            stage.action=MonitorCenter::Remove;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&stage));
        }
        RecentResource not_folder{ResourceKind::Recording,L"Not a folder",(file/L"child.log").wstring()};remember_monitor_resource(not_folder);MonitorCenter::Request open;open.resource=not_folder;open.action=MonitorCenter::Folder;check("Open folder rejects a regular-file parent without executing it",OnMonitorRequest(0,reinterpret_cast<LPARAM>(&open))==0);open.action=MonitorCenter::Remove;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&open));
        if(lab_.GetSafeHwnd())lab_.ShowWindow(SW_HIDE);if(advanced_tools_.GetSafeHwnd())advanced_tools_.ShowWindow(SW_HIDE);
        monitor_events_.clear();monitor_event(L"A",L"Recording stopped",L"test disk failure");monitor_event(L"A",L"Recording stopped",L"test disk failure");monitor_event(L"B",L"TX / control failed",L"test partial write");check("events coalesce by source and failure",monitor_events_.rows().size()==2&&monitor_events_.rows()[0].count==2);show_monitor_center(true);check("actual event list matches model",ListView_GetItemCount(monitor_center_.list_)==2);
        monitor_center_.SetWindowPos(nullptr,0,0,1000,760,SWP_NOMOVE|SWP_NOZORDER);check("center list/details stay within bounds",contains(monitor_center_.m_hWnd,monitor_center_.list_)&&contains(monitor_center_.m_hWnd,monitor_center_.details_));bool buttons_fit=true;for(const auto h:monitor_center_.buttons_)buttons_fit=buttons_fit&&contains(monitor_center_.m_hWnd,h)&&control_text_width(h,12)<=[] (HWND c){RECT r{};::GetWindowRect(c,&r);return r.right-r.left;}(h);check("all center action captions fit",buttons_fit);
        const auto selected_detail=value(monitor_center_.details_);monitor_event(L"A",L"Connection lost",L"Later event");check("new events preserve the inspected event",value(monitor_center_.details_)==selected_detail);
        TabCtrl_SetCurSel(tabs_,kChartTab);switch_page();check("split/center screenshots",split_actual_&&save_runtime_frame(m_hWnd,folder/L"monitor-main.png")&&save_runtime_frame(monitor_center_.m_hWnd,folder/L"monitor-events.png"));
        request.action=MonitorCenter::ClearEvents;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));check("Clear events preserves recent resources",monitor_events_.rows().empty()&&!monitor_recent_.empty());
        SerialOperation failed;failed.id=9001;failed.target=L"synthetic A";uart_operations_.begin();failed.generation=uart_operations_.generation();uart_operations_.complete({failed,0,false,false,L"Injected write failure"});poll_uart_operations();uart_operations_.cancel();
        auto& b_queue=workbench_->secondary_.queue();b_queue.begin();failed.id=9002;failed.target=L"synthetic B";failed.generation=b_queue.generation();b_queue.complete({failed,0,false,false,L"Injected B write failure"});poll_workbench();b_queue.cancel();
        check("actual A/B TX completion hooks add separate failures",monitor_events_.rows().size()==2&&monitor_events_.rows()[0].source==L"A"&&monitor_events_.rows()[1].source==L"B");
        for(size_t i=0;i<2;++i){RecordingOptions options;options.base=folder/(i?L"monitor-collision-b":L"monitor-collision-a");{std::ofstream collision(options.base.wstring()+L"_1.bin");collision<<"preserve";}auto& recording=i?workbench_->secondary_.recorder:recorder_;check("collision recording starts only in owned fixture",recording.start(options));check("recording failure completes",pump([&]{return !recording.status().active;}));if(i)poll_monitor();else poll_recording();}
        check("actual A/B recorder failure hooks add events",monitor_events_.rows().size()==4);
        request.action=MonitorCenter::ClearEvents;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));
        request.action=MonitorCenter::Remove;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));check("Remove recent preserves source file",monitor_recent_.empty()&&std::filesystem::exists(file));
        check("feature checks never opened UART or armed automation",!serial_mgr_->is_connected()&&!automation_.active()&&!file_tx_active_);
        if(monitor_center_.GetSafeHwnd())monitor_center_.ShowWindow(SW_HIDE);if(advanced_tools_.GetSafeHwnd())advanced_tools_.ShowWindow(SW_HIDE);
    }catch(const std::exception& e){report<<"Exception: "<<e.what()<<'\n';check("monitor workflow completed",false);}return pass;
}
