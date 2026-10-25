#include "block_protocol.h"
#include <condition_variable>
#include <deque>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
namespace {unsigned checks=0;void check(bool ok,const char* what){++checks;if(!ok)throw std::runtime_error(what);}
struct Wire {std::mutex mutex;std::condition_variable cv;std::deque<unsigned char> bytes;int read(unsigned ms){std::unique_lock<std::mutex> lock(mutex);if(!cv.wait_for(lock,std::chrono::milliseconds(ms),[&]{return !bytes.empty();}))return -1;const auto c=bytes.front();bytes.pop_front();return c;}void write(std::string_view data){std::lock_guard<std::mutex> lock(mutex);for(const unsigned char c:data)bytes.push_back(c);cv.notify_all();}};
void roundtrip(BlockMode mode,bool corrupt,bool drop,bool padded=false){Wire toSender,toReceiver;std::atomic<bool> failed{false};std::string senderError,receiverError;unsigned damaged=0,dropped=0;std::vector<std::pair<std::string,std::string>> inputs;std::string all;for(unsigned i=0;i<33001;++i)all.push_back(static_cast<char>(i));inputs.push_back({"one.bin",all});if(mode==BlockMode::Ymodem){inputs.push_back({"two.bin","second file"});inputs.push_back({"empty.bin",""});}std::vector<BlockSource> sources;for(const auto& input:inputs){const auto data=input.second;sources.push_back({input.first,data.size(),[data](uint64_t at,size_t n){return data.substr(static_cast<size_t>(at),n);}});}
    const auto guard=[&]{if(failed)throw std::runtime_error("Peer failed");};BlockLink a{[&](unsigned ms){return toSender.read(ms);},[&](std::string_view bytes){guard();std::string out(bytes);if(corrupt&&!damaged&&out.size()>128&&(out[0]==1||out[0]==2)){out[10]^=1;++damaged;}toReceiver.write(out);},guard,{}};
    BlockLink b{[&](unsigned ms){return toReceiver.read(ms);},[&](std::string_view bytes){guard();if(drop&&!dropped&&bytes==std::string(1,char(6))){++dropped;return;}toSender.write(bytes);},guard,{}};
    std::map<std::string,std::string> outputs;std::string name,current;unsigned commits=0;BlockSink sink{[&](const std::string& n,std::optional<uint64_t>){name=n.empty()?"one.bin":n;current.clear();},[&](std::string_view bytes){current.append(bytes);},[&]{outputs[name]=current;++commits;}};
    const BlockOptions options{mode,100,10};std::thread sender([&]{try{block_send(options,sources,a);}catch(const std::exception& e){senderError=e.what();failed=true;}});std::thread receiver([&]{try{block_receive(options,sink,b,mode==BlockMode::Ymodem||padded?std::optional<uint64_t>{}:std::optional<uint64_t>{all.size()});}catch(const std::exception& e){receiverError=e.what();failed=true;}});sender.join();receiver.join();if(failed)throw std::runtime_error("roundtrip: "+senderError+" / "+receiverError);
    check(commits==inputs.size(),"exactly one commit per transfer file");for(const auto& input:inputs){const auto& out=outputs[input.first];if(padded){check(out.size()==33024&&out.substr(0,input.second.size())==input.second&&out.back()==26,"XMODEM unknown size preserves padding");}else check(out==input.second,"file bytes survive split/CRC/retry/wrap/batch");}if(corrupt)check(damaged==1,"corruption fixture used");if(drop)check(dropped==1,"lost ACK fixture used");
}
}
int main(){try{check(xmodem_crc("123456789")==0x31C3,"XMODEM CRC init-zero known vector");for(const auto mode:{BlockMode::XChecksum,BlockMode::XCrc,BlockMode::X1K,BlockMode::Ymodem})roundtrip(mode,true,true);roundtrip(BlockMode::XCrc,false,false,true);
    for(const auto name:{"../file.bin","a/b","a\\b","CON.txt","nul","COM1.bin","file.","file ","C:file",""})check(!safe_transfer_name(name),"unsafe received filename rejected");check(safe_transfer_name("safe-1.bin"),"portable basename accepted");
    std::vector<std::string> sent;unsigned reads=0;BlockLink silent{[&](unsigned){++reads;return -1;},[&](std::string_view s){sent.emplace_back(s);},[]{},{}};BlockSource source{"a.bin",1,[](uint64_t,size_t){return std::string("a");}};bool rejected=false;try{block_send({BlockMode::XCrc,100,2},{source},silent);}catch(...){rejected=true;}check(rejected&&reads==2&&sent.empty(),"bounded silent-peer handshake");
    unsigned cancels=0;BlockLink cancelled{[&](unsigned){return 24;},[](std::string_view){},[&]{++cancels;},{}};rejected=false;try{block_send({BlockMode::XCrc,100,2},{source},cancelled);}catch(...){rejected=true;}check(rejected&&cancels<10,"remote double-CAN abort");
    auto bad=xmodem_packet(0,std::string("../x.bin\0" "1\0",11),128,true);size_t at=0;bool began=false;BlockLink unsafe{[&](unsigned){return at<bad.size()?static_cast<unsigned char>(bad[at++]):-1;},[](std::string_view){},[]{},{}};BlockSink sink{[&](const auto&,auto){began=true;},[](auto){},[]{}};rejected=false;try{block_receive({BlockMode::Ymodem,100,1},sink,unsafe);}catch(...){rejected=true;}check(rejected&&!began,"unsafe metadata never opens output");
    std::cout<<"PASS: "<<checks<<" X/YMODEM wire/CRC/retry/batch/path checks; in-memory peers only.\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
