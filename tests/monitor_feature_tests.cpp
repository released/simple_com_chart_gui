#include "monitor_features.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
int main(){int failed=0;const auto check=[&](const char* name,bool result){std::cout<<(result?"PASS: ":"FAIL: ")<<name<<'\n';failed+=!result;};
    const auto rejects=[](auto work){try{work();return false;}catch(const std::exception&){return true;}};
    std::vector<KeywordRule> rules;std::wstring error;
    check("literal case/scope rules parse",parse_keyword_rules(L"red,line,0,error\r\nyellow,word,1,WARN",rules,error)&&rules.size()==2);
    const auto previous=rules;check("bad Apply retains rules",!parse_keyword_rules(L"bad,word,1,abc",rules,error)&&rules.size()==previous.size()&&rules[0].text==previous[0].text);
    bool limited=false;auto spans=keyword_spans(L"ok\rERROR here\rWARN next",rules,0,limited);check("case insensitive whole line and literal word",spans.size()==2&&spans[0].start==3&&spans[0].count==10&&spans[1].count==4&&!limited);
    check("tail restyle does not revisit old lines",keyword_spans(L"ERROR old\rWARN new",rules,10,limited).size()==1);
    std::wstring many;for(int i=0;i<17;++i)many+=L"red,word,1,x\n";check("rule count bounded",!parse_keyword_rules(many,rules,error));
    check("rule text bounded",!parse_keyword_rules(L"red,word,1,"+std::wstring(129,L'x'),rules,error));
    check("control bytes rejected in keyword",!parse_keyword_rules(std::wstring(L"red,word,1,x")+wchar_t(0),rules,error));
    check("regex characters remain literal",parse_keyword_rules(L"red,word,1,[A].*",rules,error)&&keyword_spans(L"[A].* AAA",rules,0,limited).size()==1);
    parse_keyword_rules(L"red,word,1,x",rules,error);spans=keyword_spans(std::wstring(5000,L'x'),rules,0,limited);check("span count bounded with visible limit",spans.size()==4096&&limited);
    const auto unicode=std::wstring(L"板子,\"A\"|測試");std::wstring decoded;check("INI-safe Unicode round trip",monitor_unhex_text(monitor_hex_text(unicode),decoded)&&decoded==unicode);
    check("truncated hex rejected",!monitor_unhex_text(L"004",decoded));check("embedded NUL rejected",!monitor_unhex_text(L"0000",decoded));
    const auto colored=monitor_colored_rtf(L"{\\rtf1}\r\t\u4e2d\U0001F642",{{0,7,RGB(255,0,0),RGB(0,0,0)}},10);
    check("RTF escapes raw syntax, newlines and Unicode code units",colored.find("\\{\\\\rtf1\\}")!=colored.npos&&colored.find("\\par \\tab \\u20013?")!=colored.npos&&colored.find("\\u-10179?\\u-8638?")!=colored.npos);
    check("RTF rejects oversized display input",rejects([&]{monitor_colored_rtf(std::wstring(2*1024*1024+1,L'a'),{},10);}));
    const std::string bytes("A\r\r\n\0Z",6);const auto dump=inspect_monitor_bytes(bytes,100,100,6);
    check("exact CR CR LF and NUL inspection",dump.find(L"41 0D 0D 0A 00 5A")!=dump.npos&&dump.find(L"|A....Z|")!=dump.npos&&dump.find(L"CR CR LF=1")!=dump.npos&&dump.find(L"<NUL>")!=dump.npos);
    check("absolute offsets used",dump.find(L"0000000000000064")!=dump.npos);
    check("past end rejected",rejects([&]{inspect_monitor_bytes(bytes,100,105,2);}));check("expired offset rejected",rejects([&]{inspect_monitor_bytes(bytes,100,99,1);}));check("zero count rejected",rejects([&]{inspect_monitor_bytes(bytes,100,100,0);}));
    check("4096 full inspection fits preview",inspect_monitor_bytes(std::string(4096,'\0'),0,0,4096).size()<32768);
    std::vector<RecentResource> recent;remember_resource(recent,{ResourceKind::Recording,unicode,L"D:\\test\\source_1.bin",L"D:\\chosen\\source_1.log"});auto restored=decode_recent_resources(encode_recent_resources(recent));
    check("private recent references round trip",restored.size()==1&&restored[0].name==unicode&&restored[0].related==L"D:\\chosen\\source_1.log");
    remember_resource(recent,{ResourceKind::Recording,L"renamed",L"d:\\TEST\\SOURCE_1.bin"});check("same path is deduplicated and preserves custom label",recent.size()==1&&recent[0].name==unicode&&!recent[0].related.empty());
    for(int i=0;i<30;++i)remember_resource(recent,{ResourceKind::Profile,L"p",L"D:\\profiles\\p"+std::to_wstring(i)+L".sccprofile"});check("recent history capped at 20",recent.size()==20);
    check("relative resource rejected",rejects([&]{remember_resource(recent,{ResourceKind::Profile,L"p",L"..\\outside"});}));check("malformed recent input rejected",rejects([&]{decode_recent_resources(L"v1|9,0041,0042");}));
    for(const auto path:{L"\\\\.\\COM7",L"C:\\safe\\COM1.sccprofile",L"C:\\safe\\nul.log",L"C:\\safe\\p.sccprofile:stream",L"\\\\?\\GLOBALROOT\\Device\\test"})check("device namespaces, reserved names and streams rejected",rejects([&]{remember_resource(recent,{ResourceKind::Profile,L"p",path});}));
    MonitorEvents events;check("new event added",events.add({1000,20,L"A",L"failed",L"disk"}));check("repeat coalesces",!events.add({1500,21,L"A",L"failed",L"disk"})&&events.rows()[0].count==2);
    check("other port remains separate",events.add({1600,22,L"B",L"failed",L"disk"})&&events.rows().size()==2);check("late repeat becomes new event",events.add({7000,23,L"A",L"failed",L"disk"}));
    for(int i=0;i<250;++i)events.add({10000+static_cast<uint64_t>(i)*6000,i,L"A",L"failed",std::to_wstring(i)});check("event history bounded",events.rows().size()==200);events.clear();check("events clear independently",events.rows().empty()&&recent.size()==20);
    parse_keyword_rules(L"red,word,1,"+std::wstring(127,L'a')+L"b",rules,error);rules.resize(16,rules.front());
    const auto began=std::chrono::steady_clock::now();const auto adversarial=keyword_spans(std::wstring(262144,L'a'),rules,0,limited);
    const auto micros=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-began).count();
    std::cout<<"adversarial_keyword_us="<<micros<<" limited="<<limited<<'\n';check("adversarial literal input never fabricates a match",adversarial.empty());
    return failed?1:0;
}
