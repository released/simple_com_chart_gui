#include "session_fixture.h"
#include "uart_recorder.h"
#include "session_metadata.h"
#include <iostream>
#include <stdexcept>
void session_check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
template<class F> bool rejected(F work){try{work();}catch(...){return true;}return false;}
void recording_limit_tests() {
    RecordingLimits l{100,2,50};
    session_check(recording_limit_reason(l,60,40,1000,100).empty(),"budget accepts exact total/free boundary");
    session_check(!recording_limit_reason(l,60,41,1000,100).empty()&&!recording_limit_reason(l,0,0,2000,100).empty()&&!recording_limit_reason(l,0,1,0,50).empty(),"independent output/time/free-space limits");
    session_check(decode_recording_limits(encode_recording_limits(l)).free_bytes==50&&decode_recording_limits("").bytes==0,"versioned limits and legacy defaults");
    session_check(rejected([]{decode_recording_limits("v1:1:2:3:4");})&&rejected([]{decode_recording_limits("v1:-1:2:3");}),"invalid limits cannot disable a guard silently");
    SessionFixture f;const auto base=f.root/L"capture";
    for(const auto& ext:{L".bin",L".log",L".idx"})f.file(std::wstring(L"capture_1")+ext);
    UartRecorder writer;RecordingOptions options;options.base=base;options.limits.bytes=180;
    session_check(writer.start(options),"bounded recorder starts");writer.append(std::string(200,'X'),1000);
    session_check(writer.wait_idle(3000),"output limit stops without manual stop");
    auto status=writer.status();uint64_t bytes=0;for(const auto& ext:{L".bin",L".log",L".idx"})bytes+=std::filesystem::file_size(f.root/(std::wstring(L"capture_1")+ext));
    session_check(status.limit_reached&&status.output_bytes==bytes&&bytes<=180&&status.bytes==0,"all three actual outputs counted; rejected chunk never partially indexed");
    for(const auto& ext:{L".bin",L".log",L".idx"})f.file(std::wstring(L"timer_1")+ext);
    options.base=f.root/L"timer";options.limits={0,1,0};session_check(writer.start(options),"same recorder can start a separate finite session");
    session_check(writer.wait_idle(3000)&&writer.status().limit_reached,"time limit stops even while RX is quiet");
    for(const auto& ext:{L".bin",L".log",L".idx"})f.file(std::wstring(L"normal_1")+ext);
    options.base=f.root/L"normal";options.limits={};session_check(writer.start(options),"legacy unlimited recording remains available");writer.append("a:1\n",1010);writer.append("CMD",1020,true);writer.stop();
    session_check(writer.wait_idle(3000)&&!writer.status().limit_reached&&writer.status().bytes==4&&writer.status().tx_bytes==3,"manual stop preserves RX/TX accounting");
    session_check(uart_read_file(f.root/L"normal_1.bin",100,f.cancel)=="a:1\n","raw recording remains RX only");
    for(const auto& ext:{L".bin",L".log",L".idx"})f.file(std::wstring(L"meta_1")+ext);f.file(L"meta.sccmeta");
    options.base=f.root/L"meta";options.metadata=encode_capture_metadata(UartProfile{},"1.0.1.029","A");session_check(writer.start(options),"metadata writer starts");writer.append("v:201\n",1030);writer.event("NUMERIC CONFIG CHANGED: test",1040);writer.event("NUMERIC CONFIG CHANGED: again",1050);writer.stop();session_check(writer.wait_idle(3000),"metadata writer drains");
    bytes=std::filesystem::file_size(f.root/L"meta.sccmeta");for(const auto& ext:{L".bin",L".log",L".idx"})bytes+=std::filesystem::file_size(f.root/(std::wstring(L"meta_1")+ext));
    const auto metadata=uart_read_file(f.root/L"meta.sccmeta",1100*1024,f.cancel);session_check(writer.status().output_bytes==bytes&&decode_capture_metadata(metadata).changed&&metadata.size()==options.metadata.size()+11,"actual sidecar/change marker counted exactly once across setting changes");f.cleanup();
}
void session_algorithms_tests();
void session_file_tests();void transaction_tests();
int main(){try{recording_limit_tests();session_algorithms_tests();session_file_tests();transaction_tests();return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
