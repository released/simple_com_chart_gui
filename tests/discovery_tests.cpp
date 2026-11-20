#include "message_lens.h"
#include "masked_pattern.h"
#include "numeric_watch.h"
#include "byte_comparison.h"
#include "linked_cursor.h"
#include "recording_library.h"
#include "uart_offline.h"
#include "background_job.h"
#include "discovery_geometry.h"
#include <iostream>
#include <stdexcept>
#include <numeric>
namespace {
void check(bool value,const char* label){if(!value)throw std::runtime_error(label);std::cout<<"PASS: "<<label<<'\n';}
template<class F>bool rejected(F f){try{f();}catch(...){return true;}return false;}
void lens_tests(){
    MessageBuffer bytes;bytes.append("boot\r\nok\r\nERR one\r\nignore\r\ntail\r\n",0,1000);const auto original=bytes.bytes();
    MessageLens lens;std::string error;check(lens.configure({"ERR","ignore",false,1,1},error),"lens valid staged config");
    MessageDisplayOptions options;const auto view=lens.update(bytes,options);
    check(view.formatted.text==L"ok\r\nERR one\r\n"&&view.rows.size()==2&&view.rows[0].offset==6&&view.rows[1].offset==10,"include context and exclusion priority with exact byte spans");
    check(bytes.bytes()==original&&bytes.receipts().front().unix_ms==1000,"lens leaves raw bytes and receipt metadata immutable");
    check(view.formatted.colors.size()==1&&view.formatted.colors[0].start==0&&view.formatted.colors[0].count==view.formatted.text.size()&&view.formatted.colors[0].color==0x0000ff00,"plain lens uses one equivalent native foreground span across logical rows");
    check(!lens.configure({"(","",true,0,0},error)&&lens.config().include=="ERR","invalid regex retains active lens");
    bytes.append("ERR two\r",0,1050);const auto partial=lens.update(bytes,options);check(partial.formatted.text.find(L"tail\r\nERR two\r\n")!=std::wstring::npos,"new match reveals its previously hidden preceding context");
    bytes.append("\nafter\r\n",0,1100);const auto complete=lens.update(bytes,options);check(complete.formatted.text.find(L"ERR two\r\n\r\n")==std::wstring::npos&&complete.rows.back().offset+complete.rows.back().count==bytes.bytes().size(),"split CRLF is one row and new after-context retains exact ending");
    const auto rich=lens.update(bytes,options,true);check(rich.formatted.text.find(L'\n')==std::wstring::npos,"RichEdit row mapping uses native CR offsets");
    MessageBuffer utf;utf.append("ERR \xe4",0,1500);MessageLens utf_lens;utf_lens.configure({},error);options.encoding=MessageEncoding::Utf8;utf_lens.update(utf,options);utf.append("\xb8\xad\n",0,1550);check(utf_lens.update(utf,options).formatted.text==L"ERR \u4e2d\r\n","UTF-8 split suffix is reassembled before lens display");
    MessageBuffer retained(12);retained.append("old\nERR\n",0,1000);lens.configure({"ERR","",false,0,0},error);lens.update(retained,{});retained.append("new\nERR\n",0,1100);const auto evicted=lens.update(retained,{});check(evicted.rows.size()==2&&evicted.rows.front().offset>=retained.offset(),"retention eviction invalidates cached offsets instead of guessing old rows");
    MessageBuffer long_row;long_row.append(std::string(65537,'x')+"\nERR\n");lens.configure({},error);check(lens.update(long_row,{}).limited==1,"oversized lens rows are visibly work-limited without matching truncated prefixes");
    const auto partial_search=search_retained_messages(long_row,{},L"ERR",false,true);
    check(partial_search.matches==1&&partial_search.skipped_rows==1&&!partial_search.limited&&partial_search.rows.front().offset==65538,"search reports skipped oversized rows separately from the4096-hit limit with exact visible source offset");
    const auto no_hits=search_retained_messages(long_row,{},L"absent",false,true);
    check(no_hits.matches==0&&no_hits.skipped_rows==1&&long_row.bytes()==std::string(65537,'x')+"\nERR\n","zero-result search still exposes incomplete coverage and preserves raw input");
    const auto search=search_retained_messages(bytes,{},L"ERR",false,true);check(search.matches==2&&search.rows.size()==2&&std::accumulate(search.buckets.begin(),search.buckets.end(),size_t{})==2,"search list and 64 frequency buckets independently count actual matches");
    check(search.rows[0].offset==10&&search.rows[0].receipt==1000&&search.rows[1].receipt==1050,"search preserves per-source row offsets and batch receipt times");
    MessageBuffer edges;for(const int64_t stamp:{1000,1010,1639,1640})edges.append("hit\n",0,stamp);const auto buckets=search_retained_messages(edges,{},L"hit",false,true);
    check(buckets.begin_ms==1000&&buckets.end_ms==1640&&buckets.buckets[0]==1&&buckets.buckets[1]==1&&buckets.buckets[63]==2,"frequency uses 64 equal intervals and includes the final receipt in the last bucket");
    MessageBuffer many;many.append(std::string(4100,'a'));const auto limited=search_retained_messages(many,{},L"a",false,true);check(limited.matches==4096&&limited.limited,"search result cap is explicit, not claimed as a complete scan");
    check(!search_retained_messages(bytes,{},L"(",true,true).error.empty(),"invalid search reports an error without a result oracle");
    for(const bool rich:{false,true}){MessageBuffer clocked;MessageLens incremental,whole;incremental.configure({},error);whole.configure({},error);MessageDisplayOptions clock;clock.clock=MessageClock::Delta;clock.suppress_nul=true;clocked.append("A\n",0,1000);clocked.append(std::string("\0\n",2),0,1100);clocked.append(std::string("\0\n",2),0,1200);incremental.update(clocked,clock,rich);clocked.append("B\n",0,1300);const auto a=incremental.update(clocked,clock,rich).formatted,b=whole.update(clocked,clock,rich).formatted;check(a.text==b.text&&a.text.find(L"[dt +0.300s] B")!=a.text.npos,"delta clock is chunk-invariant after consecutive hidden rows in plain and RichEdit formats");check(a.colors.size()==1&&a.colors.front().count==a.text.size(),"clock prefixes and visible rows share one complete foreground span");}
}
void pattern_tests(){
    MaskedPattern p;std::wstring error;check(parse_masked_pattern(L"AA 55 ?? ?? 0D 0A",p,error),"masked pattern parses whole-byte placeholders");
    const std::string input("\x00\xaa\x55\x01\x02\r\n",7);check(find_masked(input,p)==1,"masked search ignores only selected variable bytes");
    check(!parse_masked_pattern(L"A?",p,error)&&p.value.size()==6,"invalid nibble wildcard preserves previous parsed pattern");
    check(!parse_masked_pattern(L"0x AA",p,error)&&!parse_masked_pattern(L"",p,error),"mask rejects TX-style prefix and empty pattern");
    std::wstring limit;for(unsigned i=0;i<257;++i)limit+=L"?? ";check(!parse_masked_pattern(limit,p,error),"masked pattern size is bounded");
    parse_masked_pattern(L"?? ??",p,error);check(find_masked("A",p)==std::string::npos&&find_masked("AB",p)==0,"wildcard matching honors full pattern length");
}
void watch_tests(){
    std::vector<NumericWatchRule> rules;std::string error;check(parse_numeric_watches("A;temp;0;10;1;0.1;1",rules,error),"finite watch configuration accepted");
    NumericWatches engine;engine.arm(rules);check(engine.feed(0,{{"temp",11}},7,1000).empty(),"watch dwell does not alarm on first excursion");
    const auto alarm=engine.feed(0,{{"temp",11}},7,1100);check(alarm.size()==1&&alarm[0].state=="Above maximum","continuous excursion alarms once after dwell");
    check(engine.feed(0,{{"temp",9.5}},7,1200).empty(),"hysteresis avoids oscillating alarm at threshold");
    engine.feed(0,{{"temp",8.5}},7,1300);check(engine.feed(0,{{"temp",8.5}},7,1400).front().state=="Returned to range","clear threshold and dwell are applied without changing numeric data");
    check(engine.poll(2501).size()==1&&engine.poll(2600).empty(),"stale data emits one unavailable event, never a zero sample");
    check(engine.feed(0,{{"temp",3}},7,2700).size()==1,"new valid data explicitly resumes after stale gap");
    check(engine.feed(0,{{"temp",11}},8,2800).front().state.find("Disarmed")==0&&engine.feed(0,{{"temp",11}},8,3000).empty(),"generation change disarms and cannot silently resume");
    engine.arm(rules,4000);check(engine.poll(5100).size()==1&&engine.poll(5200).empty(),"watch with no first sample reports unavailable after its configured deadline");
    engine.arm(rules);check(engine.disconnect(0).size()==1&&engine.disconnect(0).empty(),"disconnect disarms even without any received sample");
    engine.arm(rules);check(engine.gap(1).empty()&&engine.armed(),"an unrelated source gap preserves the active watch");check(engine.gap(0).size()==1&&!engine.armed()&&engine.gap(0).empty()&&engine.feed(0,{{"temp",11}},7,1000).empty(),"observed input loss disarms once and never resumes without explicit arm");
    check(!parse_numeric_watches("A;temp;0;10;6;0;2",rules,error)&&!parse_numeric_watches("A;temp;0;10;0;2;1",rules,error)&&!parse_numeric_watches("A;temp;nan;10;0;0;1",rules,error),"watch boundaries reject invalid hysteresis stale period and nonfinite values");
    check(!parse_numeric_watches("A;t;0;10;0;0;1\nA;t;0;10;0;0;1",rules,error),"duplicate source/key rules are rejected");
}
void cursor_diff_tests(){
    ByteComparisonRequest request;
    check(parse_byte_comparison_request({L"1",L"2",L"10",L"0"},2,request)&&request.frame_a==0&&request.frame_b==1&&request.offset_a==10&&request.offset_b==0,"comparison validates once into exact decimal frame and byte indexes");
    for(const auto text:{L"1e1",L"10.0",L"10x",L"-1",L"+1",L" 1",L"",L"4097",L"999999999999999999999"})
        check(!parse_byte_comparison_request({L"1",L"2",text,L"0"},2,request)&&request.offset_a==10,"invalid comparison spelling preserves the last typed request");
    check(parse_byte_comparison_request({L"1",L"2",L"4096",L"000"},2,request)&&request.offset_a==4096&&request.offset_b==0,"comparison decimal offset upper boundary is exact");
    check(!parse_byte_comparison_request({L"0",L"1",L"0",L"0"},2,request)&&!parse_byte_comparison_request({L"1",L"3",L"0",L"0"},2,request),"comparison frame numbers remain inside the retained snapshot");
    CaptureStore capture;capture.append("",{{"x",2},{"y",4}},10,0,0,3,0);capture.append("",{{"x",8}},10.05,0,0,3,0);const auto snapshot=capture.snapshot();
    check(linked_cursor_value(snapshot,"y",10.04,.1)==4,"linked cursor uses nearest valid sparse field within explicit age");
    check(!linked_cursor_value(snapshot,"y",10.4,.1)&&!linked_cursor_value(snapshot,"missing",10,.1),"old and missing cursor samples stay n/a");
    capture.append("",{{"x",11}},10.06,0,0,4,0);check(!linked_cursor_value(capture.snapshot(),"y",10.06,.1),"linked cursor never borrows a field across a generation boundary");
    capture.append("",{{"B:y",77}},10.061,0,0,2,1);check(!linked_cursor_value(capture.snapshot(),"x",10.061,.1),"linked cursor refuses incompatible source pairing");
    check(linked_cursor_value(capture.snapshot(),"x",10.061,.1,capture.snapshot().rows[2].get())==11,"explicit cursor-row identity resolves coincident A/B source clocks");
    CaptureStore retained(2);retained.append("",{{"x",1}},1);const auto old=retained.rows().front();retained.append("",{{"x",2}},1.01);retained.append("",{{"x",3}},1.02);check(!linked_cursor_value(retained.snapshot(),"x",1,.1,old.get()),"expired cursor context cannot borrow a nearby retained value even while another snapshot holds it");
    const auto diff=compare_bytes("ABCD","AXC");check(diff.size()==2&&diff[0].offset==1&&diff[0].a=='B'&&diff[0].b=='X'&&diff[1].b==-1,"byte diff reports exact change and unequal trailing length");
    check(compare_bytes("zABC","ABC",1,0).empty(),"byte comparison requires explicit alignment and leaves originals unchanged");
    check(rejected([]{compare_bytes(std::string(4097,'a'),"a");})&&rejected([]{compare_bytes("A","B",2,0);}),"comparison byte and offset bounds enforced");
}
std::thread fail_thread(std::function<void()>){throw std::runtime_error("Injected launch failure");}
void job_geometry_tests(){
    BackgroundJob<int> job;bool invoked=false;
    check(!job.start([&](const auto&){invoked=true;return 7;},fail_thread)&&!job.busy()&&!invoked,"thread launch failure is observable without executing work or leaving a busy job");
    int value=0;std::wstring error;
    check(!job.take(value,error),"failed launch cannot be mistaken for a completed successful job");
    check(job.start([](const auto&){return 7;}),"same job can retry normally after launch failure");
    const auto deadline=GetTickCount64()+3000;bool taken=false;
    while(!(taken=job.take(value,error))&&GetTickCount64()<deadline)Sleep(1);
    check(taken&&value==7&&error.empty()&&!job.busy(),"successful retry is collected before lifetime teardown");
    for(const RECT work:std::vector<RECT>{{0,0,800,600},{-1600,20,-800,740},{1000,-100,2920,980},{0,0,1,1}}){
        const auto bounds=fit_discovery_bounds({60,60,1060,740},work);
        check(bounds.left>=work.left&&bounds.top>=work.top&&bounds.right<=work.right&&bounds.bottom<=work.bottom&&bounds.right>bounds.left&&bounds.bottom>bounds.top,"initial and DPI-style rectangles fit small and negative-coordinate work areas");
    }
}
void file_tests(){
    wchar_t work[32768]{};const auto n=GetEnvironmentVariableW(L"SCC_TEST_WORK",work,32768);if(!n||n>=32768)throw std::runtime_error("Registered SCC_TEST_WORK required");const std::filesystem::path owned(work);if(!std::filesystem::is_regular_file(owned.parent_path()/L".owner.json"))throw std::runtime_error("Central ownership marker required");const auto root=owned/(L"discovery-core-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));if(!std::filesystem::create_directory(root))throw std::runtime_error("Fresh fixture required");
    std::atomic<bool> cancel{false};const auto a=root/L"discovery-a.bin",b=root/L"discovery-b.bin",catalog=root/L"discovery.scclibrary";uart_write_file(a,std::string("\xaa\x55",2),false,cancel);uart_write_file(b,std::string("\x01\x02\r\n",4),false,cancel);
    {ReplayData data;data.files.push_back(std::make_shared<ReplayFile>(a));data.files.push_back(std::make_shared<ReplayFile>(b));data.records.push_back({0,2,0,1000,0,0});data.records.push_back({0,4,.05,1050,1,0});data.records.finish();MaskedPattern pattern;std::wstring error;parse_masked_pattern(L"AA 55 ?? ?? 0D 0A",pattern,error);check(replay_find_masked(data,0,pattern,cancel)==0,"masked replay match spans actual source-file boundaries");cancel=true;check(rejected([&]{replay_find_masked(data,0,pattern,cancel);}),"masked source reads honor cancellation");cancel=false;}
    RecordingReference entry;entry.resource={ResourceKind::Recording,L"board A",a.wstring(),L""};entry.tags=L"regression,\u4e2d\u6587";
    const auto encoded=encode_recording_library({entry});check(decode_recording_library(encoded).front().tags==entry.tags,"private library round-trips Unicode tags without delimiter collisions");
    check(rejected([&]{decode_recording_library(encoded+"bad\n");})&&rejected([&]{encode_recording_library({entry,entry});}),"malformed and duplicate catalog entries are rejected");
    RecordingLibrary original;save_recording_reference(catalog,original,entry,cancel);auto loaded=read_recording_library(catalog,{},cancel);check(loaded.entries.size()==1&&loaded.entries[0].bytes==2&&loaded.entries[0].version.empty(),"library reads only referenced file metadata and never guesses missing version");
    uart_write_file(catalog,encoded+"broken",true,cancel);check(rejected([&]{save_recording_reference(catalog,loaded,entry,cancel);}),"externally changed library cannot be overwritten by stale review");
    check(uart_read_file(a,10,cancel)==std::string("\xaa\x55",2)&&uart_read_file(b,10,cancel)==std::string("\x01\x02\r\n",4),"search catalog and comparison retain exact raw files");
}
}
int main(){try{lens_tests();pattern_tests();watch_tests();cursor_diff_tests();job_geometry_tests();file_tests();std::cout<<"PASS: discovery core boundaries and preservation\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
