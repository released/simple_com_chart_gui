#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "uart_files.h"
#include <cmath>
bool UartWorkbench::start_forwarding(ForwardOptions options,std::wstring& error){
    if(!acquire(0,ToolRun::Forward,error))return false;if(!acquire(1,ToolRun::Forward,error)){runs_[0].kind=ToolRun::None;return false;}
    std::string detail;if(!forwarder_.start(options,{primary_status_.generation,secondary_.generation()},GetTickCount64(),detail)){runs_[0].kind=runs_[1].kind=ToolRun::None;error=uart_wide(detail);return false;}
    for(auto& run:runs_)run.status=L"Forwarding: exclusive A/B TX; Stop either port stops both.";return true;
}
void UartWorkbench::stop_forwarding(const std::string& reason){forwarder_.stop(reason);for(size_t i=0;i<2;++i){auto& run=runs_[i];if(run.kind!=ToolRun::Forward)continue;if(run.id){if(i==0){Request r;r.action=Request::CancelTx;r.id=run.id;request(r);}else secondary_.cancel(run.id);}run.kind=ToolRun::None;run.status=uart_wide(reason)+L" | driver accepted "+std::to_wstring(forwarder_.accepted())+L" bytes";commands_.state(i,run.status);}}
void UartWorkbench::poll_forwarding(){if(!forwarder_.active())return;const auto now=GetTickCount64();
    if(!primary_status_.connected||!secondary_.connected()||primary_status_.generation!=runs_[0].generation||secondary_.generation()!=runs_[1].generation){stop_forwarding("STOPPED: port disconnected/changed; no automatic forwarding resume");return;}
    forwarder_.tick(now);
    for(size_t i=0;i<2&&forwarder_.active();++i){auto& run=runs_[i];const auto batch=(i==0?*primary_:secondary_.traffic).after(run.sequence,32);if(batch.revision!=run.revision||(!batch.records.empty()&&batch.records.front().sequence>run.sequence+1)){stop_forwarding("STOPPED: raw history gap; cannot guarantee forwarding completeness");return;}
        for(const auto& record:batch.records){run.sequence=record.sequence;forwarder_.receive(i,record,now);if(!forwarder_.active())break;}}
    if(!forwarder_.active()){stop_forwarding(forwarder_.status());return;}
    for(size_t i=0;i<2;++i){if(runs_[i].id)continue;std::string bytes;if(forwarder_.next(i,bytes)){std::wstring error;if(!submit_tool(i,std::move(bytes),error)){stop_forwarding("STOPPED: forwarding TX submission failed; no retry");return;}}}
}
void UartWorkbench::forward_dialog(){UartToolForm form(L"Opt-in A/B forwarding",L"Requires two connected ports and reserves both TX queues. Raw logs remain separate. RX bytes are forwarded without parsing/modification. Stops on queue/history gap, TX error/partial write, disconnect, time/byte limit or possible echoed loop. Stop either port stops both. Windows receipt/driver acceptance is not wire timing. No automatic start/reconnect resume.",{{L"Direction",L"A -> B",{L"A -> B",L"B -> A",L"Both"}},{L"Maximum duration (s)",L"300"},{L"Maximum total RX (MiB)",L"1"}},{},[](const auto& f,std::wstring& error){double seconds=0,bytes=0;if(!uart_number(f[1],1,3600,seconds)||seconds!=std::floor(seconds)||!uart_number(f[2],1,1024,bytes)||bytes!=std::floor(bytes)){error=L"Duration 1..3600 s and byte budget 1..1024 MiB, whole numbers.";return false;}return true;},this);
    if(form.DoModal()!=IDOK||shutdown_)return;const auto f=form.values();ForwardOptions options;options.direction=f[0]==L"A -> B"?ForwardOptions::AtoB:f[0]==L"B -> A"?ForwardOptions::BtoA:ForwardOptions::Both;double seconds=0,bytes=0;uart_number(f[1],1,3600,seconds);uart_number(f[2],1,1024,bytes);options.seconds=static_cast<unsigned>(seconds);options.byte_limit=static_cast<uint64_t>(bytes)*1024*1024;
    const auto a=primary_status_.generation,b=secondary_.generation();const auto prompt=L"Start "+f[0]+L" forwarding now?\r\n\r\nThis sends received data to the other physical device and may execute device commands. Review both COM selections/wiring. Both TX queues will be reserved until Stop/error/limit.";
    if(notice_(m_hWnd,prompt.c_str(),L"Start physical serial forwarding",MB_OKCANCEL|MB_ICONWARNING)!=IDOK)return;if(shutdown_||!GetSafeHwnd()||a!=primary_status_.generation||b!=secondary_.generation()){warn(L"Connection changed; forwarding not started.");return;}std::wstring error;if(!start_forwarding(options,error))warn(error);
}
