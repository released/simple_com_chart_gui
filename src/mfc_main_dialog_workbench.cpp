#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_files.h"

void CMainDialog::show_workbench(){
    if(!workbench_)workbench_=std::make_unique<UartWorkbench>();
    workbench_->primary_queue(&uart_operations_);
    workbench_->display_sources([this]{return capture_.keys();},[this](const std::string& key,size_t limit){std::vector<std::pair<double,double>> result;const auto& keys=capture_.keys();const auto found=std::find(keys.begin(),keys.end(),key);if(found==keys.end())return result;const auto column=static_cast<size_t>(found-keys.begin());for(auto it=capture_.rows().rbegin();it!=capture_.rows().rend()&&result.size()<std::min<size_t>(4096,limit);++it){double value=0;if(CaptureStore::value(**it,column,value))result.push_back({(*it)->source_seconds,display_preferences_.project(key,value)});}std::reverse(result.begin(),result.end());return result;});
    workbench_->analysis_sources([this]{return capture_.snapshot();},[this]{RunOptions options;options.extraction=extractor_.config();options.binary=replay_binary_enabled_;options.schema=replay_binary_config_;options.include=message_utf8(active_include_);options.exclude=message_utf8(active_exclude_);options.regex=active_regex_;const auto interval=advanced_tools_.field_hwnd(AdvancedTools::Replay,1);double ms=0;wchar_t value[32]{};if(::IsWindow(interval)){::GetWindowTextW(interval,value,32);if(uart_number(value,1,60000,ms))options.interval_ms=static_cast<unsigned>(ms);}return options;},[this](const std::string& key,double value){return display_preferences_.project(key,value);});
    workbench_->set_scanner(port_scanner_);poll_workbench();
    if(!workbench_->show(this,settings_path_,ui_font_,primary_traffic_)){show_status_message(L"Cannot open UART Workbench",5000);return;}
    workbench_feed_enabled_=true;
}
void CMainDialog::poll_workbench(){
    if(!workbench_)return;
    const auto& extraction=extractor_.config();if(workbench_->requires_receipt_clock()&&(!extraction.time_field.empty()||(extraction.mode==ExtractionConfig::Regex&&extraction.time_group>0))){workbench_->suspend_binary();workbench_->suspend_text();show_status_message(L"Workbench numeric sources disabled: stored device-time extraction conflicts with shared PC-receipt charts. Review parser settings.",8000);log_line(L"Workbench numeric sources disabled due to incompatible stored clock settings.");}
    workbench_numeric_owned_=workbench_->binary_enabled(0)||workbench_->numeric_suspended(0);
    uart_operations_.pacing(workbench_->pacing(0));
    UartWorkbench::Primary p;p.active=connection_.mode()!=SerialConnectionState::Mode::Idle||disconnect_pending_||serial_open_.busy();p.connected=serial_mgr_->is_connected()&&!disconnect_pending_;p.stopping=disconnect_pending_;
    p.settings=connection_.active();if(!p.active){p.settings.port=selection_hint_;p.settings.controls=desired_controls_;wchar_t value[32]{};::GetWindowTextW(combo_baud_,value,32);try{p.settings.baud=std::stoi(value);}catch(...){p.settings.baud=115200;}::GetWindowTextW(combo_data_,value,32);p.settings.data_bits=_wtoi(value);::GetWindowTextW(combo_parity_,value,32);p.settings.parity=std::wstring(value)==L"EVEN"?EVENPARITY:std::wstring(value)==L"ODD"?ODDPARITY:NOPARITY;::GetWindowTextW(combo_stop_,value,32);p.settings.stop_bits=std::wstring(value)==L"1.5"?ONE5STOPBITS:std::wstring(value)==L"2"?TWOSTOPBITS:ONESTOPBIT;}
    p.generation=uart_operations_.generation();p.recording=recorder_.status();p.status=p.connected?L"Connected: "+p.settings.port.device:p.active?L"Opening / reconnecting / stopping: "+p.settings.port.device:L"Disconnected";workbench_->update_primary(p);
    workbench_->tick();
    const auto frames=workbench_->take_binary();
    if(!frames.empty()&&!replay_.data()){
        for(const auto& item:frames){std::unordered_map<std::string,double> values;for(const auto& value:item.frame.values)values[(item.port==0?"A.":"B.")+value.first]=value.second;
            if(values.empty())continue;const double at=item.frame.sample_seconds>0?item.frame.sample_seconds:traffic_monotonic_seconds();
            accept_numeric((item.port==0?"A binary: ":"B binary: ")+uart_hex(item.frame.bytes),values,at,UINT64_MAX,0,static_cast<unsigned>(item.port),item.frame.generation,UINT16_MAX);}
        model_.prune(now_seconds());sync_channels();if(::IsWindowVisible(data_page_.hwnd()))data_page_.refresh(now_seconds());if(!snapshot_&&::IsWindowVisible(plot_view_.hwnd()))plot_view_.update_from_model(now_seconds());
        std::unordered_map<std::string,double> latest;for(const auto& key:model_.get_keys()){double v=0;if(model_.latest_value(key,v))latest[key]=v;}channel_panel_.update_values(latest);
    }
    const auto text=workbench_->take_text();
    if(!text.empty()&&!replay_.data()){
        for(const auto& frame:text){std::unordered_map<std::string,double> values;for(const auto& value:frame.values)values["B."+value.first]=value.second;
            accept_numeric("B text: "+frame.line,values,frame.seconds,UINT64_MAX,0,1,frame.generation,8);}
        model_.prune(now_seconds());std::unordered_map<std::string,double> latest;for(const auto& key:model_.get_keys()){double value=0;if(model_.latest_value(key,value))latest[key]=value;}channel_panel_.update_values(latest);
        sync_channels();if(::IsWindowVisible(data_page_.hwnd()))data_page_.refresh(now_seconds());if(!snapshot_&&::IsWindowVisible(plot_view_.hwnd()))plot_view_.update_from_model(now_seconds());
    }
    if(close_after_workbench_&&workbench_->shutdown_complete()){close_after_workbench_=false;OnCancel();}
}
LRESULT CMainDialog::OnWorkbenchRequest(WPARAM,LPARAM value){
    if(!value||closing_serial_)return 0;const auto copy=*reinterpret_cast<const UartWorkbench::Request*>(value);const auto* r=&copy;
    if(r->action==UartWorkbench::Request::ResetNumeric){recorder_.event("NUMERIC CONFIG CHANGED",message_unix_ms());if(workbench_)workbench_->secondary_.recorder.event("NUMERIC CONFIG CHANGED",message_unix_ms());diagnostics_.reset();for(auto& sequence:sequences_)sequence.reset();}
    if(r->action==UartWorkbench::Request::CheckLive)return !profile_job_.busy()&&!replay_load_.busy()&&!replay_.data();
    if(r->action==UartWorkbench::Request::DiscardText){workbench_numeric_owned_=workbench_&&(workbench_->binary_enabled(0)||workbench_->numeric_suspended(0));std::lock_guard<std::mutex> lock(pending_mutex_);pending_.lines.clear();source_lines_.clear_partial();return 1;}
    if(r->action==UartWorkbench::Request::CheckClock){const auto& c=extractor_.config();return !profile_job_.busy()&&!exporting_&&!replay_load_.busy()&&!replay_.data()&&c.time_field.empty()&&!(c.mode==ExtractionConfig::Regex&&c.time_group>0);}
    if(r->action==UartWorkbench::Request::CheckBinary){const auto& c=extractor_.config();return (!workbench_||(!workbench_->tx_reserved(0)&&!workbench_->tx_reserved(1)))&&!profile_job_.busy()&&!exporting_&&!replay_load_.busy()&&!replay_.data()&&(!r->flag||(c.time_field.empty()&&!(c.mode==ExtractionConfig::Regex&&c.time_group>0)));}
    if(r->action==UartWorkbench::Request::ResetNumeric){snapshot_=false;plot_view_.set_frozen(false);::SetWindowTextW(btn_snapshot_,L"Snapshot");::SendMessageW(btn_snapshot_,BM_SETCHECK,BST_UNCHECKED,0);workbench_numeric_owned_=workbench_&&(workbench_->binary_enabled(0)||workbench_->numeric_suspended(0));model_.reset();capture_.reset(static_cast<size_t>(model_.get_max_channels()));data_page_.reset();plot_view_.reset_visual();channel_panel_.reset();extracted_clock_=false;{std::lock_guard<std::mutex> lock(pending_mutex_);pending_.lines.clear();source_lines_.clear_partial();}show_status_message(L"Binary settings applied; new numeric capture. Raw RX/recording unchanged.",5000);return 1;}
    if(r->action==UartWorkbench::Request::RefreshA){poll_workbench();return 0;}
    if(r->action==UartWorkbench::Request::CancelTx){uart_operations_.cancel_pending(r->id);return 1;}
    if(r->action==UartWorkbench::Request::CheckTx){return serial_running_&&!disconnect_pending_&&serial_mgr_->is_connected()&&connection_.mode()==SerialConnectionState::Mode::Connected&&!automation_.active()&&!file_tx_busy()&&!controls_pending_id_&&!uart_operations_.busy()&&!profile_job_.busy()&&!replay_.data();}
    if(r->action==UartWorkbench::Request::RecordB){start_recording(1);return 0;}
    if(r->action==UartWorkbench::Request::RecordA){if(recorder_.status().active)recorder_.stop();else start_recording();return 0;}
    const bool active=connection_.mode()!=SerialConnectionState::Mode::Idle||disconnect_pending_||serial_open_.busy();
    if(r->generation!=uart_operations_.generation()||r->was_active!=active){show_status_message(L"Port A connection changed; review the device and retry the action.",6000);return 0;}
    if(r->action==UartWorkbench::Request::SendRaw){if(!serial_running_||disconnect_pending_||!serial_mgr_->is_connected()||automation_.active()||file_tx_busy()||controls_pending_id_)return 0;SerialOperation operation;operation.bytes=r->bytes;operation.target=connection_.active().port.device;operation.pacing_override=r->flag;uint64_t id=0;std::wstring error;if(!uart_operations_.submit(std::move(operation),id,error)){show_status_message(error,8000);return 0;}return static_cast<LRESULT>(id);}
    if(r->action==UartWorkbench::Request::SendA){UartConsole::Request request;request.action=UartConsole::Request::Action::Send;request.tx=r->tx;return OnUartRequest(0,reinterpret_cast<LPARAM>(&request));}
    if(connection_.mode()!=SerialConnectionState::Mode::Idle||serial_mgr_->is_connected()){disconnect();return 0;}
    if(replay_.data()||replay_load_.busy()||profile_job_.busy())return fail_manual_connection(r->settings.port.device,L"Unload offline replay / finish profile work before connecting");
    bool ok=false;const auto ports=port_scanner_(&ok);const int index=ok?find_serial_port(ports,r->settings.port):-1;
    if(index<0)return fail_manual_connection(r->settings.port.device,L"The selected device is no longer uniquely available; Scan and select it again");
    auto settings=r->settings;settings.port=ports[index];if(!valid_serial_settings(settings))return fail_manual_connection(settings.port.device,L"Invalid serial settings");
    update_port_combo(ports);::SendMessageW(combo_port_,CB_SETCURSEL,index,0);::SetWindowTextW(combo_baud_,std::to_wstring(settings.baud).c_str());
    ::SendMessageW(combo_data_,CB_SETCURSEL,settings.data_bits-5,0);::SendMessageW(combo_parity_,CB_SETCURSEL,settings.parity==EVENPARITY?1:settings.parity==ODDPARITY?2:0,0);::SendMessageW(combo_stop_,CB_SETCURSEL,settings.stop_bits,0);
    begin_serial_open(settings,false);return 0;
}
