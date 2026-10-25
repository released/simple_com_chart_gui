#include "mfc_main_dialog.h"
#include "chart_analysis.h"
#include "uart_workbench.h"
#include <sstream>

bool CMainDialog::advanced_idle() const {
    return !serial_running_ && !disconnect_pending_ && !serial_open_.busy() && connection_.mode()==SerialConnectionState::Mode::Idle && !serial_mgr_->is_connected() && (!workbench_||(!workbench_->secondary_active()&&!workbench_->analysis_busy()&&!workbench_->tx_reserved(0)&&!workbench_->tx_reserved(1)));
}
void CMainDialog::show_advanced_tools(int page) {
    if (!advanced_tools_.show(this,ui_font_,page)) show_status_message(L"Cannot create advanced tools window",5000);
    if (page==AdvancedTools::Analysis && advanced_tools_.field(page,0).empty() && !capture_.keys().empty()) advanced_tools_.set_field(page,0,uart_wide(capture_.keys().front()));
    std::vector<std::wstring> keys;for(const auto& key:capture_.keys()) keys.push_back(uart_wide(key));
    advanced_tools_.set_choices(AdvancedTools::Analysis,0,keys);advanced_tools_.set_choices(AdvancedTools::Channels,0,keys);
    if(page==AdvancedTools::Extraction) {
        const auto& c=extractor_.config();::SendMessageW(advanced_tools_.field_hwnd(page,0),CB_SETCURSEL,c.mode,0);
        advanced_tools_.set_field(page,1,uart_wide(c.columns));advanced_tools_.set_field(page,2,uart_wide(c.pattern));
        advanced_tools_.set_field(page,3,std::to_wstring(c.name_group));advanced_tools_.set_field(page,4,std::to_wstring(c.value_group));advanced_tools_.set_field(page,5,std::to_wstring(c.time_group));advanced_tools_.set_field(page,6,uart_wide(c.time_field));
    }
}
LRESULT CMainDialog::OnAdvancedRequest(WPARAM,LPARAM parameter) {
    if (!parameter || closing_serial_) return 0;
    const auto request=*reinterpret_cast<const AdvancedTools::Request*>(parameter);
    if (request.page==-1) {
        stop_automation(L"Stopped: advanced window closed");cancel_file_tx(L"File TX stopped: advanced window closed."); replay_.pause(GetTickCount64()); replay_load_.cancel(); replay_io_.cancel(); profile_job_.cancel();
        if (replay_load_.busy() || replay_io_.busy() || profile_job_.busy() || file_tx_job_.busy()) { advanced_tools_.notice(profile_job_.busy()?AdvancedTools::Profiles:AdvancedTools::Replay,L"Cancellation requested; waiting for file worker. Close again after completion; completed saves are not undone."); }
        else advanced_tools_.ShowWindow(SW_HIDE);
        return 0;
    }
    if (profile_job_.busy() && profile_job_action_==4) { advanced_tools_.notice(AdvancedTools::Profiles,L"Wait for profile Apply to finish, or close this window to cancel."); return 0; }
    try {
    if (request.page==AdvancedTools::Replay) replay_request(request);
    else if (request.page==AdvancedTools::Profiles) profile_request(request);
    else if (request.page==AdvancedTools::Analysis) analysis_request(request);
    else if (request.page==AdvancedTools::Automation) automation_request(request);
    else if (request.page==AdvancedTools::Extraction) extraction_request(request);
    else if (request.page==AdvancedTools::Channels) channel_display_request(request);
    else if (request.page==AdvancedTools::FileTx) file_tx_request(request);
    } catch (const std::exception&) { advanced_tools_.notice(request.page,L"Operation failed (file/path/encoding error); no automatic retry. Check the selected path/settings."); }
    return 0;
}
void CMainDialog::poll_advanced_tools() {
    poll_replay_io();
    poll_file_tx();
    std::wstring error; std::shared_ptr<ReplayData> data;
    if (replay_load_.take(data,error)) {
        if (!error.empty()) advanced_tools_.notice(AdvancedTools::Replay,error);
        else if (!advanced_idle() || recorder_.status().active) advanced_tools_.notice(AdvancedTools::Replay,L"Live state changed: offline load not applied");
        else { replay_.load(std::move(data)); reset_offline_view(); message_page_.set_offline_source(true,!replay_.data()->receipt_times);
            advanced_tools_.notice(AdvancedTools::Replay,L"Loaded / paused. "+replay_.data()->provenance); }
    }
    UartProfile profile;
    if (profile_job_.take(profile,error,profile_job_action_<3)) {
        if (profile_job_action_==4) lock_profile_ui(false);
        if (!error.empty()) advanced_tools_.notice(AdvancedTools::Profiles,error);
        else if (profile_job_action_==4) { load_settings(); load_startup_tab(); if(workbench_)workbench_->reload_preferences(); advanced_tools_.notice(AdvancedTools::Profiles,L"Profile applied and saved. Port remains CLOSED; startup tab applies next launch. Capture reset."); }
        else {
            staged_profile_=std::move(profile); has_staged_profile_=true;
            const auto& p=staged_profile_;
            std::wstring preview=L"Staged (port CLOSED): "+(p.serial.port.device.empty()?std::wstring(L"no port selected"):p.serial.port.device)+L" / "+std::to_wstring(p.serial.baud)+
                L" / bits "+std::to_wstring(p.serial.data_bits)+L" / parity "+std::to_wstring(p.serial.parity)+L" / stop "+std::to_wstring(p.serial.stop_bits)+
                L" | flow "+std::to_wstring(static_cast<int>(p.serial.controls.flow))+L" / DTR "+(p.serial.controls.dtr?L"on":L"off")+L" / RTS "+(p.serial.controls.rts?L"on":L"off")+
                L"\r\nInclude: "+p.include+L" | Exclude: "+p.exclude+(p.regex?L" | Regex":L" | Text")+L" | Max CH "+std::to_wstring(p.max_channels)+L" | Font "+std::to_wstring(p.font_points)+L" pt";
            for (size_t i=0;i<4;++i) preview+=L"\r\nChart "+std::to_wstring(i+1)+L": "+(p.charts[i].empty()?std::wstring(L"(none)"):uart_wide(p.charts[i]));
            advanced_tools_.state(AdvancedTools::Profiles,preview);
            advanced_tools_.notice(AdvancedTools::Profiles,profile_job_action_==1 || profile_job_action_==2 ? L"Profile staged / validated; no live settings changed. Review above, then Apply staged when idle." : L"Profile saved; private commands/history were not included.");
        }
    }
    if (close_after_tools_ && !replay_load_.busy() && !profile_job_.busy() && !replay_io_.busy() && !file_tx_job_.busy()) { close_after_tools_=false; OnCancel(); return; }
    const auto now=GetTickCount64();
    const auto first=replay_.position(); const auto count=(replay_io_.busy()||!replay_frames_.empty())?0:replay_.due(now,replay_binary_enabled_?1:4);
    if (count) feed_replay(first,count);
    if (replay_.data()) {
        const auto value=std::wstring(replay_.playing()?L"Playing":L"Paused / end")+L" | record "+std::to_wstring(replay_.position())+L" / "+std::to_wstring(replay_.data()->records.size())+L" | "+replay_.data()->provenance+(replay_binary_enabled_?L" | Binary CRC errors "+std::to_wstring(replay_crc_errors_)+L" / discarded "+std::to_wstring(replay_discarded_):L"");
        advanced_tools_.state(AdvancedTools::Replay,value); set_left_status(L"OFFLINE: "+value);
    }
    if (automation_.active()) {
        if (!serial_running_ || !serial_mgr_->is_connected() || disconnect_pending_ || connection_.mode()!=SerialConnectionState::Mode::Connected || !uart_operations_.current(automation_generation_)) stop_automation(L"Stopped: connection changed; no automatic resume");
        else {
            std::string bytes;
            if (automation_.next(now,bytes)) {
                SerialOperation operation; operation.target=connection_.active().port.device; operation.bytes=std::move(bytes);
                if (!uart_operations_.submit(operation,automation_id_,error)) stop_automation(L"Stopped: TX queue unavailable; "+error);
                else { const auto line=L"Automation queued #"+std::to_wstring(automation_id_)+L" for "+operation.target;
                    advanced_tools_.notice(AdvancedTools::Automation,line); recorder_.event(message_utf8(line),message_unix_ms()); }
            }
        }
    }
    if (!automation_.active() && automation_id_) { uart_operations_.cancel_pending(automation_id_); automation_id_=0; }
    advanced_tools_.state(AdvancedTools::Automation,automation_.status());
}
void CMainDialog::analysis_request(const AdvancedTools::Request& request) {
    if (request.action==1) { advanced_tools_.clear_analysis(); return; }
    if (request.fields.size()!=6) return;
    const auto data=request.action==2?advanced_tools_.analysis_snapshot():capture_.snapshot(); const auto name=message_utf8(request.fields[0]);
    const auto key=std::find(data.keys.begin(),data.keys.end(),name);
    double a=0,b=0,end=0,window=0,reference=0;
    if (key==data.keys.end() || data.rows.empty() || !uart_number(request.fields[1],0,1e12,a) || !uart_number(request.fields[2],0,1e12,b) ||
        !uart_number(request.fields[3],0,1e12,end) || !uart_number(request.fields[4],1,86400,window) ||
        (!request.fields[5].empty() && !uart_number(request.fields[5],-1e100,1e100,reference))) {
        advanced_tools_.notice(AdvancedTools::Analysis,L"Choose an existing exact channel; enter finite A/B/end seconds and a 1..86400 s window. Optional Y must be finite."); return;
    }
    const auto column=static_cast<size_t>(key-data.keys.begin()); const auto stats=analyze_capture(data,column,a,b);
    if(request.action==2) advanced_tools_.update_analysis_cursors(a,b,!request.fields[5].empty(),reference);
    else advanced_tools_.show_analysis(data,column,a,b,end,window,!request.fields[5].empty(),reference);
    std::wostringstream text; text.precision(10);
    text<<L"Retained snapshot: "<<data.rows.size()<<L" rows; expired "<<data.dropped<<L". Range "<<data.rows.front()->seconds<<L".."<<data.rows.back()->seconds<<L" s.\r\n";
    if (stats.retained_count) text<<L"Nearest A="<<stats.a<<L", B="<<stats.b<<L", dY="<<stats.delta<<L", dT="<<b-a<<L" s. ";
    else text<<L"No retained samples for this field. ";
    text<<L"Interval N="<<stats.count;
    if (stats.count) text<<L", min="<<stats.minimum<<L", max="<<stats.maximum<<L", mean="<<stats.mean<<L", RMS="<<stats.rms<<L", population SD="<<stats.stddev;
    else text<<L"; interval statistics n/a";
    text<<L". Reference line drawn only inside current Y range; empty windows contain no invented samples.";
    advanced_tools_.state(AdvancedTools::Analysis,text.str());
}
