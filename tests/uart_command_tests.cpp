#include "command_panel.h"
#include "step_script.h"
#include "tx_pacing.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
namespace {size_t checks=0;void check(bool ok,const char* what){++checks;if(!ok)throw std::runtime_error(what);}}
int main(){try{std::string bytes,error;PanelCommand c;c.group="Control";c.name="Set value";c.body="set {value}";c.value=20;c.ending=TxEnding::CRLF;
    check(panel_payload(c,21,bytes,error)&&bytes=="set 21\r\n","text substitution/ending");
    c.hex=true;c.body="AA {value:u16be}";c.maximum=65535;c.value=1000;c.ending=TxEnding::None;c.checksum=FrameChecksum::Modbus16;
    check(panel_payload(c,1000,bytes,error)&&bytes.substr(0,3)==std::string("\xAA\x03\xE8",3)&&bytes.size()==5,"typed binary substitution/checksum");
    check(frame_checksum(std::string_view(bytes).substr(0,3),FrameChecksum::Modbus16)==static_cast<uint32_t>(static_cast<unsigned char>(bytes[3])|(static_cast<unsigned char>(bytes[4])<<8)),"checksum ordering");
    check(!panel_payload(c,65536,bytes,error)&&!panel_payload(c,1.5,bytes,error),"binary range/fraction rejection");
    c.body="{value:i16le}";c.minimum=-32768;c.value=-2;c.checksum=FrameChecksum::None;
    check(panel_payload(c,-2,bytes,error)&&bytes==std::string("\xFE\xFF",2),"signed binary substitution");
    c.body="{value:u7}";check(!panel_payload(c,1,bytes,error),"unknown placeholder rejection");
    c.hex=false;c.body="{\"a\":{value}}\r\nline,\"quoted\"";c.value=2;c.group="量測";
    const auto encoded=encode_panel({c});std::vector<PanelCommand> decoded;
    check(decode_panel(encoded,decoded,error)&&decoded.size()==1&&decoded[0].body==c.body&&decoded[0].group==c.group,"Unicode/quote/newline round trip");
    check(!decode_panel("SCC_COMMANDS_V2\r\n",decoded,error)&&decoded.size()==1,"unknown version atomic rejection");
    check(!decode_panel(encoded+"broken\r\n",decoded,error)&&decoded[0].body==c.body,"partial import atomic rejection");
    check(!validate_panel({c,c},error),"duplicate group/name rejection");
    std::vector<std::vector<std::string>> rows;check(!uart_csv_rows("\"unterminated",rows,error)&&!uart_csv_rows("\"a\"junk",rows,error),"malformed CSV rejected");
    check(!uart_csv_rows(std::string(129*1024,'a'),rows,error),"definition bounded");
    ScriptStep first;first.command=c;first.command.name="first";first.command.body="Q";first.expect="ABABAC";first.delay_ms=20;first.timeout_ms=100;
    ScriptStep second=first;second.command.name="second";second.command.body="R";second.expect.clear();second.delay_ms=10;
    const auto definition=encode_script({first,second});std::vector<ScriptStep> steps;check(decode_script(definition,steps,error)&&steps.size()==2&&steps[0].expect=="ABABAC","step definition round trip");
    check(!decode_script(definition+"bad",steps,error)&&steps.size()==2,"script atomic rejection");
    StepScript script;check(script.start(steps,9,100,error),"script start");script.receive("ABABAC",9,101);check(!script.next(119,bytes)&&script.next(120,bytes)&&bytes=="Q","delay cannot consume earlier RX");
    script.receive("ABAB",9,121);script.receive("ABAC",9,122);check(script.active()&&script.index()==0,"response during TX waits for write completion");script.complete(true,130);check(script.index()==1&&!script.next(139,bytes)&&script.next(140,bytes)&&bytes=="R","overlapping split expect and next step delay");script.complete(true,141);check(!script.active()&&script.results().size()==2&&script.status().find("COMPLETED")!=std::string::npos,"ordered per-step results");
    script.start({first},10,200,error);script.next(220,bytes);script.complete(true,221);script.receive("ABABAC",10,321);check(!script.active()&&script.status().find("TIMEOUT")!=std::string::npos,"deadline wins over late response");
    script.start({first},10,400,error);script.next(420,bytes);script.complete(false,421);check(!script.active()&&!script.next(1000,bytes),"partial TX no retry");
    script.start({first},10,500,error);script.next(520,bytes);script.receive("A",11,521);check(!script.active(),"connection generation cancellation");
    script.start({first},10,600,error);script.tick(3600600);check(!script.active(),"finite execution cap");
    std::string actual;unsigned calls=0;std::wstring detail;const auto writer=[&](std::string_view part,std::wstring&){actual+=part;++calls;return part.size();};
    check(paced_write("A\r\nB\n",{0,1},writer,[]{return true;},detail)==5&&actual=="A\r\nB\n"&&calls==2&&detail.empty(),"line pacing preserves CRLF bytes");
    actual.clear();calls=0;check(paced_write("ABC",{1,0},writer,[]{return true;},detail)==3&&calls==3&&actual=="ABC","byte pacing segments without changes");
    calls=0;check(paced_write("ABC",{},[&](std::string_view,std::wstring&){++calls;return size_t(1);},[]{return true;},detail)==1&&calls==1&&!detail.empty(),"partial pacing never retries");
    calls=0;check(paced_write("ABC",{100,0},writer,[&]{return calls==0;},detail)==1&&calls==1&&!detail.empty(),"cancel interrupts pacing");
    SerialOperationQueue queue;queue.begin();queue.pacing({2,3});SerialOperation operation;operation.bytes="Q";uint64_t id=0;check(queue.submit(operation,id,detail)&&queue.busy()&&queue.take(operation)&&queue.busy()&&operation.pacing.byte_ms==2,"queue holds pacing snapshot and in-flight busy");queue.complete({operation,1,true,false,{}});check(!queue.busy(),"completion clears in-flight reservation");
    operation.pacing_override=true;operation.pacing={};check(queue.submit(operation,id,detail)&&queue.take(operation)&&operation.pacing.byte_ms==0,"protocol can explicitly bypass user pacing");queue.cancel();queue.complete({operation,0,false,true,{}});check(!queue.busy(),"cancelled in-flight completion drains");
    c.body.clear();c.ending=TxEnding::CRLF;c.checksum=FrameChecksum::None;check(panel_payload(c,c.value,bytes,error)&&bytes=="\r\n","empty command with explicit ending sends Enter");
    queue.begin();queue.pacing({0,60000});operation={};operation.bytes=std::string(61,'\n');check(!queue.submit(operation,id,detail)&&detail.find(L"one hour")!=std::wstring::npos,"excessive per-operation pacing rejected before queueing");
    queue.pacing({});operation={};operation.bytes="Q";bool completion=false;operation.completion=[&](const SerialOperationResult& r){completion=r.cancelled;};check(queue.submit(operation,id,detail)&&queue.take(operation)&&queue.cancel_pending(id),"in-flight pacing cancellation token is addressable");queue.complete({operation,1,true,false,{}});check(completion&&queue.results().back().cancelled,"worker completion callback observes cancelled outcome");
    std::vector<PanelCommand> large(64,c);for(size_t i=0;i<large.size();++i){large[i].name=std::to_string(i);large[i].body=std::string(4000,'x');large[i].ending=TxEnding::None;}check(!validate_panel(large,error),"oversized library rejected before replacing saved definitions");
    ScriptStep numeric;numeric.command.group="Test";numeric.command.name="Range";numeric.command.body="Q";numeric.numeric_key="rpm";numeric.minimum=200;numeric.maximum=210;
    StepScript range;check(range.start({numeric},42,0,error)&&range.next(0,bytes),"numeric script start");range.receive("rpm:20",42,1);range.complete(true,2);check(range.active(),"numeric split reply waits");range.receive("1\n",42,3);check(!range.active()&&range.results()[0].find("PASS")==0,"numeric range match");check(range.csv().find("rpm")!=std::string::npos&&range.csv().find("201")!=std::string::npos,"structured numeric report");
    check(range.start({numeric},42,10,error)&&range.next(10,bytes),"numeric second explicit run");range.complete(true,11);range.receive("rpm:999\n",42,12);check(!range.active()&&range.results()[0].find("FAIL")==0,"out-of-range fails immediately");
    std::vector<ScriptStep> numeric_copy;check(decode_script(encode_script({numeric}),numeric_copy,error)&&numeric_copy[0].numeric_key=="rpm"&&numeric_copy[0].maximum==210,"v2 numeric step round trip");
    std::cout<<"PASS: "<<checks<<" command/script/pacing checks; no hardware.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
