#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"

namespace {std::wstring presentation_ms(double value){wchar_t text[64]{};swprintf_s(text,L"%.3f",value);return text;}}

void CMainDialog::monitor_feature_command(int id){
    if(closing_serial_||discovery_modal_)return;
    if(id==ID_FD_CADENCE)monitor_cadence();
    else if(id==ID_FD_TIMELINE)monitor_timeline_view();
    else if(id==ID_FD_DISTRIBUTION)monitor_distribution_view();
    else if(id==ID_FD_JSON)monitor_json_paths();
    else if(id==ID_FD_ALARMS)monitor_alarm_view();
    else if(id==ID_FD_OBSERVE){
        MonitorModal modal(discovery_modal_);
        UartToolForm form(L"Observation only",L"Session only; off on launch. Blocks TX, scripts, forwarding, controls, Break and parser/profile Apply on both ports. Stop outstanding work before changing mode. Connect/disconnect can still affect device pins. Viewing and explicit RX recording remain available.",{{L"Mode",observation_only_?L"Observation only":L"Normal operation",{L"Normal operation",L"Observation only"}}},{},[this](const auto& f,std::wstring& error){return set_observation(f[0]==L"Observation only",error);},this);
        form.DoModal();
    }
}
bool CMainDialog::set_observation(bool enabled,std::wstring& error){
    if(observation_only_==enabled)return true;
    if(automation_.active()||automation_id_||file_tx_busy()||controls_pending_id_||profile_job_.busy()||serial_open_.busy()||disconnect_pending_||
       (workbench_&&(workbench_->tx_reserved(0)||workbench_->tx_reserved(1)||workbench_->secondary_.tx_busy()||workbench_->secondary_.changing_connection()))){error=ui::tr(L"Stop outstanding TX / tool / settings work before changing observation mode.");return false;}
    if(!uart_operations_.observation(enabled)){error=ui::tr(L"Wait for pending UART operations.");return false;}
    if(workbench_&&!workbench_->secondary_.queue().observation(enabled)){uart_operations_.observation(observation_only_);error=ui::tr(L"Wait for pending UART operations.");return false;}
    observation_only_=enabled;
    ::CheckMenuItem(::GetMenu(m_hWnd),ID_FD_OBSERVE,MF_BYCOMMAND|(enabled?MF_CHECKED:MF_UNCHECKED));
    poll_uart_operations();update_source_status();
    if(workbench_&&workbench_->GetSafeHwnd()){workbench_->refresh_group(0);workbench_->refresh_group(1);}
    show_status_message(ui::tr(enabled?L"Observation only: TX, serial controls and Break are blocked.":L"Normal operation restored; no command was started."),8000);
    return true;
}
void CMainDialog::monitor_cadence(){
    MonitorModal modal(discovery_modal_);const auto interval=message_page_.cadence().interval;
    UartToolForm form(L"Display cadence / cost",L"Presentation only; acquisition, numeric drain and raw recording keep their existing limits. User navigation/reflow remains immediate. Update cost excludes deferred painting. Samples remain unchanged while hidden or paused; a slower cadence does not fix a long individual update.",{{L"Presentation interval (ms)",std::to_wstring(interval),{L"50",L"100",L"250"}}},
        [this](const auto&){const auto& c=message_page_.cadence();const auto now=GetTickCount64();auto text=ui::tr(L"Messages: update / max / sample age / pending (ms)")+L" "+presentation_ms(c.cost)+L" / "+presentation_ms(c.maximum)+L" / "+(c.last_end&&now>=c.last_end?std::to_wstring(now-c.last_end):ui::tr(L"Unavailable"))+L" / "+std::to_wstring(c.pending&&now>=c.pending?now-c.pending:0);if(workbench_)for(const auto& group:workbench_->groups_)text+=L"\r\n"+ui::tr(L"Workbench: update / max / sample age / pending (ms)")+L" "+presentation_ms(group.cadence.cost)+L" / "+presentation_ms(group.cadence.maximum)+L" / "+(group.cadence.last_end&&now>=group.cadence.last_end?std::to_wstring(now-group.cadence.last_end):ui::tr(L"Unavailable"))+L" / "+std::to_wstring(group.cadence.pending&&now>=group.cadence.pending?now-group.cadence.pending:0);return text;},
        [this](const auto& f,std::wstring& error){const unsigned value=f[0]==L"100"?100:f[0]==L"250"?250:50;if(!save_ini_patch(settings_path_,{{L"Messages",L"PresentationInterval",std::to_wstring(value)}},error))return false;message_page_.presentation_interval(value);if(workbench_)for(auto& group:workbench_->groups_)group.cadence.interval=value;return true;},this);
    form.DoModal();
}
