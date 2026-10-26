#include "uart_workbench.h"
#include "binary_inspector.h"
void UartWorkbench::inspect_dialog(size_t port){
    std::vector<TrafficRecord> frames(groups_[port].packets.begin(),groups_[port].packets.end());
    if(frames.empty()){const auto raw=(port?secondary_.traffic:*primary_).after(0,32);frames=raw.records;}
    if(frames.empty()){warn(L"No retained packets on this port. Receive data first; Packet inspection selects the framing mode.");return;}
    BinaryFrameConfig config;std::wstring error;if(!workbench_binary_config(binary_[port].fields,config,error)){warn(error);return;}
    if(config.codec!=WireCodec::None){WireDecoder decoder;decoder.configure(config.codec);std::vector<TrafficRecord> decoded;uint64_t generation=UINT64_MAX;for(const auto& raw:frames){if(raw.tx)continue;if(generation!=raw.generation){decoder.reset();generation=raw.generation;}const auto packets=decoder.feed(raw.bytes,raw.receipt_ms,raw.sample_seconds);for(size_t i=0;i<packets.size()&&decoded.size()<256;++i){auto value=raw;value.bytes=packets[i];value.receipt_ms=decoder.stamps()[i].ms;decoded.push_back(std::move(value));}}frames=std::move(decoded);if(frames.empty()){warn(L"No complete decoded wire packets in this retained snapshot. Raw Messages/recording are unchanged.");return;}}
    BinaryInspector dialog(this,std::move(frames),std::move(config));dialog.DoModal();
}
