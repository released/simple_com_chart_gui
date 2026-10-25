#include "mfc_main_dialog.h"
#include <sstream>

void CMainDialog::load_display_preferences() {
    std::vector<wchar_t> value(65536);const auto n=GetPrivateProfileStringW(L"ChannelDisplay",L"Configuration",L"",value.data(),static_cast<DWORD>(value.size()),settings_path_.c_str());
    try {std::string decoded;if(n>=value.size()-1 || !uart_unhex(message_utf8(value.data()),decoded)) throw std::runtime_error("Invalid settings");display_preferences_=DisplayPreferences::decode(decoded);}
    catch(...) {display_preferences_=DisplayPreferences{};show_status_message(L"Invalid channel display settings; raw-value display active.",6000);}
    model_.set_display_preferences(&display_preferences_);channel_panel_.set_display_preferences(&display_preferences_,true);data_page_.set_display_preferences(&display_preferences_);
}
void CMainDialog::channel_display_request(const AdvancedTools::Request& request) {
    if(request.fields.size()!=6) return;const auto key=message_utf8(request.fields[0]);
    if(key.empty() || std::find(capture_.keys().begin(),capture_.keys().end(),key)==capture_.keys().end()) {advanced_tools_.notice(request.page,L"Choose an existing channel key from the searchable list.");return;}
    if(request.action==0) {
        const auto d=display_preferences_.get(key);advanced_tools_.set_field(request.page,1,uart_wide(d.alias));advanced_tools_.set_field(request.page,2,uart_wide(d.unit));advanced_tools_.set_field(request.page,3,std::to_wstring(d.decimals));
        std::wostringstream scale,offset;scale.precision(17);offset.precision(17);scale<<d.scale;offset<<d.offset;advanced_tools_.set_field(request.page,4,scale.str());advanced_tools_.set_field(request.page,5,offset.str());return;
    }
    auto next=display_preferences_;ChannelDisplay d;std::string error;double decimals=0;
    if(request.action==2) next.erase(key);
    else {
        d.alias=message_utf8(request.fields[1]);d.unit=message_utf8(request.fields[2]);
        if(!uart_number(request.fields[3],-1,12,decimals) || decimals!=static_cast<int>(decimals) || !uart_number(request.fields[4],-1e12,1e12,d.scale) || !uart_number(request.fields[5],-1e100,1e100,d.offset)) {advanced_tools_.notice(request.page,L"Enter decimals -1..12, finite scale/offset.");return;}
        d.decimals=static_cast<int>(decimals);if(!next.set(key,d,error)) {advanced_tools_.notice(request.page,uart_wide(error));return;}
    }
    const auto encoded=uart_wide(uart_hex(next.encode()));
    if(!WritePrivateProfileStringW(L"ChannelDisplay",L"Configuration",encoded.c_str(),settings_path_.c_str())) {advanced_tools_.notice(request.page,L"Cannot save display settings; previous values retained.");return;}
    display_preferences_=std::move(next);channel_panel_.set_display_preferences(&display_preferences_,true);data_page_.set_display_preferences(&display_preferences_);
    if(snapshot_) {plot_view_.set_frozen(false);snapshot_=false;::SetWindowTextW(btn_snapshot_,L"Snapshot");::SendMessageW(btn_snapshot_,BM_SETCHECK,BST_UNCHECKED,0);}
    plot_view_.update_from_model(now_seconds());advanced_tools_.notice(request.page,L"Display saved. Raw capture/export/recording unchanged; non-finite transformed values show n/a.");
}
