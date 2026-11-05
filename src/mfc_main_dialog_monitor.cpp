#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
#include <shellapi.h>
#include <cmath>

void CMainDialog::load_monitor_preferences(){
    wchar_t layout[32]{};GetPrivateProfileStringW(L"UI",L"MonitorLayout",L"0:50",layout,32,settings_path_.c_str());const std::wstring choice(layout);
    if(choice==L"0:25"||choice==L"0:50"||choice==L"0:75"||choice==L"1:25"||choice==L"1:50"||choice==L"1:75"){split_logs_=choice[0]==L'1';split_percent_=_wtoi(choice.c_str()+2);}
    monitor_flash_=GetPrivateProfileIntW(L"Monitor",L"FlashErrors",0,settings_path_.c_str())!=0;
    std::wstring text(30002,0);const auto n=GetPrivateProfileStringW(L"Monitor",L"Recent",L"",text.data(),static_cast<DWORD>(text.size()),settings_path_.c_str());text.resize(n);
    try{if(n>=30001)throw std::runtime_error("Recent list is truncated.");monitor_recent_=decode_recent_resources(text);}catch(...){monitor_event(L"Settings",L"Invalid recent list",L"Recent entries could not be loaded; use Monitor center to review and save a new list.");}
}
bool CMainDialog::save_monitor_recent(const std::vector<RecentResource>& rows){
    try{const auto encoded=encode_recent_resources(rows);if(!WritePrivateProfileStringW(L"Monitor",L"Recent",encoded.c_str(),settings_path_.c_str()))throw std::runtime_error("Cannot save recent resources beside EXE.");monitor_recent_=rows;monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);return true;}
    catch(const std::exception& e){show_status_message(uart_wide(e.what()),6000);if(monitor_center_.GetSafeHwnd())::SetWindowTextW(monitor_center_.details_,ui::caption(uart_wide(e.what()).c_str()));return false;}
}
void CMainDialog::remember_monitor_resource(RecentResource resource){try{auto staged=monitor_recent_;remember_resource(staged,std::move(resource));save_monitor_recent(staged);}catch(const std::exception& e){show_status_message(uart_wide(e.what()),6000);}}
void CMainDialog::show_monitor_center(bool events){monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);if(!monitor_center_.show(this,events))show_status_message(ui::tr(L"Cannot open Monitor center."),6000);}
void CMainDialog::monitor_event(std::wstring source,std::wstring kind,std::wstring detail){
    if(closing_serial_)return;
    const bool added=monitor_events_.add({GetTickCount64(),message_unix_ms(),std::move(source),std::move(kind),std::move(detail),1});
    monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);
    if(added&&monitor_flash_&&::IsWindow(m_hWnd)){const auto foreground=::GetForegroundWindow();if(foreground!=m_hWnd&&!::IsChild(m_hWnd,foreground)&&::GetAncestor(foreground,GA_ROOTOWNER)!=m_hWnd){FLASHWINFO flash{sizeof(flash),m_hWnd,FLASHW_TRAY,3,0};::FlashWindowEx(&flash);}}
}
void CMainDialog::poll_monitor(){
    for(size_t i=0;i<2;++i){if(i&&!workbench_)continue;const auto status=i?workbench_->secondary_.recorder.status():recorder_.status();
        if(status.segments&&!monitor_recording_paths_[i].empty()){const auto path=monitor_recording_paths_[i];remember_monitor_resource({ResourceKind::Recording,path.stem().wstring().substr(0,64),path.wstring(),monitor_readable_paths_[i].wstring()});monitor_recording_paths_[i].clear();}
        if(i&&status.detail!=monitor_b_recording_notice_){monitor_b_recording_notice_=status.detail;if(status.lost||status.limit_reached||status.detail.find(L"ERROR")!=status.detail.npos)monitor_event(L"B",L"Recording stopped",status.detail);}
    }
    if(workbench_){const bool connected=workbench_->secondary_connected();if(monitor_b_connected_&&!connected&&!closing_serial_)monitor_event(L"B",L"Connection closed",workbench_->secondary_.status());monitor_b_connected_=connected;}
}
LRESULT CMainDialog::OnMessageFeature(WPARAM action,LPARAM){
    if(closing_serial_||profile_job_.busy())return 0;
    if(action==MessagePage::EditKeywords){
        const auto validate=[this](const std::vector<std::wstring>& fields,std::wstring& error){std::vector<KeywordRule> rules;if(!parse_keyword_rules(fields[0],rules,error)){error=ui::tr(error);return false;}if(profile_job_.busy()||closing_serial_){error=ui::tr(L"Settings are busy; previous rules retained.");return false;}const auto encoded=monitor_hex_text(fields[0]);if(!WritePrivateProfileStringW(L"Messages",L"KeywordRules",encoded.c_str(),settings_path_.c_str())){error=ui::tr(L"Cannot save keyword rules; previous rules retained.");return false;}message_page_.set_keywords(fields[0],rules);return true;};
        UartToolForm form(L"Keyword rules",L"One rule per line: color,word/line,0/1,text. Colors: red,yellow,cyan,magenta,blue,white,green. 1 = case sensitive. Up to 16 literal rules; last rule wins, Find highlight wins over rules. Empty clears rules. Example: red,line,0,error",{{L"Rules",message_page_.keyword_definition(),{},8}},[](const auto& f){std::vector<KeywordRule> rules;std::wstring error;if(!parse_keyword_rules(f[0],rules,error))return ui::tr(error);return ui::tr(L"Valid rules:")+L" "+std::to_wstring(rules.size());},validate,this);form.DoModal();return 1;
    }
    if(action==MessagePage::InspectBytes){
        const auto bytes=message_page_.buffer().bytes();const auto offset=message_page_.buffer().offset();if(bytes.empty()){show_status_message(ui::tr(L"No retained bytes to inspect."),5000);return 0;}
        const auto range=[bytes,offset](const std::vector<std::wstring>& f)->std::wstring{try{uint64_t first=0,count=0;const auto integer=[](const std::wstring& s,uint64_t& out){if(s.empty()||s.size()>20||s.find_first_not_of(L"0123456789")!=s.npos)return false;try{out=std::stoull(s);return true;}catch(...){return false;}};if(!integer(f[0],first)||!integer(f[1],count)||count>4096)throw std::runtime_error("Invalid byte range.");return inspect_monitor_bytes(bytes,offset,first,static_cast<size_t>(count),ui::tr(L"Offset | HEX | ASCII"),ui::tr(L"Control annotations: first 128 only; HEX/ASCII covers the complete selected range."));}catch(...){return ui::tr(L"Select 1..4096 bytes inside the retained snapshot.");}};
        UartToolForm form(L"Retained byte inspector",L"Immutable raw RX snapshot, independent of terminal edits and hidden NUL. Enter an absolute DECIMAL byte offset and 1..4096 bytes, then Preview. Dots in ASCII mean nonprintable bytes; HEX remains exact. Close only closes this viewer; no TX or file changes.",{{L"Absolute byte offset",std::to_wstring(offset)},{L"Byte count",std::to_wstring(std::min<size_t>(4096,bytes.size()))}},range,{},this);form.accept_caption(L"Close");form.monospace_preview();return form.DoModal();
    }return 0;
}
LRESULT CMainDialog::OnMonitorRequest(WPARAM,LPARAM parameter){
    if(!parameter||closing_serial_||profile_job_.busy())return 0;const auto r=*reinterpret_cast<const MonitorCenter::Request*>(parameter);
    try{
        if(r.action==MonitorCenter::Flash){if(!WritePrivateProfileStringW(L"Monitor",L"FlashErrors",r.flag?L"1":L"0",settings_path_.c_str()))throw std::runtime_error("Cannot save notification preference.");monitor_flash_=r.flag;monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);return 1;}
        if(r.action==MonitorCenter::ClearEvents){monitor_events_.clear();monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);return 1;}
        if(r.action==MonitorCenter::Add){CFileDialog pick(TRUE,nullptr,nullptr,OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,ui::caption(L"UART resources (*.bin;*.log;*.txt;*.sccprofile;*.sccview;*.sccsession)|*.bin;*.log;*.txt;*.sccprofile;*.sccview;*.sccsession||"),&monitor_center_);if(pick.DoModal()!=IDOK)return 0;const std::filesystem::path path(pick.GetPathName().GetString());auto ext=path.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),[](wchar_t c){return static_cast<wchar_t>(towlower(c));});ResourceKind kind;if(ext==L".sccprofile")kind=ResourceKind::Profile;else if(ext==L".sccview")kind=ResourceKind::View;else if(ext==L".sccsession")kind=ResourceKind::Session;else if(ext==L".bin"||ext==L".log"||ext==L".txt")kind=ResourceKind::Recording;else throw std::runtime_error("Choose a supported UART resource.");remember_monitor_resource({kind,path.stem().wstring().substr(0,64),path.wstring()});return 1;}
        const auto found=std::find_if(monitor_recent_.begin(),monitor_recent_.end(),[&](const auto& e){return e.kind==r.resource.kind&&e.path==r.resource.path;});if(found==monitor_recent_.end())return 0;
        if(r.action==MonitorCenter::Remove||r.action==MonitorCenter::Rename){auto staged=monitor_recent_;const auto at=static_cast<size_t>(found-monitor_recent_.begin());if(r.action==MonitorCenter::Remove)staged.erase(staged.begin()+at);else staged[at].name=r.name;return save_monitor_recent(staged)?1:0;}
        if(r.action==MonitorCenter::Folder){const auto folder=std::filesystem::path(found->related.empty()?found->path:found->related).parent_path();if(!std::filesystem::is_directory(folder)||reinterpret_cast<INT_PTR>(ShellExecuteW(m_hWnd,L"explore",folder.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)throw std::runtime_error("Cannot open resource folder.");return 1;}
        const auto resource=*found;
        if(resource.kind==ResourceKind::Recording){show_advanced_tools(AdvancedTools::Replay);advanced_tools_.set_field(AdvancedTools::Replay,0,resource.path);advanced_tools_.notice(AdvancedTools::Replay,L"Recent source selected. Review the interval and click Load; no playback or TX started.");}
        else if(resource.kind==ResourceKind::Profile){show_advanced_tools(AdvancedTools::Profiles);if(profile_job_.busy())return 0;const auto path=std::filesystem::path(resource.path);if(profile_job_.start([path](const std::atomic<bool>& cancel){return decode_profile(uart_read_file(path,65536,cancel));})){profile_job_action_=2;monitor_profile_path_=path;advanced_tools_.notice(AdvancedTools::Profiles,L"Reading selected profile in background; review and Apply separately.");}}
        else {show_uart_lab();if(lab_file_.busy())throw std::runtime_error("Wait for Laboratory file work.");const auto path=std::filesystem::path(resource.path);if(resource.kind==ResourceKind::View){lab_.select(UartLabWindow::Views);if(start_lab_file(UartLabWindow::Views,2,[path](const std::atomic<bool>& cancel){LabFileResult result;result.view=std::make_shared<ViewPreset>(decode_view(uart_read_file(path,65536,cancel)));result.report="View staged; explicit Apply required. Capture and ports unchanged.";return result;})){monitor_lab_path_=path;monitor_lab_kind_=ResourceKind::View;}}
            else {lab_.select(UartLabWindow::Sessions);lab_.field(UartLabWindow::Sessions,1,resource.path);lab_.notice(UartLabWindow::Sessions,L"Recent session selected. Verify and Stage explicitly; no source loaded and no port opened.");}}
        return 1;
    }catch(const std::exception& e){show_status_message(uart_wide(e.what()),8000);if(monitor_center_.GetSafeHwnd())::SetWindowTextW(monitor_center_.details_,ui::caption(uart_wide(e.what()).c_str()));monitor_center_.update(monitor_recent_,monitor_events_,monitor_flash_);return 0;}
}
