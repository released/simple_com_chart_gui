#include "uart_workbench.h"
#include "settings_fields.h"
#include <cmath>
void UartWorkbench::reload_preferences(){
    if(secondary_.reserved()||primary_status_.active)return;
    const bool instruments_visible=instruments_.GetSafeHwnd()&&instruments_.IsWindowVisible();
    const bool signals_visible=signals_.GetSafeHwnd()&&signals_.IsWindowVisible();
    if(signals_.GetSafeHwnd())signals_.DestroyWindow();
    if(instruments_.GetSafeHwnd())instruments_.DestroyWindow();
    const auto path=ini_;ini_.clear();pacing_={};for(auto& group:groups_)group.save_error.clear();initialize(path,primary_);
    if(ready_){load_b();load_views();ratio_=std::clamp(GetPrivateProfileIntW(L"DualCOM",L"SplitPercent",50,ini_.c_str())/100.0,0.25,0.75);layout();}
    if(instruments_visible)instruments_.show(this,ini_);
    if(signals_visible)signals_.show(this,ini_);
}
void UartWorkbench::load_framing(){for(size_t i=0;i<2;++i){auto& group=groups_[i];std::string detail;group.framer.configure(TrafficFraming{},detail);group.packets.clear();group.packet_bytes=0;group.dirty=true;
    wchar_t value[1024]{};const auto section=i?L"Workbench.Framing.B":L"Workbench.Framing.A";const auto n=GetPrivateProfileStringW(section,L"Configuration",L"",value,1024,ini_.c_str());if(!n)continue;
    try{if(n>=1023)throw std::runtime_error("Truncated framing.");const auto fields=unpack_settings(value,4);double m=0,b=0,t=0;if(!uart_number(fields[0],0,5,m)||!uart_number(fields[1],1,65536,b)||!uart_number(fields[2],1,60000,t)||m!=std::floor(m)||b!=std::floor(b)||t!=std::floor(t))throw std::runtime_error("Invalid framing counts.");TrafficFraming config;config.mode=static_cast<TrafficFraming::Mode>(static_cast<int>(m));config.bytes=static_cast<size_t>(b);config.idle_ms=static_cast<unsigned>(t);if(!uart_unhex(message_utf8(fields[3]),config.header)||!group.framer.configure(config,detail))throw std::runtime_error("Invalid framing schema.");}
    catch(...){group.save_error=L"Invalid saved framing; read-chunk mode selected. Review Packet inspection.";}
}}
void UartWorkbench::persist_view(size_t i){const auto value=pack_settings({std::to_wstring(groups_[i].mode),::SendMessageW(groups_[i].follow,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0"});if(!WritePrivateProfileStringW(i?L"Workbench.View.B":L"Workbench.View.A",L"Configuration",value.c_str(),ini_.c_str()))groups_[i].save_error=L"Cannot save display preferences.";}
void UartWorkbench::load_views(){for(size_t i=0;i<2;++i){auto& group=groups_[i];group.mode=0;bool follow=true;wchar_t value[1024]{};const auto n=GetPrivateProfileStringW(i?L"Workbench.View.B":L"Workbench.View.A",L"Configuration",L"",value,1024,ini_.c_str());if(n)try{const auto fields=unpack_settings(value,2);double mode=0;if(n>=1023||!uart_number(fields[0],0,3,mode)||mode!=std::floor(mode)||(fields[1]!=L"0"&&fields[1]!=L"1"))throw std::runtime_error("Invalid view settings.");group.mode=static_cast<int>(mode);follow=fields[1]==L"1";}catch(...){group.save_error=L"Invalid saved display settings; ASCII/follow selected.";}::SendMessageW(group.view,CB_SETCURSEL,group.mode,0);::SendMessageW(group.follow,BM_SETCHECK,follow?BST_CHECKED:BST_UNCHECKED,0);group.dirty=true;}}
