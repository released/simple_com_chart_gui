#include <iostream>
#include <cmath>
#include <functional>
#include "session_fixture.h"
#include "workflow_views.h"
#include "workflow_notes.h"
#include "workflow_profile_diff.h"
#include "workflow_report.h"
#include "workflow_waterfall.h"
#include "packet_builder.h"
#include "lab_metrics.h"
namespace {int failures=0,checks=0;void check(bool ok,const char* name){++checks;std::cout<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';if(!ok)++failures;}bool rejects(std::function<void()> work){try{work();return false;}catch(const std::exception&){return true;}}}
int main(){try{SessionFixture f;
    ViewPreset view;view.checked={"rpm","temp"};view.columns={"rpm"};view.charts[1]="rpm";view.axes[0]={12.5,-10,1200,true};view.axes[2].window=60;const auto encoded=encode_view(view);const auto decoded=decode_view(encoded);
    check(decoded.checked==view.checked&&decoded.charts==view.charts&&decoded.axes[0].fixed&&decoded.axes[0].low==-10&&decoded.axes[0].window==12.5,"view preserves raw keys, none slots, fractional window and independent axes");
    check(encode_view(decode_view(encode_view(ViewPreset{})))==encode_view(ViewPreset{}),"empty view stays none without auto-selection");
    check(rejects([]{decode_view("SimpleComChartView=2\n");}),"view rejects future/unknown format without settings writes");
    check(rejects([&]{decode_view(encoded+"P=\n");})&&rejects([&]{decode_view(encoded.substr(0,encoded.find("A=")));}),"view rejects excess or incomplete sections");
    check(rejects([&]{auto v=view;v.checked.push_back("rpm");encode_view(v);}),"view rejects duplicate checked IDs");
    check(rejects([&]{auto v=view;v.axes[0].window=0;encode_view(v);})&&rejects([&]{auto v=view;v.axes[0].high=v.axes[0].low;encode_view(v);}),"view rejects invalid ranges");
    check(rejects([&]{auto v=view;v.axes[0].window=0.5;encode_view(v);})&&rejects([&]{auto v=view;v.axes[0].high=1e101;encode_view(v);}),"view validates renderer limits before any partial UI application");
    check(rejects([]{decode_view(std::string(65537,'x'));})&&rejects([&]{auto v=view;v.charts[0]=std::string(1,'\xff');encode_view(v);}),"view bounds bytes and UTF-8 before application");
    auto rows=packet_parameters("address,1,0,255\ntarget,1000,-2000,2000\n");check(rows.size()==2&&packet_parameters(encode_packet_parameters(rows))==rows,"parameter grid preserves validated CSV row values");
    check(rejects([]{packet_parameters("x,1,0,2\nx,2,0,3");})&&rejects([]{packet_parameters("sequence,1,0,2");}),"parameter grid rejects duplicates and reserved placeholders");
    check(rejects([]{packet_parameters("x,3,0,2");})&&rejects([]{packet_parameters("x,nan,0,2");})&&rejects([]{packet_parameters("x,1,2,0");}),"parameters reject out-of-range, nonfinite and reversed values");
    check(rejects([]{encode_packet_parameters({{"x","1\ny,2,0,3","0","4"}});}),"grid cell cannot inject extra CSV rows");
    check(rejects([]{std::vector<PacketParameterRow> x(33,{"x","1","0","2"});encode_packet_parameters(x);}),"parameter count bounded to 32");
    PacketDefinition packet;packet.command.hex=true;packet.command.body="A5 {address:u8} {target:u16le}";packet.parameters=encode_packet_parameters(rows);std::string wire,error;check(packet_payload(packet,wire,error)&&wire==std::string("\xA5\x01\xE8\x03",4),"existing packet preview uses the same parameter validation and encoding");
    f.write(L"record.bin","abc");auto notes=bind_notes(f.file(L"record.bin"),f.cancel);check(notes.source.bytes==3&&notes.source.sha256=="BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD","note source SHA256 matches an independent standard vector");
    notes.notes.push_back({1,2,"Observed response <not a script>\nline two"});auto copied=decode_notes(encode_notes(notes));verify_notes(copied,f.file(L"record.bin"),f.cancel);check(copied.notes[0].offset==1&&copied.notes[0].text==notes.notes[0].text,"notes preserve exact byte span and multiline text");
    check(rejects([&]{auto n=notes;n.notes[0].offset=4;validate_notes(n);})&&rejects([&]{auto n=notes;n.notes[0].count=3;validate_notes(n);}),"notes reject offset/count beyond source without overflow");
    check(rejects([&]{auto n=notes;n.source.leaf=L"../record.bin";validate_notes(n);})&&rejects([&]{auto n=notes;n.notes[0].text=std::string(1,'\xff');validate_notes(n);}),"notes reject traversal and malformed UTF-8");
    check(rejects([&]{auto n=notes;n.source.leaf=L"COM1.bin";validate_notes(n);})&&rejects([&]{auto n=notes;n.source.leaf=L"record.bin. ";validate_notes(n);}),"notes reject reserved device names and Windows filename aliases before opening files");
    check(rejects([&]{auto n=notes;n.notes.resize(257,n.notes[0]);encode_notes(n);})&&rejects([]{decode_notes(std::string(1024*1024+1,'x'));}),"note rows and serialized size are bounded");
    auto altered=notes;altered.source.sha256[0]='0';check(rejects([&]{verify_notes(altered,f.file(L"record.bin"),f.cancel);}),"same filename and size with a wrong hash never verifies");
    f.write(L"other.bin","abc");check(rejects([&]{verify_notes(notes,f.file(L"other.bin"),f.cancel);}),"same bytes with different recorded filename require deliberate rebinding");
    f.cancel=true;check(rejects([&]{bind_notes(f.file(L"record.bin"),f.cancel);}),"source hashing obeys cancellation");f.cancel=false;
    f.write(L"notes.sccnotes",encode_notes(notes));const auto saved=uart_read_file(f.file(L"notes.sccnotes"),1024*1024,f.cancel);check(rejects([&]{uart_write_file(f.file(L"notes.sccnotes"),"bad",false,f.cancel);})&&uart_read_file(f.file(L"notes.sccnotes"),1024*1024,f.cancel)==saved,"atomic no-overwrite failure preserves existing notes");
    CaptureRow text;text.raw="a:201,b:2\r\n";check(capture_case_input(text)==text.raw,"A parser case retains exact text and CR/LF");text.raw="B text: "+text.raw;text.parser_prefix=8;check(capture_case_input(text)=="a:201,b:2\r\n","B parser case removes only its producer-recorded display prefix");text.parser_prefix=UINT16_MAX;check(rejects([&]{capture_case_input(text);}),"binary display descriptions cannot masquerade as received parser input");
    const auto caseRow=capture_case_csv("a:201,b:2","a:201,b:2","");ExtractionConfig config;const auto cases=check_parser_cases(config,"Input,Expected,Seconds\n"+caseRow,0);check(cases.passed==1&&cases.failed==0,"selected-row case quotes comma fields and runs offline");
    check(check_parser_cases(config,"Input,Expected,Seconds\n"+capture_case_csv("a:201","a:200",""),0).failed==1,"reviewed expected values can detect a mismatch, never forced to match");
    check(rejects([]{capture_case_csv(std::string(4097,'x'),"","");}),"captured case input stays within existing 4096-byte limit");
    CaptureStore store;store.append("x:1",{{"x",1}},10,0,3,1);store.append("x:3",{{"x",3}},11,3,3,1);store.append("missing",{},12,6,7,1);store.append("x:9",{{"x",9}},13,13,3,2);
    const auto report=workflow_html(store.snapshot(),{"x"},"<script>alert(1)</script>",0,3,f.cancel);check(report.find("&lt;script&gt;")!=report.npos&&report.find("<script>")==report.npos&&report.find("default-src 'none'")!=report.npos,"HTML escapes untrusted title and contains no executable script or remote resources");
    check(report.find("<td>3</td>")!=report.npos&&report.find("<svg")!=report.npos,"report summaries and static plots use selected retained samples");
    check(report.find("<path")!=report.rfind("<path"),"missing values and new generations split report paths");
    check(rejects([&]{workflow_html(store.snapshot(),{"missing"},"title",0,3,f.cancel);})&&rejects([&]{workflow_html(store.snapshot(),{"x","x"},"title",0,3,f.cancel);}),"report rejects unknown/duplicate selections without output");
    check(rejects([&]{workflow_html(store.snapshot(),{},"title",0,3,f.cancel);})&&rejects([&]{workflow_html(store.snapshot(),{"x"},"title",3,1,f.cancel);}),"report never silently selects all or accepts reversed intervals");
    f.cancel=true;check(rejects([&]{workflow_html(store.snapshot(),{"x"},"title",0,3,f.cancel);}),"report generation observes cancellation");f.cancel=false;
    UartProfile a,b;a.serial.port.device=L"COM3";b=a;b.serial.baud=9600;b.charts[2]="rpm";const auto diff=profile_differences(a,b);check(diff.size()==2&&diff[0][0]=="Baud"&&diff[0][1]=="115200"&&diff[0][2]=="9600","profile review compares actual typed settings rather than full private INI");check(profile_differences(a,a).empty(),"identical supported profiles produce no invented differences");
    auto workspace=a;workspace.workspace=true;workspace.workspace_settings=read_workspace_settings(f.file(L"workspace.ini").wstring());auto legacy=workspace;legacy.workspace=false;legacy.workspace_settings.clear();check(profile_differences(workspace,legacy).empty(),"legacy profile review leaves unsupported workspace settings unchanged, matching the writer");
    check(report.find("<circle")!=report.npos,"isolated report sample renders a visible point rather than an empty path");
    WaterfallFrame frame;frame.seconds=1;frame.generation=1;frame.spectrum.sample_rate=100;frame.spectrum.bins={{0,0},{25,0.1},{50,1}};WaterfallHistory history;check(history.push(frame,2)&&std::abs(history.rows()[0][1]+20)<1e-5,"waterfall uses amplitude dB with finite zero floor");check(!history.push(frame,2)&&history.rows().size()==1,"duplicate source timestamps do not fabricate new history");frame.seconds=2;history.push(frame,2);frame.seconds=3;history.push(frame,2);check(history.rows().size()==2&&history.newest()==3,"waterfall evicts old rows at configured bound");frame.generation=2;frame.seconds=4;history.push(frame,2);check(history.rows().size()==1,"reconnect generation clears waterfall continuity");frame.seconds=10;history.push(frame,2);check(history.rows().size()==1,"source gaps clear waterfall continuity");frame.seconds=11;frame.spectrum.sample_rate=200;history.push(frame,2);check(history.rows().size()==1&&history.rate()==200,"sample-rate changes cannot mix waterfall frequency axes");check(rejects([&]{history.push(frame,257);}),"waterfall history cannot exceed 256 rows");frame.spectrum.bins[0].second=-1;check(rejects([&]{history.push(frame,2);}),"invalid spectral amplitudes cannot enter history");
    std::cout<<"Checks: "<<checks<<"; failures: "<<failures<<'\n';f.cleanup();return failures?1:0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
