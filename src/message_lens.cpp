#include "message_lens.h"
#include <algorithm>
#include <stdexcept>
namespace {
bool same(const MessageDisplayOptions& a,const MessageDisplayOptions& b){return a.encoding==b.encoding&&a.clock==b.clock&&a.newline==b.newline&&a.code_page==b.code_page&&a.ansi_sgr==b.ansi_sgr&&a.suppress_nul==b.suppress_nul;}
int64_t receipt_at(const MessageBuffer& b,uint64_t offset){const auto& r=b.receipts();const auto p=std::upper_bound(r.begin(),r.end(),offset,[](uint64_t v,const auto& row){return v<row.offset;});return p==r.begin()?(r.empty()?0:r.front().unix_ms):(p-1)->unix_ms;}
}
bool MessageLens::configure(const MessageLensConfig& c,std::string& error){
    if(c.before>10||c.after>10||c.include.size()>256||c.exclude.size()>256){error="Display lens: patterns <=256 UTF-8 bytes; context 0..10 rows.";return false;}
    LineFilter next,excluded;if(!next.configure(c.include,"",c.regex,error)||!excluded.configure("",c.exclude,c.regex,error))return false;filter_=std::move(next);exclusion_=std::move(excluded);config_=c;reset();return true;
}
const MessageLensView& MessageLens::update(const MessageBuffer& buffer,MessageDisplayOptions options,bool rich) {
    if(options.encoding==MessageEncoding::Hex)throw std::runtime_error("Display lens requires raw text mode, not HEX.");
    const auto& bytes=buffer.bytes();
    if(offset_!=buffer.offset()||bytes.size()<size_||!options_valid_||!same(options,options_)||rich_!=rich)cached_.clear();
    options_=options;options_valid_=true;rich_=rich;offset_=buffer.offset();size_=bytes.size();
    size_t first=0;
    if(!cached_.empty()){first=static_cast<size_t>(cached_.back().offset-offset_);cached_.pop_back();}
    int64_t previous=receipt_at(buffer,offset_);
    for(auto row=cached_.rbegin();row!=cached_.rend();++row)if(row->text.text.find_first_not_of(L"\r\n")!=std::wstring::npos){previous=row->receipt;break;}
    while(first<bytes.size()) {
        size_t end=first;
        for(;end<bytes.size();++end){const bool crlf=bytes[end]=='\r'&&end+1<bytes.size()&&bytes[end+1]=='\n';
            const bool stop=options.newline==MessageNewline::Auto?(bytes[end]=='\r'||bytes[end]=='\n'):options.newline==MessageNewline::LF?bytes[end]=='\n':options.newline==MessageNewline::CR?bytes[end]=='\r':crlf;
            if(stop){end+=(crlf&&(options.newline==MessageNewline::Auto||options.newline==MessageNewline::CRLF))?2:1;break;}}
        Cached row;row.offset=offset_+first;row.count=end-first;
        size_t visible=first;while(visible<end&&options.suppress_nul&&bytes[visible]==0)++visible;
        row.receipt=receipt_at(buffer,offset_+visible);
        auto body=bytes.substr(first,end-first);while(!body.empty()&&(body.back()=='\r'||body.back()=='\n'))body.pop_back();
        row.decision=body.size()>65536?LineFilter::WorkLimit:config_.exclude.empty()?LineFilter::Accept:exclusion_.evaluate(body);
        if(row.decision==LineFilter::Accept)row.decision=filter_.evaluate(body);
        auto plain=options;plain.clock=MessageClock::None;
        row.text=format_message(std::string_view(bytes).substr(first,end-first),row.offset,{},plain,buffer.origin_ms());
        if(visible<end&&!row.text.text.empty()&&row.text.text!=L"\r\n"){
            const auto prefix=format_message_timestamp(row.receipt,buffer.origin_ms(),previous,options.clock);
            for(auto& color:row.text.colors)color.start+=prefix.size();row.text.text.insert(0,prefix);previous=row.receipt;
        }
        if(rich){auto& text=row.text.text;for(auto& color:row.text.colors){const auto begin=std::min(color.start,text.size()),past=std::min(color.start+color.count,text.size());const auto before=std::count(text.begin(),text.begin()+begin,L'\n');const auto inside=std::count(text.begin()+begin,text.begin()+past,L'\n');color.start-=before;color.count-=inside;}text.erase(std::remove(text.begin(),text.end(),L'\n'),text.end());}
        cached_.push_back(std::move(row));first=end;
    }
    view_={};view_.total=cached_.size();std::vector<bool> shown(cached_.size());
    for(size_t i=0;i<cached_.size();++i){if(cached_[i].decision==LineFilter::WorkLimit)++view_.limited;if(cached_[i].decision!=LineFilter::Accept)continue;
        const auto a=i>config_.before?i-config_.before:0,b=std::min(cached_.size(),i+config_.after+1);
        for(size_t j=a;j<b;++j)if(cached_[j].decision!=LineFilter::Excluded&&cached_[j].decision!=LineFilter::WorkLimit)shown[j]=true;
    }
    for(size_t i=0;i<cached_.size();++i)if(shown[i]){const auto& row=cached_[i];const auto at=view_.formatted.text.size();view_.formatted.text+=row.text.text;for(auto color:row.text.colors){color.start+=at;view_.formatted.colors.push_back(color);}view_.rows.push_back({row.offset,row.count,at,view_.formatted.text.size(),row.receipt,row.decision==LineFilter::Accept});}
    // Without SGR the whole projection has one foreground, including clock prefixes.
    // Keep one span so retention refreshes do not issue a native style call per row.
    if(!options.ansi_sgr&&!view_.formatted.text.empty())view_.formatted.colors={{0,view_.formatted.text.size(),0x0000ff00}};
    return view_;
}
MessageSearchResults search_retained_messages(const MessageBuffer& buffer,MessageDisplayOptions options,const std::wstring& pattern,bool regex,bool sensitive){
    MessageSearchResults result;if(pattern.empty()){result.error=L"Enter a search pattern.";return result;}
    MessageLens lens;std::string error;lens.configure({},error);options.encoding=options.encoding==MessageEncoding::Hex?MessageEncoding::Ascii:options.encoding;
    const auto& all=lens.update(buffer,options);std::wstring search_error;
    const auto hits=find_message_matches(all.formatted.text,pattern,regex,sensitive,search_error);result.error=search_error;if(!search_error.empty())return result;
    result.matches=hits.size();result.limited=hits.size()==4096;result.buckets.assign(64,0);
    size_t row=0;const auto begin=all.rows.empty()?0:all.rows.front().receipt,end=all.rows.empty()?0:all.rows.back().receipt;
    result.timed=end>begin;result.begin_ms=begin;result.end_ms=end;
    for(const auto& hit:hits){while(row<all.rows.size()&&all.rows[row].end<=hit.start)++row;if(row==all.rows.size())break;const auto& r=all.rows[row];
        if(result.rows.empty()||result.rows.back().offset!=r.offset)result.rows.push_back({r.offset,r.count,0,r.receipt,all.formatted.text.substr(r.start,std::min<size_t>(240,r.end-r.start))});++result.rows.back().hits;
        const auto fraction=end>begin?(static_cast<long double>(r.receipt)-begin)/(static_cast<long double>(end)-begin):0;const auto bucket=std::min<size_t>(63,static_cast<size_t>(std::clamp(fraction,0.0L,1.0L)*64));++result.buckets[bucket];}
    result.frequency=end>begin?L"PC receipt frequency: 64 equal time buckets\r\n":L"Receipt time range unavailable; matches are in bucket 0\r\n";
    for(size_t i=0;i<64;++i)if(result.buckets[i])result.frequency+=std::to_wstring(i)+L": "+std::to_wstring(result.buckets[i])+L" "+std::wstring(std::min<size_t>(40,result.buckets[i]),L'|')+L"\r\n";
    return result;
}
