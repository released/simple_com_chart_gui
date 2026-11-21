#include "message_text_patch.h"
#include "monitor_analysis.h"
#include "json_numeric.h"
#include "watch_alarms.h"
#include "uart_operations.h"
#include "message_format.h"
#include <iostream>
#include <windows.h>
#include <numeric>
#include <limits>
#include <stdexcept>
namespace {
int failed=0,count=0;
void check(bool ok,const char* name){++count;std::cout<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';if(!ok)++failed;}
template<class F>bool rejects(F f){try{f();return false;}catch(const std::exception&){return true;}}
TrafficRecord frame(uint64_t seq,int64_t time,std::string bytes,uint64_t generation=1,bool tx=false){TrafficRecord r;r.sequence=seq;r.receipt_ms=time;r.bytes=std::move(bytes);r.generation=generation;r.tx=tx;return r;}
}
int main(){
    std::atomic<bool> cancel{false};
    const auto apply_patch=[](std::wstring before,const std::wstring& after){const auto patch=message_text_patch(before,after);if(!patch)return false;before.erase(0,patch->erase_head);before+=after.substr(patch->shared);return before==after;};
    const std::wstring retained=std::wstring(40,L'a')+L"unique retained tail";
    check(apply_patch(L"head"+retained,retained+L"new tail"),"retained text patch reconstructs exact target");
    check(apply_patch(retained,retained),"identical retained text requires no edit");
    check(apply_patch(std::wstring(2000,L'a')+L"tail",std::wstring(1500,L'a')+L"tail"),"hysteresis shrink starts at the minimum feasible head position");
    check(!message_text_patch(std::wstring(80,L'a'),std::wstring(80,L'b')),"unmatched retained text requires full update");
    check(!message_text_patch(std::wstring(65537,L'x')+retained,retained+L"tail"),"head search is bounded before native editing");
    check(!message_text_patch(std::wstring(1024*1024+1,L'a'),retained),"large text uses the existing full-update fallback");
    const std::wstring split=std::wstring(31,L'a')+L"\xD83D";
    check(!message_text_patch(L"head"+split,split+L"\xDE00tail"),"text patch never splits a UTF-16 surrogate pair");
    const std::wstring unicode=L"\xD83D\xDE00"+retained;
    check(apply_patch(L"head"+unicode,unicode+L"new"),"whole surrogate pairs survive retained edits");
    check(!message_text_patch(L"head"+retained,retained.substr(0,retained.size()-1)+L"changed"),"partial prefix match cannot authorize a stale suffix");
    PresentationCadence cadence;check(cadence.due(1),"default cadence preserves immediate legacy dispatch");cadence.interval=100;cadence.finish(1000,1007);check(!cadence.due(1099)&&cadence.due(1100)&&cadence.cost==7,"cadence uses render start, exposes measured cost");check(cadence.due(999),"backward clock does not stall presentation");cadence.finish(1100,1100,0.125);check(cadence.cost==0.125&&cadence.maximum==7&&cadence.last_end==1100,"fractional update timing and sample identity survive same tick");
    std::vector<double> hundred;for(unsigned i=1;i<=100;++i)hundred.push_back(i);auto d=monitor_distribution(hundred,cancel);
    check(d.count==100&&d.p50==50&&d.p95==95&&d.p99==99,"independent nearest-rank quantiles");check(std::accumulate(d.bins.begin(),d.bins.end(),size_t{})==100&&d.bins.front()==4&&d.bins.back()==4,"32 bins include both ends without losing samples");
    d=monitor_distribution({7,7,7},cancel);check(d.bins[0]==3&&d.p99==7,"constant distribution");check(!monitor_distribution({},cancel).count,"empty distribution remains unavailable");
    const auto maximum=std::numeric_limits<double>::max();d=monitor_distribution({-maximum,0,maximum},cancel);check(d.bins[0]==1&&d.bins[16]==1&&d.bins[31]==1,"opposite finite double extremes do not overflow bucket arithmetic");
    check(rejects([&]{monitor_distribution({std::numeric_limits<double>::quiet_NaN()},cancel);}),"nonfinite distribution rejected");check(rejects([&]{monitor_distribution(std::vector<double>(10001,1),cancel);}),"distribution count bounded before sort");cancel=true;check(rejects([&]{monitor_distribution(hundred,cancel);}),"cancelled distribution rejected");cancel=false;
    CaptureStore capture;capture.append("x",{{"x",10}},1,0,1,1,0);capture.append("x",{{"x",20}},2,1,1,1,1);capture.append("x",{{"x",30}},3,2,1,1,0);auto snapshot=capture.snapshot();capture.reset();
    check(monitor_values(snapshot,"x",0,0,2,cancel)==std::vector<double>{10,30},"immutable capture range uses raw sparse source values after source reset");check(monitor_values(snapshot,"x",1,0,2,cancel)==std::vector<double>{20},"source B is independently selected");check(rejects([&]{monitor_values(snapshot,"missing",0,0,2,cancel);}),"expired/missing key is not a fabricated zero");
    TrafficBatch a,b;a.records={frame(1,100,"a"),frame(2,150,"b")};b.records={frame(1,100,"c"),frame(2,125,"d")};auto timeline=monitor_timeline(a,b,cancel);
    check(timeline.size()==4&&timeline[0].source==0&&timeline[1].source==1&&timeline[2].record.receipt_ms==125,"merged receipt timeline has deterministic explicit ties");a.records[0].bytes="changed";check(timeline[0].record.bytes=="a","timeline is immutable after source changes");
    a.records={frame(1,100,"a"),frame(2,150,"b"),frame(3,1000,"c",2),frame(4,1075,"d",2),frame(6,2000,"e",2),frame(7,2001,"f",2,true),frame(8,2002,"g",2)};
    check(monitor_intervals(a,cancel)==std::vector<double>({50,75}),"receipt intervals never bridge generations, skipped sequence or TX");
    a.records={frame(1,100,std::string(512*1024+1,'a'))};check(rejects([&]{monitor_timeline(a,b,cancel);}),"timeline byte budget checked before row copies");
    std::vector<TrafficRecord> frames={frame(1,1,std::string("\0\x55",2)),frame(2,2,std::string("\1\x55",2)),frame(3,3,std::string("\3\x55",2))};auto variation=monitor_variation(frames,0,2);
    check(variation.size()==2&&variation[0].distinct==3&&variation[0].transitions[0]==1&&variation[0].transitions[1]==1&&variation[1].distinct==1,"independent per-byte diversity and adjacent bit transitions");frames[1].generation=2;check(rejects([&]{monitor_variation(frames,0,2);}),"variation rejects mixed generations");frames[1].generation=1;frames[1].tx=true;check(rejects([&]{monitor_variation(frames,0,2);}),"variation rejects mixed directions");frames[1].tx=false;frames[1].bytes="short";check(rejects([&]{monitor_variation(frames,0,2);}),"variation rejects unequal lengths");check(rejects([&]{monitor_variation(frames,2,1);}),"variation rejects reversed range");
    const std::string json="{\"a/b\":{\"~x\":[1,2]},\"comma,quote\\\"\":3,\"ignored\":true}";const auto tree=json_numeric_tree(json);std::vector<size_t> selected;for(size_t i=0;i<tree.size();++i)if(tree[i].numeric)selected.push_back(i);const auto definition=json_tree_selection(tree,selected);const auto paths=json_selections(definition.substr(4));const auto numbers=json_numbers(json,paths);
    check(paths.size()==3&&paths[0].second=="/a~1b/~0x/0"&&numbers.at("value1")==1&&numbers.at("value3")==3,"tree selections round-trip pointer and CSV quote/comma escapes");check(tree[2].parent==1&&tree[3].parent==2,"tree preserves explicit container parent identity");
    check(json_numeric_tree("5").front().numeric&&json_tree_selection(json_numeric_tree("5"),{0})=="ptr:\"value1=\"","numeric JSON root is selectable");check(rejects([&]{json_tree_selection(tree,{});}),"empty tree selection cannot silently select flat JSON");check(rejects([&]{json_tree_selection(tree,{selected[0],selected[0]});}),"duplicate selections rejected");check(rejects([]{json_numeric_tree("{\"x\":{\"a\":1,\"a\":2}}");}),"nested duplicate JSON keys rejected");check(rejects([]{json_numeric_tree(std::string(65537,' '));}),"sample byte cap before parsing");check(rejects([]{json_numeric_tree("  ");}),"whitespace-only JSON rejected safely");check(json_numbers("{\"x\":1,\"nested\":{\"y\":2}}",{}).size()==1,"legacy flat JSON behavior retained");
    check(rejects([]{json_numeric_tree("{\""+std::string(513,'a')+"\":1}");}),"JSON tree path length capped before retention");
    check(rejects([]{std::string input="{\""+std::string(480,'a')+"\":[";for(int i=0;i<3000;++i)input+="0,";input+="0]}";json_numeric_tree(input);}),"small JSON input cannot amplify retained paths beyond 1 MiB");
    SerialOperationQueue queue;queue.begin();check(queue.observation(true),"idle queue enters observation");for(const auto kind:{SerialOperation::Kind::Write,SerialOperation::Kind::Controls,SerialOperation::Kind::Break}){SerialOperation op;op.kind=kind;op.bytes="x";uint64_t id=123;std::wstring error;check(!queue.submit(op,id,error)&&id==123&&!error.empty(),"central observation gate rejects every operation kind");}queue.cancel();queue.begin();check(queue.observation(),"observation survives disconnect and new generation");check(queue.observation(false),"explicit idle exit restores operations");SerialOperation operation;operation.bytes="x";uint64_t id=0;std::wstring error;check(queue.submit(operation,id,error)&&!queue.observation(true),"pending TX prevents observation transition");queue.take(operation);check(!queue.observation(true),"running TX prevents observation transition");queue.complete({operation,1,true,false,{}});check(queue.observation(true),"completed TX permits explicit observation entry");
    std::vector<NumericWatchRule> rules;std::string detail;check(parse_numeric_watches("A;x;0;10;1;0;2",rules,detail)&&rules[0].severity==1,"legacy watch severity defaults to Warning");check(parse_numeric_watches("A;x;0;10;1;0;2;Critical",rules,detail)&&rules[0].severity==2,"optional severity parsed");const auto old=rules;check(!parse_numeric_watches("A;x;0;10;1;0;2;invalid",rules,detail)&&rules[0].severity==old[0].severity,"invalid severity retains prior definitions");NumericWatches watches;watches.arm(rules,100);const auto events=watches.feed(0,{{"x",20}},1,100);check(events.size()==1&&events[0].severity==2,"watch events preserve configured severity");
    WatchAlarms alarms;alarms.observe(events[0],100);const auto alarm=alarms.rows()[0].id;check(alarms.snooze(alarm,100)&&alarms.reminders(60099).empty()&&alarms.reminders(60100).size()==1&&alarms.reminders(60101).empty(),"snooze emits exactly one due reminder");alarms.snooze(alarm,70000);check(alarms.acknowledge(alarm)&&alarms.reminders(130000).empty()&&alarms.rows()[0].active,"acknowledgement leaves condition active and cancels reminder");alarms.observe({0,"x","Returned to range",5},140000);check(!alarms.rows()[0].active&&!alarms.snooze(alarm,150000),"resolved alarms do not snooze");alarms.observe(events[0],160000);check(alarms.rows().size()==2&&!alarms.rows().back().acknowledged,"new episode has independent review state");alarms.observe(watches.gap(0).front(),170000);check(!alarms.rows().back().active&&!watches.armed(),"input gap resolves alarm and disarms watch");for(size_t i=0;i<200;++i)alarms.observe({0,std::to_string(i),"Above maximum",20},i);check(alarms.rows().size()==128&&!alarms.acknowledge(alarm),"alarm history and stale action identities bounded");alarms.resolve_all();check(alarms.reminders(999999).empty(),"stop does not remind or re-arm");
    check(format_message_timestamp(0,0,0,MessageClock::UtcIso)==L"[1970-01-01T00:00:00.000Z] ","UTC format has independent epoch expectation");check(format_message_timestamp(86400123,0,0,MessageClock::UtcIso)==L"[1970-01-02T00:00:00.123Z] ","UTC date rollover and milliseconds");const auto local=format_message_timestamp(0,0,0,MessageClock::LocalIso);check(local.size()==32&&local[11]==L'T'&&local[27]==L':'&&(local[24]==L'+'||local[24]==L'-'),"local ISO includes date and numeric offset");
    check(format_message_timestamp(std::numeric_limits<int64_t>::max(),0,0,MessageClock::UtcIso)==L"[n/a] "&&format_message_timestamp(std::numeric_limits<int64_t>::min(),0,0,MessageClock::LocalIso)==L"[n/a] ","ISO rejects out-of-range timestamps without signed overflow");
    unsigned y=0,month=0,day=0,hour=0,minute=0,second=0,milli=0,offset_hour=0,offset_minute=0;wchar_t sign=0;
    const int scanned=swscanf_s(local.c_str(),L"[%u-%u-%uT%u:%u:%u.%u%lc%u:%u]",&y,&month,&day,&hour,&minute,&second,&milli,&sign,1u,&offset_hour,&offset_minute);
    SYSTEMTIME civil{};civil.wYear=static_cast<WORD>(y);civil.wMonth=static_cast<WORD>(month);civil.wDay=static_cast<WORD>(day);civil.wHour=static_cast<WORD>(hour);civil.wMinute=static_cast<WORD>(minute);civil.wSecond=static_cast<WORD>(second);civil.wMilliseconds=static_cast<WORD>(milli);FILETIME ft{};const bool converted=SystemTimeToFileTime(&civil,&ft)!=FALSE;ULARGE_INTEGER ticks{};ticks.LowPart=ft.dwLowDateTime;ticks.HighPart=ft.dwHighDateTime;
    const auto offset=static_cast<int64_t>(offset_hour*60+offset_minute)*(sign==L'-'?-1:1);
    check(scanned==10&&converted&&static_cast<int64_t>(ticks.QuadPart/10000)-11644473600000LL-offset*60000==0,"independent local ISO date/offset round-trip preserves the receipt epoch");
    std::cout<<"checks="<<count<<" failed="<<failed<<'\n';return failed?1:0;
}
