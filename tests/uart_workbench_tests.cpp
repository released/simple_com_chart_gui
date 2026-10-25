#include "workbench_session.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <deque>

namespace {
int checks=0;
void check(bool value,const char* name){if(!value)throw std::runtime_error(name);++checks;}
struct Device {
    std::mutex mutex;
    std::deque<std::string> rx;
    std::vector<std::string> writes;
    std::atomic<bool> hold{false},fail{false},removed{false};
    std::atomic<int> live{0},opens{0};
    void feed(std::string bytes){std::lock_guard<std::mutex> lock(mutex);rx.push_back(std::move(bytes));}
};
class Fake final:public SerialTransport {
public:
    explicit Fake(std::shared_ptr<Device> device):device_(std::move(device)){++device_->live;}
    ~Fake()override{--device_->live;}
    bool is_connected()const override{return connected_;}
    bool connect(const std::wstring&,int,int,int,int,std::wstring* error)override{
        ++device_->opens;while(device_->hold)Sleep(1);if(device_->fail){if(error)*error=L"synthetic busy";return false;}connected_=true;return true;
    }
    void disconnect()override{connected_=false;}
    std::vector<std::string> read_lines(std::wstring* error,std::string* bytes)override{
        if(device_->removed.exchange(false)){if(error)*error=L"synthetic device removal";connected_=false;return {};}
        std::lock_guard<std::mutex> lock(device_->mutex);if(!device_->rx.empty()){*bytes=std::move(device_->rx.front());device_->rx.pop_front();}return {};
    }
    int consume_rx_overflow()override{return 0;}
    size_t write_bytes(std::string_view bytes,std::wstring*)override{std::lock_guard<std::mutex> lock(device_->mutex);device_->writes.emplace_back(bytes);return bytes.size();}
    void cancel_io()override{}
private:
    std::shared_ptr<Device> device_;std::atomic<bool> connected_{false};
};
SerialSettings settings(const wchar_t* port,const wchar_t* id){SerialSettings s;s.port={port,L"synthetic",id,L"synthetic-interface"};return s;}
std::string received(const TrafficStore& traffic){std::string out;for(const auto& r:traffic.snapshot().records)if(!r.tx)out+=r.bytes;return out;}
std::string file_bytes(const std::filesystem::path& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
}
int main(int argc,char** argv){try{
    const auto root=std::filesystem::path(argc>1?argv[1]:"workbench-fixture")/std::to_string(GetTickCount64());std::filesystem::create_directories(root);
    TrafficStore a(512,16),b(512,16);std::thread ta([&]{for(int n=0;n<1000;++n)a.append("A",n);}),tb([&]{for(int n=0;n<1000;++n)b.append("B",n);});ta.join();tb.join();
    check(a.snapshot().rx==1000&&b.snapshot().rx==1000,"concurrent byte counters independent");check(received(a).find('B')==std::string::npos&&received(b).find('A')==std::string::npos,"port streams never merge");
    check(a.snapshot().records.size()<=16&&a.snapshot().expired>0,"bounded history reports expiry");const auto btext=received(b);const auto old=a.snapshot().revision;a.clear();check(a.snapshot().revision!=old&&received(a).empty()&&received(b)==btext,"clear one port preserves the other");
    a.append(std::string(1000,'A'),7);check(received(a).size()==512,"oversized read remains bounded");check(a.snapshot().expired==488,"oversized history expiry counts bytes");
    check(serial_ports_conflict(settings(L"COM10",L"id").port,settings(L"com10",L"other").port),"same COM cannot be owned twice");
    check(serial_ports_conflict(settings(L"COM10",L"id").port,settings(L"COM11",L"ID").port),"same device after renumbering cannot be owned twice");
    check(!serial_ports_conflict(settings(L"COM10",L"a").port,settings(L"COM11",L"b").port),"distinct devices allowed");
    WorkbenchSession one,two;const auto d1=std::make_shared<Device>(),d2=std::make_shared<Device>();one.set_factory([d1]{return std::make_unique<Fake>(d1);});two.set_factory([d2]{return std::make_unique<Fake>(d2);});
    const auto s1=settings(L"COM71",L"A"),s2=settings(L"COM72",L"B");const std::vector<SerialPortInfo> ports={s1.port,s2.port};
    const auto pump=[&](auto predicate,unsigned timeout=3500){const auto until=GetTickCount64()+timeout;do{one.poll(ports);two.poll(ports);if(predicate())return true;Sleep(1);}while(GetTickCount64()<until);return false;};
    std::wstring error;check(one.connect(s1,error)&&two.connect(s2,error),"two opens start independently");check(pump([&]{return one.connected()&&two.connected();}),"both asynchronous connections complete");
    d1->feed("AAA\r\n");d2->feed("BBB\r\n");check(pump([&]{return received(one.traffic)=="AAA\r\n"&&received(two.traffic)=="BBB\r\n";}),"simultaneous distinct receive streams");
    check(one.send("a-command",error)&&two.send("b-command",error),"independent bounded TX queues");
    check(pump([&]{return one.traffic.snapshot().tx==9&&two.traffic.snapshot().tx==9;}),"accepted TX captured by correct port");
    RecordingOptions r1;r1.base=root/L"portA";RecordingOptions r2;r2.base=root/L"portB";
    check(one.recorder.start(r1)&&two.recorder.start(r2),"separate raw recording destinations");
    const auto rx1=one.traffic.snapshot().rx,rx2=two.traffic.snapshot().rx;d1->feed("record-A");d2->feed("record-B");check(pump([&]{return one.traffic.snapshot().rx>=rx1+8&&two.traffic.snapshot().rx>=rx2+8;}),"both recording producers receive");
    one.recorder.stop();two.recorder.stop();check(one.recorder.wait_idle(3000)&&two.recorder.wait_idle(3000),"both recorders drain without sharing files");
    check(file_bytes(root/L"portA_1.bin")=="record-A"&&file_bytes(root/L"portB_1.bin")=="record-B","recorded raw streams do not cross ports");
    one.disconnect();check(pump([&]{return one.idle();})&&two.connected(),"disconnect A preserves B connection");
    d2->feed("still-B");check(pump([&]{return received(two.traffic).find("still-B")!=std::string::npos;}),"B keeps receiving after A disconnect");
    d1->fail=true;check(one.connect(s1,error),"failed open runs off UI thread");check(pump([&]{return one.idle();})&&two.connected(),"failed A open cannot disconnect B");check(one.take_error().find(L"synthetic busy")!=std::wstring::npos,"manual open error remains actionable");
    d1->fail=false;d1->hold=true;check(one.connect(s1,error),"late open starts");Sleep(10);one.disconnect();check(!one.idle()&&two.connected(),"cancel keeps pending ownership until late open completes");d1->hold=false;check(pump([&]{return one.idle();}),"late success discarded after cancellation");check(d1->live==0,"late transport released");
    const auto beforeRecovery=received(two.traffic);d2->removed=true;
    check(pump([&]{return !two.connected();}),"unexpected B removal detected");
    check(pump([&]{return two.connected();},4000)&&received(two.traffic)==beforeRecovery,"same-device B recovery preserves history");
    two.disconnect();check(pump([&]{return two.idle();}),"independent shutdown drains workers");
    std::cout<<"PASS: "<<checks<<" workbench isolation/lifecycle checks; no real COM hardware.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
