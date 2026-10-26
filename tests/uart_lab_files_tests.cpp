#include "recording_tools.h"
#include "finite_script.h"
#include "uart_files.h"
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <utility>
namespace {
void check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);}
void reject_links(const std::filesystem::path& path){std::filesystem::path part;for(const auto& component:path){part/=component;const auto a=GetFileAttributesW(part.c_str());if(a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Fixture reparse point rejected.");}}
class Fixture {
public:
    Fixture(){const auto project=std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path()).lexically_normal();root=project/L"build"/L"verify"/(L"lab-core-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));reject_links(root);std::filesystem::create_directories(root.parent_path());if(!std::filesystem::create_directory(root))throw std::runtime_error("Fixture must be fresh.");}
    ~Fixture(){try{cleanup();}catch(const std::exception& e){std::cerr<<"CLEANUP FAIL: "<<e.what()<<'\n';}}
    std::filesystem::path path(const wchar_t* name){const std::filesystem::path leaf(name);if(leaf.has_parent_path())throw std::runtime_error("Fixture leaf required.");const auto out=root/leaf;if(std::find(paths.begin(),paths.end(),out)==paths.end())paths.push_back(out);return out;}
    void write(const wchar_t* name,const std::string& text){const auto p=path(name);uart_write_file(p,text,false,cancel);}
    void cleanup(){reject_links(root);for(const auto& p:paths){reject_links(p);if(std::filesystem::exists(p)&&!std::filesystem::remove(p))throw std::runtime_error("Cannot remove exact owned fixture file.");}if(std::filesystem::exists(root)&&!std::filesystem::remove(root))throw std::runtime_error("Unknown files remain; fixture preserved.");}
    std::filesystem::path root;std::atomic<bool> cancel{false};
private:std::vector<std::filesystem::path> paths;
};
class FakeTransport:public SerialTransport {
public:
    bool is_connected()const override{return connected;}
    bool connect(const std::wstring&,int,int,int,int,std::wstring*)override{return connected=true;}
    void disconnect()override{connected=false;}
    int consume_rx_overflow()override{return 0;}
    std::vector<std::string> read_lines(std::wstring*,std::string* raw)override{std::lock_guard<std::mutex> lock(mutex);if(raw)*raw=std::exchange(rx,{});return {};}
    size_t write_bytes(std::string_view bytes,std::wstring*)override{std::lock_guard<std::mutex> lock(mutex);++writes;if(reply)rx="OK\nrpm:201\n";return partial?bytes.size()-1:bytes.size();}
    bool connected=true,partial=false,reply=true;size_t writes=0;
private:std::mutex mutex;std::string rx;
};
}
void lab_files_tests() {
    Fixture f;f.write(L"input.bin","ABCD");f.write(L"input.idx","SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,0,1000,connect\r\n0,2,1000,RX\r\n2,2,1010,RX\r\n");
    const auto source=f.path(L"input.bin");auto health=inspect_recording(source,f.cancel);check(health.complete&&health.valid_bytes==4&&health.prefix->records.size()==2,"complete recording health");
    const auto output=f.path(L"slice.bin");f.path(L"slice.idx");export_recording_range(*health.prefix,1,2,output,f.cancel);
    {auto replay=load_replay(output,100,f.cancel);check(replay->total_bytes==2&&replay->records[0].unix_ms==1010&&replay->read_record(0,f.cancel)=="CD","exported pair replays exact bytes and receipt time");}
    f.write(L"collision.idx","USER SENTINEL");const auto collision=f.path(L"collision.bin");bool failed=false;
    try{export_recording_range(*health.prefix,0,2,collision,f.cancel);}catch(...){failed=true;}
    check(failed&&!std::filesystem::exists(collision)&&uart_read_file(f.path(L"collision.idx"),100,f.cancel)=="USER SENTINEL","existing index preserved and owned raw rolled back");
    f.write(L"damaged.bin","ABCDZ");f.write(L"damaged.idx","SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,2,1000,RX\r\n2,2,1010,RX\r\n4,20,1020,RX\r\n");
    {const auto broken=inspect_recording(f.path(L"damaged.bin"),f.cancel);check(!broken.complete&&broken.valid_bytes==4,"truncated tail isolates valid prefix");f.path(L"rescued.idx");export_recording_range(*broken.prefix,0,broken.prefix->records.size(),f.path(L"rescued.bin"),f.cancel);check(uart_read_file(f.path(L"rescued.bin"),100,f.cancel)=="ABCD","salvage contains valid prefix only");}
    const std::atomic<bool> cancelled{true};failed=false;try{export_recording_range(*health.prefix,0,2,f.path(L"cancel.bin"),cancelled);}catch(...){failed=true;}check(failed&&!std::filesystem::exists(f.path(L"cancel.bin")),"cancelled export leaves no payload");
    f.write(L"plain.txt","x:1\nx:2\n");{auto replay=load_replay(f.path(L"plain.txt"),100,f.cancel);export_recording_range(*replay,0,replay->records.size(),f.path(L"synthetic.bin"),f.cancel);check(!std::filesystem::exists(f.path(L"synthetic.idx")),"synthetic source never fabricates receipt index");}
    // Readers must release before fixture cleanup; these input handles are owned by health.
    const auto count=health.prefix.use_count();check(count==1,"no leaked background prefix owners");
    health.prefix.reset();f.cleanup();check(!std::filesystem::exists(f.root),"exact fixture cleanup checked");
    std::cout<<"PASS: range export/health/salvage/cancel/no-overwrite\n";
}
void lab_runner_tests() {
    ScriptStep step;step.command.name="Read";step.command.body="Q";step.expect="OK";step.numeric_key="rpm";step.minimum=200;step.maximum=210;
    std::atomic<bool> cancel{false};FakeTransport good;auto result=execute_finite_script(good,{step,step},cancel,1000);
    check(result.passed&&good.writes==2&&result.csv.find("201")!=result.csv.npos,"finite fake runner reuses numeric/raw assertions");
    FakeTransport partial;partial.partial=true;result=execute_finite_script(partial,{step,step},cancel,1000);check(!result.passed&&partial.writes==1,"partial write stops runner without retry");
    FakeTransport silent;silent.reply=false;result=execute_finite_script(silent,{step},cancel,20);check(!result.passed&&silent.writes==1,"finite duration stops waiting without retry");
    FakeTransport stopped;cancel=true;result=execute_finite_script(stopped,{step},cancel,1000);check(!result.passed&&stopped.writes==0,"cancel before TX never sends");
    StepScript timing;std::string error;step.numeric_key.clear();step.expect.clear();check(timing.start({step},1,100,error),"timing fixture starts");std::string bytes;timing.next(100,bytes);timing.complete(true,120);check(timing.matched_timings().empty(),"driver-only timing excluded from reply statistics");
    std::cout<<"PASS: finite fake runner/partial/cancel/driver-only timing\n";
}
