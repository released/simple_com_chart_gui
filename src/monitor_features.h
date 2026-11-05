#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct KeywordRule { std::wstring text; COLORREF color=0; bool line=false,match_case=true; };
struct KeywordSpan { size_t start=0,count=0; COLORREF color=0; };
bool parse_keyword_rules(std::wstring_view,std::vector<KeywordRule>&,std::wstring&);
std::vector<KeywordSpan> keyword_spans(std::wstring_view,const std::vector<KeywordRule>&,size_t begin,bool& limited);
struct RichColorSpan {size_t start=0,count=0;COLORREF foreground=0,background=0;};
std::string monitor_colored_rtf(std::wstring_view,const std::vector<RichColorSpan>&,int font_points);
std::wstring inspect_monitor_bytes(std::string_view,uint64_t retained_offset,uint64_t first,size_t count,
    const std::wstring& header=L"Offset | HEX | ASCII",const std::wstring& hint=L"Control annotations: first 128 only; HEX/ASCII covers the complete selected range.");
std::wstring monitor_hex_text(std::wstring_view);
bool monitor_unhex_text(std::wstring_view,std::wstring&);

enum class ResourceKind { Recording,Profile,View,Session };
struct RecentResource { ResourceKind kind=ResourceKind::Recording; std::wstring name,path,related; };
std::wstring encode_recent_resources(const std::vector<RecentResource>&);
std::vector<RecentResource> decode_recent_resources(std::wstring_view);
void remember_resource(std::vector<RecentResource>&,RecentResource);
struct MonitorEvent { uint64_t tick=0; int64_t unix_ms=0; std::wstring source,kind,detail; unsigned count=1;uint64_t sequence=0; };
class MonitorEvents {
public:
    bool add(MonitorEvent);
    void clear() { rows_.clear(); ++revision_; }
    const std::vector<MonitorEvent>& rows()const {return rows_;}
    uint64_t revision()const {return revision_;}
private:
    std::vector<MonitorEvent> rows_;
    uint64_t revision_=0,next_sequence_=0;
};
