#include "uart_workbench.h"
#include "binary_inspector.h"
void UartWorkbench::inspect_dialog(size_t port){
    std::vector<TrafficRecord> frames(groups_[port].packets.begin(),groups_[port].packets.end());
    if(frames.empty()){const auto raw=(port?secondary_.traffic:*primary_).after(0,32);frames=raw.records;}
    if(frames.empty()){warn(L"No retained packets on this port. Receive data first; Packet inspection selects the framing mode.");return;}
    BinaryFrameConfig config;std::wstring error;if(!workbench_binary_config(binary_[port].fields,config,error)){warn(error);return;}
    BinaryInspector dialog(this,std::move(frames),std::move(config));dialog.DoModal();
}
