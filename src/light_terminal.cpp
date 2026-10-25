#include "light_terminal.h"
#include <algorithm>
#include <deque>
FormattedMessage format_light_terminal(std::string_view bytes,const MessageDisplayOptions& options) {
    std::deque<std::wstring> lines(1);size_t column=0;std::string run;
    auto display=options;display.clock=MessageClock::None;display.ansi_sgr=false;
    const auto flush=[&] {const auto formatted=format_message(run,0,{},display);run.clear();auto& line=lines.back();for(const wchar_t c:formatted.text){if(column>=4096) continue;if(column>=line.size()) line.resize(column+1,L' ');line[column++]=c;}};
    for(size_t i=0;i<bytes.size();++i) {const auto c=static_cast<unsigned char>(bytes[i]);
        if(c=='\r'||c=='\n'||c=='\b') {flush();if(c=='\r') column=0;else if(c=='\b') {if(column) --column;}else {lines.emplace_back();column=0;if(lines.size()>8192) lines.pop_front();}continue;}
        if(c==27 && i+2<bytes.size() && bytes[i+1]=='[') {
            size_t end=i+2;char mode='0';if(bytes[end]>='0'&&bytes[end]<='2') mode=bytes[end++];
            if(end<bytes.size() && bytes[end]=='K') {flush();auto& line=lines.back();if(mode=='0') line.resize(std::min(column,line.size()));else if(mode=='1') std::fill(line.begin(),line.begin()+std::min(column+1,line.size()),L' ');else line.clear();i=end;continue;}
        }
        run+=static_cast<char>(c);
    }
    flush();FormattedMessage result;for(size_t i=0;i<lines.size();++i){if(i) result.text+=L"\r\n";result.text+=lines[i];}return result;
}
