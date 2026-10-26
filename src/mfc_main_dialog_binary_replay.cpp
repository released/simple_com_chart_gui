#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "settings_fields.h"
namespace {std::vector<std::wstring> defaults(){return {L"Text",L"",L"4",L"0",L"1",L"Little",L"0",L"None",L"0",L"Little",L"value,0,u16le",L""};}}
void CMainDialog::reset_binary_replay(){replay_crc_errors_=replay_discarded_=replay_codec_errors_=replay_unknown_types_=replay_layout_errors_=0;replay_frames_.clear();replay_binary_state_=std::make_shared<ReplayBinary>();std::string error;if(replay_binary_enabled_)replay_binary_state_->decoder.configure(replay_binary_config_,error);}
void CMainDialog::load_binary_replay(){
    replay_binary_fields_=defaults();replay_binary_enabled_=false;std::vector<wchar_t> text(32768);const auto n=GetPrivateProfileStringW(L"Replay.Binary",L"Configuration",L"",text.data(),static_cast<DWORD>(text.size()),settings_path_.c_str());
    if(n)try{auto fields=unpack_settings(text.data(),12);if(n>=text.size()-1||(fields[0]!=L"Text"&&fields[0]!=L"Binary"))throw std::runtime_error("Invalid replay mode");auto config_fields=fields;config_fields[0]=L"Enabled";BinaryFrameConfig config;std::wstring error;if(!workbench_binary_config(config_fields,config,error))throw std::runtime_error("Invalid binary replay schema");replay_binary_fields_=std::move(fields);replay_binary_config_=std::move(config);replay_binary_enabled_=replay_binary_fields_[0]==L"Binary";}catch(...){show_status_message(L"Invalid offline binary settings; Text replay selected. Review UART > Binary replay schema.",7000);}
    reset_binary_replay();
}
void CMainDialog::binary_replay_options(){
    if(!advanced_idle()||profile_job_.busy()||replay_io_.busy()||replay_load_.busy()||recorder_.status().active||exporting_){show_status_message(L"Disconnect both ports and finish file/profile work before changing replay schema.",6000);return;}
    const auto f=replay_binary_fields_.empty()?defaults():replay_binary_fields_;
    UartToolForm form(L"Offline binary replay schema",L"Applies only to offline files. Binary uses this independent schema, recorded PC receipt intervals (or explicitly synthetic file time), and no live COM/TX. Apply pauses, seeks to start and clears the current offline view. Existing files and live Binary parser settings stay unchanged.",
        {{L"Offline extraction",f[0],{L"Text",L"Binary"}},{L"Header HEX",f[1]},{L"Frame bytes (0=length)",f[2]},{L"Length offset",f[3]},{L"Length bytes",f[4],{L"1",L"2",L"4"}},{L"Length byte order",f[5],{L"Little",L"Big"}},{L"Add to length",f[6]},{L"Checksum",f[7],{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"}},{L"Checksum start",f[8]},{L"Checksum byte order",f[9],{L"Little",L"Big"}},{L"Fields name,offset,type[,scale,add]",f[10],{},6},{L"Preview HEX",L"",{},3}},
        [](auto fields){fields[0]=L"Enabled";BinaryFrameConfig config;std::wstring error;if(!workbench_binary_config(fields,config,error))return error;TxEntry tx{fields[11],true,TxEnding::None};std::string bytes;if(!make_tx_payload(tx,bytes,error))return error;BinaryFrameDecoder decoder;std::string detail;decoder.configure(config,detail);const auto frames=decoder.feed(bytes,0);std::wstring out=L"Frames "+std::to_wstring(frames.size())+L"; CRC errors "+std::to_wstring(decoder.checksum_errors());for(const auto& frame:frames)for(const auto& value:frame.values)out+=L"\r\n"+uart_wide(value.first)+L" = "+std::to_wstring(value.second);return out;},
        [](auto fields,std::wstring& error){fields[0]=L"Enabled";BinaryFrameConfig config;return workbench_binary_config(fields,config,error);},this);
    if(form.DoModal()!=IDOK||!advanced_idle())return;auto fields=form.values();auto config_fields=fields;config_fields[0]=L"Enabled";BinaryFrameConfig config;std::wstring error;if(!workbench_binary_config(config_fields,config,error))return;fields[11].clear();
    try{const auto encoded=pack_settings(fields);if(!WritePrivateProfileStringW(L"Replay.Binary",L"Configuration",encoded.c_str(),settings_path_.c_str())){show_status_message(L"Cannot save replay schema; previous settings retained.",6000);return;}}catch(...){show_status_message(L"Invalid/oversized replay schema.",6000);return;}
    replay_binary_fields_=std::move(fields);replay_binary_config_=std::move(config);replay_binary_enabled_=replay_binary_fields_[0]==L"Binary";replay_.pause(GetTickCount64());if(replay_.data()){reset_offline_view();replay_.seek(0,GetTickCount64());}else reset_binary_replay();
    show_status_message(L"Offline extraction saved; use Offline replay to load/play a recording. No COM opened.",6000);
}
void CMainDialog::drain_binary_replay(){
    if(!replay_.data()||!replay_binary_enabled_||replay_frames_.empty())return;const auto start=GetTickCount64();unsigned count=0;double last=0;
    while(!replay_frames_.empty()&&count++<64&&GetTickCount64()-start<8){auto frame=std::move(replay_frames_.front());replay_frames_.pop_front();std::unordered_map<std::string,double> values(frame.values.begin(),frame.values.end());last=frame.sample_seconds;accept_numeric("Offline binary: "+uart_hex(frame.bytes),values,last,UINT64_MAX,0,0,frame.generation,UINT16_MAX);}
    sync_channels();if(::IsWindowVisible(data_page_.hwnd()))data_page_.refresh(last);if(!snapshot_&&::IsWindowVisible(plot_view_.hwnd()))plot_view_.update_from_model(last);
}
