#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "workflow_profile_diff.h"

UartProfile CMainDialog::current_profile() const {
    UartProfile p; p.serial=remembered_serial_; p.serial.port.open_path.clear(); p.serial.controls=desired_controls_;
    const auto text=[](HWND window) { wchar_t value[512]{}; ::GetWindowTextW(window,value,512); return std::wstring(value); };
    p.serial.baud=_wtoi(text(combo_baud_).c_str()); p.serial.data_bits=_wtoi(text(combo_data_).c_str());
    const auto parity=text(combo_parity_), stop=text(combo_stop_);
    p.serial.parity=parity==L"ODD"?ODDPARITY:parity==L"EVEN"?EVENPARITY:NOPARITY;
    p.serial.stop_bits=stop==L"1.5"?ONE5STOPBITS:stop==L"2"?TWOSTOPBITS:ONESTOPBIT;
    const auto selected=static_cast<int>(::SendMessageW(combo_port_,CB_GETCURSEL,0,0));
    if (selected>=0 && selected<static_cast<int>(known_ports_.size())) { p.serial.port=known_ports_[selected]; p.serial.port.open_path.clear(); }
    p.include=active_include_; p.exclude=active_exclude_; p.regex=active_regex_; p.max_channels=model_.get_max_channels(); p.extraction=extractor_.config();p.channels=display_preferences_;
    for (size_t i=0;i<4;++i) p.charts[i]=data_page_.selected_field(i);
    p.display=message_page_.options(); p.wrap=message_page_.wraps(); p.font_points=message_page_.font_points(); p.startup_tab=startup_tab_;p.workspace=true;p.workspace_settings=read_workspace_settings(settings_path_);
    if(workbench_&&workbench_->ready_){std::wstring error;const auto b=workbench_->chosen(1,error);if(error.empty()){p.workspace_settings["DualCOM.B/Port"]=message_utf8(b.port.device);p.workspace_settings["DualCOM.B/InstanceId"]=message_utf8(b.port.instance_id);p.workspace_settings["DualCOM.B/Baud"]=std::to_string(b.baud);p.workspace_settings["DualCOM.B/Data"]=std::to_string(b.data_bits);p.workspace_settings["DualCOM.B/Parity"]=std::to_string(b.parity);p.workspace_settings["DualCOM.B/Stop"]=std::to_string(b.stop_bits);}}
    return p;
}
void CMainDialog::lock_profile_ui(bool lock) {
    if(workbench_)workbench_->lock_ui(lock);
    if(lab_.GetSafeHwnd()){if(lock){profile_lab_enabled_=lab_.IsWindowEnabled()!=FALSE;lab_.EnableWindow(FALSE);}else lab_.EnableWindow(profile_lab_enabled_);}
    if (lock) {
        profile_locked_controls_.clear();
        for (HWND child=::GetWindow(m_hWnd,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT)) {
            profile_locked_controls_.push_back({child,::IsWindowEnabled(child)!=FALSE}); ::EnableWindow(child,FALSE);
        }
    } else { for (const auto& item:profile_locked_controls_) if (::IsWindow(item.first)) ::EnableWindow(item.first,item.second); profile_locked_controls_.clear(); }
}
bool CMainDialog::apply_profile(const UartProfile& profile) {
    if (!advanced_idle() || replay_.data() || replay_load_.busy() || recorder_.status().active || exporting_ || profile_job_.busy()) return false;
    if(!profile.workspace&&workbench_&&workbench_->requires_receipt_clock()&&(!profile.extraction.time_field.empty()||(profile.extraction.mode==ExtractionConfig::Regex&&profile.extraction.time_group>0)))return false;
    std::wstring error; if (!validate_profile(profile,error)) return false;
    const auto path=settings_path_;
    if (!profile_job_.start([path,profile](const std::atomic<bool>& cancel) { write_profile_ini(path,profile,cancel); return profile; })) return false;
    profile_job_action_=4; lock_profile_ui(true); return true;
}
void CMainDialog::profile_request(const AdvancedTools::Request& request) {
    const auto tell=[&](const std::wstring& text) { advanced_tools_.notice(AdvancedTools::Profiles,text); };
    if (profile_job_.busy()) { tell(L"Profile operation pending; wait or close this window to request cancellation."); return; }
    if (request.action==4) {
        if (!has_staged_profile_ || !advanced_idle() || replay_.data() || replay_load_.busy() || recorder_.status().active || exporting_) { tell(L"Stage a valid profile; disconnect/unload replay and stop recording/export before Apply."); return; }
        if(lab_file_.busy()){tell(L"Wait for Laboratory file work before profile review.");return;}
        const auto profile=staged_profile_;std::string before;
        try{const auto current=current_profile();before=encode_profile(current);WorkflowGrid::Rows rows;for(const auto& item:profile_differences(current,profile))rows.push_back({uart_wide(item[0]),uart_wide(item[1]),uart_wide(item[2])});if(rows.empty())rows.push_back({ui::tr(L"No supported preference differences."),L"",L""});WorkflowGrid review(L"Review profile changes",{L"Setting",L"Current",L"Staged"},std::move(rows),{},&advanced_tools_);if(workflow_grid_present_(review)!=IDOK)return;
            if(encode_profile(current_profile())!=before){tell(L"Current preferences changed during review. Review the differences again; nothing applied.");return;}}
        catch(const std::exception& e){tell(uart_wide(e.what()));return;}
        if (!::IsWindow(m_hWnd) || closing_serial_) return;
        if (!apply_profile(profile)) tell(L"State changed; profile not applied. Review and try again.");
        else tell(L"Preparing and atomically saving profile in background; main settings input is temporarily locked."); return;
    }
    std::filesystem::path path; bool replace=false;
    if (request.action==0 || request.action==1) {
        if (request.fields.empty() || !valid_profile_name(request.fields[0])) { tell(L"Name must be 1..48 ASCII letters/digits/_/-; no reserved device names."); return; }
        const auto folder=std::filesystem::path(settings_path_).parent_path()/L"profiles";
        const auto attributes=GetFileAttributesW(folder.c_str());
        if (attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) { tell(L"Named profiles folder must not be a link/junction."); return; }
        std::error_code ec; if (request.action==0) std::filesystem::create_directory(folder,ec);
        if (ec) { tell(L"Cannot create portable profiles folder."); return; }
        path=folder/(request.fields[0]+L".sccprofile");
        if (request.action==0 && std::filesystem::exists(path)) {
            const auto confirm=uart_notice_; if (confirm(advanced_tools_.GetSafeHwnd(),L"Replace this existing named profile?",L"Save profile",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return;
            if (!::IsWindow(m_hWnd) || closing_serial_) return; replace=true;
        }
    } else {
        if (request.action==3 && !has_staged_profile_) { tell(L"Load/import or save a profile to stage it first."); return; }
        CFileDialog file(request.action==2,L"sccprofile",L"UART.sccprofile",OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(request.action==2?OFN_FILEMUSTEXIST:OFN_OVERWRITEPROMPT),L"UART profile (*.sccprofile)|*.sccprofile||",&advanced_tools_);
        if (file.DoModal()!=IDOK) return;
        if (!::IsWindow(m_hWnd) || closing_serial_) return;
        path=file.GetPathName().GetString(); replace=request.action==3;
        if (path.extension()!=L".sccprofile") { tell(L"Use the .sccprofile extension."); return; }
    }
    UartProfile snapshot=request.action==0?current_profile():staged_profile_; std::string encoded;
    try { if (request.action==0 || request.action==3) encoded=encode_profile(snapshot); }
    catch (...) { tell(L"Current settings are invalid; profile not saved."); return; }
    const auto action=request.action;
    if (!profile_job_.start([path,action,snapshot,encoded,replace](const std::atomic<bool>& cancel) {
        if (action==1 || action==2) return decode_profile(uart_read_file(path,512*1024,cancel));
        uart_write_file(path,encoded,replace,cancel); return snapshot;
    })) { tell(L"Cannot start profile worker"); return; }
    profile_job_action_=action; tell(L"Profile file operation in background: "+path.wstring());
}
