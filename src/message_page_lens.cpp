#include "message_page.h"
#include "light_terminal.h"
#include "ui_language.h"
#include <algorithm>
bool MessagePage::set_display_lens(bool enabled,const MessageLensConfig& config,std::wstring& error){
    if(enabled&&(!raw_text_mode()||options_.ansi_sgr)){error=ui::tr(L"Display lens requires raw text mode. Turn off Light terminal and ANSI SGR, then select ASCII, UTF-8 or ANSI first.");return false;}
    std::string detail;if(!lens_.configure(config,detail)){error=ui::tr(L"Invalid display lens expression; previous view retained.")+L"\r\n"+std::wstring(detail.begin(),detail.end());return false;}
    lens_enabled_=enabled;lens_view_={};bookmarks_.clear();rebuild_=dirty_=true;refresh();error.clear();return true;
}
FormattedMessage MessagePage::format_retained(bool rich){
    const auto options=display_options();
    if(lens_enabled_&&raw_text_mode()){lens_view_=lens_.update(buffer_,options,rich);return lens_view_.formatted;}
    lens_view_={};return terminal_&&options.encoding!=MessageEncoding::Hex?format_light_terminal(buffer_.bytes(),options,buffer_.offset(),buffer_.receipts(),buffer_.origin_ms()):format_message(buffer_.bytes(),buffer_.offset(),buffer_.receipts(),options,buffer_.origin_ms());
}
MessageSearchResults MessagePage::search_snapshot(const std::wstring& pattern,bool regex,bool sensitive)const{
    auto result=search_retained_messages(buffer_,display_options(),pattern,regex,sensitive);
    if(synthetic_){result.timed=false;for(auto& row:result.rows)row.receipt=0;result.frequency=L"Synthetic source: PC receipt frequency unavailable.";}
    return result;
}
