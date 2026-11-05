#include "monitor_features.h"
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <stdexcept>
#include <cwchar>

namespace {
bool safe_text(std::wstring_view s,size_t limit) {
    return !s.empty()&&s.size()<=limit&&std::none_of(s.begin(),s.end(),[](wchar_t c){return c<32||c==127;});
}
bool resource_file_path(const std::wstring& text){
    const std::filesystem::path path(text);
    if(!path.is_absolute()||text.rfind(L"\\\\.\\",0)==0||text.rfind(L"\\\\?\\",0)==0)return false;
    for(const auto& part:path){if(part==path.root_name()||part==path.root_directory())continue;auto name=part.wstring();if(name==L"."||name==L"..")continue;if(name.find(L':')!=name.npos)return false;while(!name.empty()&&(name.back()==L' '||name.back()==L'.'))name.pop_back();name.resize(name.find(L'.')==name.npos?name.size():name.find(L'.'));std::transform(name.begin(),name.end(),name.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towupper(c));});
        if(name==L"CON"||name==L"PRN"||name==L"AUX"||name==L"NUL"||name==L"CONIN$"||name==L"CONOUT$"||(name.size()==4&&(name.substr(0,3)==L"COM"||name.substr(0,3)==L"LPT")&&name[3]>=L'0'&&name[3]<=L'9'))return false;}
    return true;
}
void validate_resource(const RecentResource& r) {
    if(static_cast<unsigned>(r.kind)>3||!safe_text(r.name,64)||!safe_text(r.path,1024)||!resource_file_path(r.path))
        throw std::runtime_error("Invalid recent resource; previous list retained.");
    if(!r.related.empty()&&(!safe_text(r.related,1024)||!resource_file_path(r.related)))throw std::runtime_error("Invalid related resource path.");
}
}
bool parse_keyword_rules(std::wstring_view input,std::vector<KeywordRule>& out,std::wstring& error) {
    error.clear(); std::vector<KeywordRule> staged;
    const auto fail=[&]{error=L"Use at most 16 rules: color,word/line,0/1,text (text 1..128 characters). Colors: red,yellow,cyan,magenta,blue,white,green. 1 means case sensitive.";return false;};
    if(input.size()>4096)return fail();
    for(size_t p=0;p<input.size();) {
        auto end=input.find(L'\n',p);if(end==input.npos)end=input.size();auto row=input.substr(p,end-p);p=end+1;
        if(!row.empty()&&row.back()==L'\r')row.remove_suffix(1);if(row.empty())continue;
        const auto a=row.find(L','),b=a==row.npos?row.npos:row.find(L',',a+1),c=b==row.npos?row.npos:row.find(L',',b+1);
        if(c==row.npos||staged.size()==16)return fail();KeywordRule r;const auto color=row.substr(0,a),scope=row.substr(a+1,b-a-1),sensitive=row.substr(b+1,c-b-1);
        const std::pair<const wchar_t*,COLORREF> colors[]={{L"red",RGB(255,90,90)},{L"yellow",RGB(255,230,80)},{L"cyan",RGB(80,230,230)},{L"magenta",RGB(245,120,245)},{L"blue",RGB(130,170,255)},{L"white",RGB(245,245,245)},{L"green",RGB(0,255,0)}};
        bool known=false;for(const auto& item:colors)if(color==item.first){r.color=item.second;known=true;break;}
        if(!known||(scope!=L"word"&&scope!=L"line")||(sensitive!=L"0"&&sensitive!=L"1")||!safe_text(row.substr(c+1),128))return fail();
        r.text=row.substr(c+1);r.line=scope==L"line";r.match_case=sensitive==L"1";staged.push_back(std::move(r));
    }
    out=std::move(staged);return true;
}
std::vector<KeywordSpan> keyword_spans(std::wstring_view text,const std::vector<KeywordRule>& rules,size_t begin,bool& limited) {
    std::vector<KeywordSpan> spans;limited=false;begin=std::min(begin,text.size());
    size_t work=0;constexpr size_t maximum_work=4*1024*1024;
    for(const auto& rule:rules) {
        if(rule.text.empty())continue;
        if(rule.text.size()>128){limited=true;return spans;}
        const auto fold=[&](wchar_t c){return rule.match_case?c:static_cast<wchar_t>(std::towlower(c));};
        size_t prefix[128]{};
        for(size_t i=1,q=0;i<rule.text.size();++i) {
            while(q&&fold(rule.text[i])!=fold(rule.text[q]))q=prefix[q-1];
            if(fold(rule.text[i])==fold(rule.text[q]))++q;
            prefix[i]=q;
        }
        for(size_t p=begin,q=0;p<text.size();) {
            if(++work>maximum_work){limited=true;return spans;}
            const auto c=fold(text[p]);
            while(q&&c!=fold(rule.text[q])) {
                if(++work>maximum_work){limited=true;return spans;}
                q=prefix[q-1];
            }
            if(c==fold(rule.text[q]))++q;
            ++p;
            if(q!=rule.text.size())continue;
            const size_t at=p-q;size_t first=at,last=p;
            if(rule.line) {
                const auto before=text.find_last_of(L"\r\n",at?at-1:0);
                first=at&&before!=text.npos?before+1:0;
                const auto after=text.find_first_of(L"\r\n",last);
                last=after==text.npos?text.size():after;
            }
            if(spans.size()==4096){limited=true;return spans;}
            spans.push_back({first,last-first,rule.color});p=last;q=0;
        }
    }
    return spans;
}
std::string monitor_colored_rtf(std::wstring_view text,const std::vector<RichColorSpan>& spans,int font_points) {
    if(text.size()>2*1024*1024||font_points<8||font_points>32)throw std::runtime_error("Invalid colored display bounds.");
    using Style=std::pair<COLORREF,COLORREF>;
    std::vector<Style> palette{{RGB(0,255,0),RGB(0,0,0)}};
    std::vector<unsigned char> styles(text.size(),0);
    for(const auto& span:spans){
        if(span.start>=text.size()||!span.count)continue;
        const Style style{span.foreground,span.background};auto found=std::find(palette.begin(),palette.end(),style);
        if(found==palette.end()){if(palette.size()==256)throw std::runtime_error("Too many display colors.");palette.push_back(style);found=palette.end()-1;}
        std::fill_n(styles.begin()+span.start,std::min(span.count,text.size()-span.start),static_cast<unsigned char>(found-palette.begin()));
    }
    std::string result="{\\rtf1\\ansi\\deff0\\uc1{\\fonttbl{\\f0\\fmodern Consolas;}}{\\colortbl ;";
    for(const auto& style:palette)for(const auto color:{style.first,style.second})result+="\\red"+std::to_string(GetRValue(color))+"\\green"+std::to_string(GetGValue(color))+"\\blue"+std::to_string(GetBValue(color))+";";
    result+="}\\f0\\fs"+std::to_string(font_points*2)+" ";
    unsigned previous=256;
    for(size_t i=0;i<text.size();++i){
        const unsigned style=styles[i];if(style!=previous){result+="\\cf"+std::to_string(style*2+1)+"\\highlight"+std::to_string(style*2+2)+" ";previous=style;}
        const auto c=text[i];
        if(c==L'\r')result+="\\par ";else if(c==L'\n')result+="\\line ";else if(c==L'\t')result+="\\tab ";
        else if(c==L'\\'||c==L'{'||c==L'}'){result+='\\';result+=static_cast<char>(c);}
        else if(c>=32&&c<127)result+=static_cast<char>(c);
        else result+="\\u"+std::to_string(c<=32767?static_cast<int>(c):static_cast<int>(c)-65536)+"?";
    }
    return result+"}";
}
std::wstring inspect_monitor_bytes(std::string_view bytes,uint64_t offset,uint64_t first,size_t count,const std::wstring& header,const std::wstring& hint) {
    if(!count||count>4096||first<offset||first-offset>bytes.size()||count>bytes.size()-static_cast<size_t>(first-offset))
        throw std::runtime_error("Select 1..4096 bytes inside the retained snapshot.");
    bytes=bytes.substr(static_cast<size_t>(first-offset),count);std::wstring result=header+L"\r\n";
    static const wchar_t digits[]=L"0123456789ABCDEF";
    for(size_t p=0;p<bytes.size();p+=16){wchar_t label[32]{};swprintf_s(label,L"%016llX  ",static_cast<unsigned long long>(first+p));result+=label;std::wstring ascii;
        for(size_t n=0;n<16;++n){if(p+n<bytes.size()){const auto b=static_cast<unsigned char>(bytes[p+n]);result+=digits[b>>4];result+=digits[b&15];result+=L' ';ascii+=b>=32&&b<127?static_cast<wchar_t>(b):L'.';}else result+=L"   ";}result+=L" |"+ascii+L"|\r\n";}
    size_t nul=0,cr=0,lf=0,crcrlf=0;std::wstring controls;unsigned shown=0;
    for(size_t i=0;i<bytes.size();++i){const auto b=static_cast<unsigned char>(bytes[i]);nul+=b==0;cr+=b==13;lf+=b==10;if(i+2<bytes.size()&&bytes.substr(i,3)=="\r\r\n")++crcrlf;
        if(shown<128&&(b<32||b==127)){wchar_t label[48]{};swprintf_s(label,L"+%llu:",static_cast<unsigned long long>(i));controls+=label;controls+=b==0?L"<NUL> ":b==13?L"<CR> ":b==10?L"<LF> ":b==9?L"<TAB> ":b==27?L"<ESC> ":L"<CTRL> ";++shown;}}
    result+=L"\r\nNUL="+std::to_wstring(nul)+L" CR="+std::to_wstring(cr)+L" LF="+std::to_wstring(lf)+L" CR CR LF="+std::to_wstring(crcrlf)+L"\r\n"+controls+L"\r\n"+hint;return result;
}
std::wstring monitor_hex_text(std::wstring_view value){std::wstring out;static const wchar_t hex[]=L"0123456789ABCDEF";for(const auto c:value)for(int shift=12;shift>=0;shift-=4)out+=hex[(static_cast<unsigned>(c)>>shift)&15];return out;}
bool monitor_unhex_text(std::wstring_view value,std::wstring& out){if(value.size()%4||value.size()>65536)return false;std::wstring staged;for(size_t p=0;p<value.size();p+=4){unsigned c=0;for(size_t i=0;i<4;++i){const auto h=value[p+i];const auto n=h>=L'0'&&h<=L'9'?h-L'0':h>=L'A'&&h<=L'F'?h-L'A'+10:16;if(n==16)return false;c=c*16+n;}if(!c)return false;staged+=static_cast<wchar_t>(c);}out=std::move(staged);return true;}
std::wstring encode_recent_resources(const std::vector<RecentResource>& rows){if(rows.size()>20)throw std::runtime_error("Recent list exceeds 20 entries.");std::wstring out=L"v1";for(const auto& r:rows){validate_resource(r);out+=L"|"+std::to_wstring(static_cast<unsigned>(r.kind))+L","+monitor_hex_text(r.name)+L","+monitor_hex_text(r.path)+L","+monitor_hex_text(r.related);}if(out.size()>30000)throw std::runtime_error("Recent list is full; remove an entry first.");return out;}
std::vector<RecentResource> decode_recent_resources(std::wstring_view text){if(text.empty())return {};if(text.substr(0,2)!=L"v1"||text.size()>30000)throw std::runtime_error("Invalid recent list.");std::vector<RecentResource> rows;for(size_t p=2;p<text.size();){if(text[p++]!=L'|')throw std::runtime_error("Invalid recent list.");auto end=text.find(L'|',p);if(end==text.npos)end=text.size();auto row=text.substr(p,end-p);p=end;const auto a=row.find(L','),b=a==row.npos?row.npos:row.find(L',',a+1),c=b==row.npos?row.npos:row.find(L',',b+1);if(a!=1||b==row.npos||row[0]<L'0'||row[0]>L'3')throw std::runtime_error("Invalid recent entry.");RecentResource r;r.kind=static_cast<ResourceKind>(row[0]-L'0');if(!monitor_unhex_text(row.substr(a+1,b-a-1),r.name)||!monitor_unhex_text(row.substr(b+1,c==row.npos?row.npos:c-b-1),r.path)||(c!=row.npos&&!monitor_unhex_text(row.substr(c+1),r.related)))throw std::runtime_error("Invalid recent encoding.");validate_resource(r);rows.push_back(std::move(r));if(rows.size()>20)throw std::runtime_error("Recent list exceeds 20 entries.");}return rows;}
void remember_resource(std::vector<RecentResource>& rows,RecentResource r){validate_resource(r);auto staged=rows;for(const auto& old:staged)if(old.kind==r.kind&&_wcsicmp(old.path.c_str(),r.path.c_str())==0){r.name=old.name;if(r.related.empty())r.related=old.related;break;}staged.erase(std::remove_if(staged.begin(),staged.end(),[&](const auto& old){return old.kind==r.kind&&_wcsicmp(old.path.c_str(),r.path.c_str())==0;}),staged.end());staged.insert(staged.begin(),std::move(r));if(staged.size()>20)staged.resize(20);for(;;){size_t size=2;for(const auto& item:staged)size+=5+4*(item.name.size()+item.path.size()+item.related.size());if(size<=30000)break;staged.pop_back();}encode_recent_resources(staged);rows=std::move(staged);}
bool MonitorEvents::add(MonitorEvent value){value.source=value.source.substr(0,64);value.kind=value.kind.substr(0,64);value.detail=value.detail.substr(0,1024);for(auto it=rows_.rbegin();it!=rows_.rend();++it)if(value.tick>=it->tick&&value.tick-it->tick<=5000&&it->source==value.source&&it->kind==value.kind&&it->detail==value.detail){it->tick=value.tick;it->unix_ms=value.unix_ms;if(it->count<1000000000)++it->count;++revision_;return false;}if(rows_.size()==200)rows_.erase(rows_.begin());value.sequence=++next_sequence_;rows_.push_back(std::move(value));++revision_;return true;}
