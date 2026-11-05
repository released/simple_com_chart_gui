#include "light_terminal.h"
#include <algorithm>
#include <deque>
FormattedMessage format_light_terminal(std::string_view bytes,const MessageDisplayOptions& options,
    uint64_t offset,const std::vector<MessageReceipt>& receipts,int64_t origin_ms) {
    struct Line {std::wstring text;int64_t unix_ms=0;bool stamped=false;};
    std::deque<Line> lines(1);size_t column=0,run_begin=0,receipt=0;std::string run;
    const auto time_at=[&](size_t i){while(receipt+1<receipts.size()&&receipts[receipt+1].offset<=offset+i)++receipt;return receipts.empty()?origin_ms:receipts[receipt].unix_ms;};
    if(!origin_ms)origin_ms=time_at(0);
    auto display=options;display.clock=MessageClock::None;display.ansi_sgr=false;
    const auto flush=[&] {
        const auto formatted=format_message(run,0,{},display);
        if(column>=4096||formatted.text.empty()){run.clear();return;}
        auto& row=lines.back();auto& line=row.text;
        if(options.clock!=MessageClock::None&&!row.stamped){
            const auto first=display.suppress_nul?run.find_first_not_of('\0'):0;
            row.unix_ms=time_at(run_begin+first);row.stamped=true;
        }
        run.clear();const auto count=std::min(formatted.text.size(),4096-column);
        if(line.size()<column)line.resize(column,L' ');
        line.replace(column,std::min(count,line.size()-column),formatted.text.data(),count);
        column+=count;
    };
    for(size_t i=0;i<bytes.size();++i) {const auto c=static_cast<unsigned char>(bytes[i]);
        if(c=='\r'||c=='\n'||c=='\b') {flush();if(c=='\r') column=0;else if(c=='\b') {if(column) --column;}else {lines.emplace_back();column=0;if(lines.size()>8192) lines.pop_front();}continue;}
        if(c==27 && i+2<bytes.size() && bytes[i+1]=='[') {
            size_t end=i+2;char mode='0';if(bytes[end]>='0'&&bytes[end]<='2') mode=bytes[end++];
            if(end<bytes.size() && bytes[end]=='K') {flush();auto& line=lines.back().text;if(mode=='0') line.resize(std::min(column,line.size()));else if(mode=='1') std::fill(line.begin(),line.begin()+std::min(column+1,line.size()),L' ');else line.clear();i=end;continue;}
        }
        if(run.empty())run_begin=i;
        run+=static_cast<char>(c);
    }
    flush();FormattedMessage result;size_t size=(lines.size()-1)*2;for(const auto& row:lines)size+=row.text.size();result.text.reserve(size);
    bool first=true;int64_t previous=0;
    for(size_t i=0;i<lines.size();++i){
        if(i)result.text+=L"\r\n";const auto& row=lines[i];
        if(!row.text.empty()&&row.stamped){if(first)previous=row.unix_ms;result.text+=format_message_timestamp(row.unix_ms,origin_ms,previous,options.clock);previous=row.unix_ms;first=false;}
        result.text+=row.text;
    }return result;
}
