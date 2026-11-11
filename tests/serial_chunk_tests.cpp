#include "serial_manager.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace metrics { size_t count=0,bytes=0;bool active=false; }
void* operator new(size_t bytes){if(auto* p=std::malloc(bytes?bytes:1)){if(metrics::active){++metrics::count;metrics::bytes+=bytes;}return p;}throw std::bad_alloc();}
void* operator new[](size_t bytes){return ::operator new(bytes);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
class Legacy : public SerialTransport {
public:
    std::string payload;bool connected=true;size_t frames=0;
    bool is_connected()const override{return connected;}
    bool connect(const std::wstring&,int,int,int,int,std::wstring*)override{return connected=true;}
    void disconnect()override{connected=false;}
    int consume_rx_overflow()override{return 0;}
    std::vector<std::string> read_lines(std::wstring*,std::string* raw)override{
        if(raw)*raw=payload;std::vector<std::string> result;
        size_t begin=0;for(size_t end=0;(end=payload.find('\n',begin))!=std::string::npos;begin=end+1)result.push_back(payload.substr(begin,end-begin));
        frames+=result.size();return result;
    }
};
class Raw final : public Legacy {
public:
    std::string read_chunk(std::wstring*,std::vector<std::string>* lines=nullptr)override{if(lines)lines->clear();return payload;}
    std::vector<std::string> read_lines(std::wstring*,std::string*)override{throw std::runtime_error("Raw path called legacy framing");}
};
int main(){try{
    Legacy legacy;Raw raw;std::wstring error;
    legacy.payload=std::string("A\0B\r\n",6);std::vector<std::string> lines;
    const auto bytes=legacy.read_chunk(&error,&lines);
    if(bytes!=legacy.payload||lines.size()!=1||lines[0]!=std::string("A\0B\r",4))throw std::runtime_error("Legacy adapter changed bytes or fallback lines");
    raw.payload=legacy.payload;lines={"stale"};
    if(raw.read_chunk(&error,&lines)!=raw.payload||!lines.empty())throw std::runtime_error("Raw path fabricated framing");
    std::cout<<"PASS: legacy adapter preserves exact binary bytes and optional fallback lines\nPASS: raw override bypasses line framing and clears stale fallback\n";
    std::string batch;while(batch.size()<65536)batch+="channel_long_name:123.456, other_long_name:7.89\r\n";
    legacy.payload=raw.payload=batch;
    for(bool direct:{false,true}){
        SerialTransport& transport=direct?static_cast<SerialTransport&>(raw):static_cast<SerialTransport&>(legacy);
        metrics::count=metrics::bytes=0;metrics::active=true;const auto begin=std::chrono::steady_clock::now();size_t total=0;
        for(int n=0;n<200;++n)total+=transport.read_chunk(&error).size();
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();metrics::active=false;
        if(total!=batch.size()*200)throw std::runtime_error("Raw chunk benchmark byte mismatch");
        std::cout<<"BENCH raw_chunk="<<direct<<" iterations=200 payload="<<batch.size()<<" total_ms="<<ms<<" cpp_allocations="<<metrics::count<<" cpp_allocated_bytes="<<metrics::bytes<<'\n';
    }
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
