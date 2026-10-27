#include "message_page.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include <iomanip>
#include <sstream>

namespace {
std::wstring kilobytes(uint64_t bytes){std::wostringstream out;out.imbue(std::locale::classic());out<<std::fixed<<std::setprecision(2)<<static_cast<double>(bytes)/1024<<L" KB";return out.str();}
void enable(HWND control,bool value){if(IsWindow(control)&&(IsWindowEnabled(control)!=FALSE)!=value)EnableWindow(control,value);}
}
void MessagePage::create_log_controls(){
    const auto button=[&](const wchar_t* text,int id){return ui::create_window(L"BUTTON",text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,0,0,1,1,hwnd_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);};
    save_log_=button(L"Save As...",SaveLog);start_log_=button(L"Start recording",StartLog);stop_log_=button(L"Stop recording",StopLog);
    log_info_=ui::create_window(L"STATIC",L"No log saved yet",WS_CHILD|WS_VISIBLE|SS_NOPREFIX,0,0,1,1,hwnd_,nullptr,nullptr,nullptr);
    enable(stop_log_,false);recording_label_=ui::tr(L"Recording off");update_log_info();
}
void MessagePage::set_exporting(bool busy){export_busy_=busy;enable(save_log_,!busy);update_log_info();}
void MessagePage::stage_log_export(std::wstring filename,uint64_t bytes){pending_log_=std::filesystem::path(filename).filename().wstring();pending_log_bytes_=bytes;update_log_info();}
void MessagePage::finish_log_export(bool success){if(success){saved_log_=std::move(pending_log_);saved_log_bytes_=pending_log_bytes_;}pending_log_.clear();pending_log_bytes_=0;update_log_info();}
void MessagePage::set_recording_status(const RecordingStatus& status,bool can_start){
    enable(start_log_,can_start&&!status.active&&!status.stopping);enable(stop_log_,status.active&&!status.stopping);
    recording_name_=std::filesystem::path(status.log_filename).filename().wstring();recording_bytes_=status.log_bytes;
    const auto label=status.detail.find(L"ERROR")!=status.detail.npos?L"Recording error":status.lost?L"Recording stopped (input lost)":status.limit_reached?L"Recording stopped (limit)":status.active?(status.stopping?L"Finishing recording":recording_name_.empty()?L"Starting recording":L"Recording in progress"):recording_name_.empty()?L"Recording off":L"Recorded";
    recording_label_=ui::tr(label);update_log_info();
}
void MessagePage::update_log_info(){
    if(!IsWindow(log_info_))return;
    auto text=export_busy_?ui::tr(L"Saving log..."):saved_log_.empty()?ui::tr(L"No log saved yet"):ui::tr(L"Saved log:")+L" "+saved_log_+L" | "+kilobytes(saved_log_bytes_);
    text+=L"    |    "+recording_label_;if(!recording_name_.empty())text+=L": "+recording_name_+L" | "+kilobytes(recording_bytes_);
    if(text==log_info_text_)return;log_info_text_=std::move(text);SetWindowTextW(log_info_,log_info_text_.c_str());
    RECT client{};GetClientRect(hwnd_,&client);const int height=std::max(control_font_height(log_info_)+12,control_wrapped_text_height(log_info_,client.right));
    if(log_info_height_&&height!=log_info_height_)layout(client.right,client.bottom);
}
