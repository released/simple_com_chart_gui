#include "message_page.h"
#include <commctrl.h>
#include <algorithm>
bool MessagePage::navigate_source(uint64_t offset,size_t count) {
    const auto size=buffer_.bytes().size();if(paused_ || terminal_ || source_end_==UINT64_MAX || source_end_<size || offset<source_end_-size || offset>=source_end_) return false;
    if(!IsWindowVisible(hwnd_)) {navigation_offset_=offset;navigation_count_=count;return true;}
    const auto first=static_cast<size_t>(offset-(source_end_-size));const auto last=std::min(size,first+count);
    SendMessageW(follow_,BM_SETCHECK,BST_UNCHECKED,0);dirty_=true;refresh();
    auto options=options_;if(synthetic_) options.clock=MessageClock::None;options.encoding=static_cast<MessageEncoding>(SendMessageW(mode_,CB_GETCURSEL,0,0));
    const auto index=[&](size_t n){auto text=format_message(std::string_view(buffer_.bytes()).substr(0,n),buffer_.offset(),buffer_.receipts(),options,buffer_.origin_ms()).text;if(view_==rich_view_) text.erase(std::remove(text.begin(),text.end(),L'\n'),text.end());return text.size();};
    size_t start=index(first),end=index(last);
    if(hex_mode()) {const size_t line=first/16;start=static_cast<size_t>(SendMessageW(view_,EM_LINEINDEX,line,0));end=start+std::min<size_t>(80,rendered_text_.size()-std::min(start,rendered_text_.size()));}
    SendMessageW(view_,EM_SETSEL,start,end);SendMessageW(view_,EM_SCROLLCARET,0,0);return true;
}
void MessagePage::locate_cursor_source() {
    if(terminal_ || paused_ || buffer_.bytes().empty()) {SetWindowTextW(info_,L"Locate requires a current, unpaused raw-monitor display (not light terminal).");return;}
    DWORD caret=0;SendMessageW(view_,EM_GETSEL,reinterpret_cast<WPARAM>(&caret),0);
    size_t byte=0;
    if(hex_mode()) byte=static_cast<size_t>(SendMessageW(view_,EM_LINEFROMCHAR,caret,0))*16;
    else {
        auto options=options_;if(synthetic_) options.clock=MessageClock::None;
        size_t a=0,b=buffer_.bytes().size();
        while(a<b) {const auto mid=a+(b-a+1)/2;auto text=format_message(std::string_view(buffer_.bytes()).substr(0,mid),buffer_.offset(),buffer_.receipts(),options,buffer_.origin_ms()).text;if(view_==rich_view_) text.erase(std::remove(text.begin(),text.end(),L'\n'),text.end());if(text.size()<=caret) a=mid;else b=mid-1;}byte=a;
    }
    if(source_end_==UINT64_MAX || source_end_<buffer_.bytes().size() || byte>=buffer_.bytes().size()) return;
    const uint64_t offset=source_end_-buffer_.bytes().size()+byte;SendMessageW(GetParent(hwnd_),kLocateSource,0,reinterpret_cast<LPARAM>(&offset));
}
