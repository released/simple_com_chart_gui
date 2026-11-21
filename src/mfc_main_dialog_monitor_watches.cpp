#include "mfc_main_dialog.h"
#include "discovery_list.h"

void CMainDialog::monitor_alarm_view(){
    MonitorModal modal(discovery_modal_);
    while(!closing_serial_){
        auto snapshot=watch_alarms_.rows();
        const auto refresh=[&]{snapshot=watch_alarms_.rows();DiscoveryListDialog::Rows rows;
        for(const auto& alarm:snapshot){const auto& e=alarm.event;const wchar_t* levels[]={L"Info",L"Warning",L"Critical"};
            rows.push_back({std::to_wstring(alarm.id),e.source?L"B":L"A",uart_wide(e.key),ui::tr(levels[std::min<unsigned>(2,e.severity)]),ui::tr(uart_wide(e.state)),ui::tr(alarm.active?L"Active":L"Resolved"),ui::tr(alarm.acknowledged?L"Acknowledged":alarm.snooze_until?L"Snoozed":L"Unacknowledged")});
        }
        SYSTEMTIME time{};GetLocalTime(&time);wchar_t stamp[40]{};swprintf_s(stamp,L"%04u-%02u-%02u %02u:%02u:%02u",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
        return std::make_pair(std::move(rows),ui::tr(L"Snapshot at:")+L" "+stamp+L"\r\n"+ui::tr(L"Refresh to update alarm states. Actions use the selected alarm ID.")+L"\r\n"+ui::tr(L"Latest 128 session alarms. Acknowledgement keeps the condition. Snooze reminds once after 60 seconds only while active and unacknowledged. No TX or recording."));};
        auto initial=refresh();
        DiscoveryListDialog list(this,L"Watch alarm review",{L"ID",L"Source",L"Channel",L"Severity",L"Condition",L"State",L"Review"},std::move(initial.first),std::move(initial.second),L"Acknowledge",L"Snooze 60 seconds");list.refresher(refresh);
        if(list.DoModal()!=IDOK||list.selection()>=snapshot.size())break;const auto id=snapshot[list.selection()].id;
        const bool ok=list.alternate()?watch_alarms_.snooze(id,GetTickCount64()):watch_alarms_.acknowledge(id);
        if(!ok)show_status_message(ui::tr(L"Alarm changed, resolved or expired; no reminder scheduled."),5000);
    }
}
