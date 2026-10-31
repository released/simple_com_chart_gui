#include "channel_model.h"
#include "error_message.h"
#include "background_job.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

void test_series_access() {
    if(error_message(u8"Cannot open 測試.log")!=L"Cannot open 測試.log"||error_message(std::string("bad\xFF",4)).find(L"\\xFF")==std::wstring::npos)
        throw std::runtime_error("UTF-8 diagnostic conversion/fallback differs");
    BackgroundJob<int> job;std::wstring error;int output=0;
    if(!job.start([](const std::atomic<bool>&)->int{throw std::runtime_error(u8"Cannot open 測試.log");}))throw std::runtime_error("Cannot start diagnostic fixture");
    const auto deadline=GetTickCount64()+2000;bool done=false;
    while(GetTickCount64()<deadline&&!(done=job.take(output,error)))Sleep(1);
    if(!done||error!=L"Cannot open 測試.log")throw std::runtime_error("Background UTF-8 exception was not preserved");
    ChannelModel model;model.set_time_window(300);
    for(int i=0;i<20000;++i)model.update_from_kv({{"sample",std::sin(i*.01)}},i*.001);
    const double first=5,last=15;const auto whole=model.get_series("sample");std::vector<ChannelSample> expected;
    for(const auto& sample:whole)if(sample.t>=first&&sample.t<=last)expected.push_back(sample);
    const auto actual=model.series_between("sample",first,last);ChannelSample latest;double low=0,high=0;
    if(actual.size()!=expected.size()||!model.extrema_between("sample",first,last,low,high)||!model.last_between("sample",first,last,latest))throw std::runtime_error("Visible series missing");
    for(size_t i=0;i<actual.size();++i)if(actual[i].t!=expected[i].t||actual[i].v!=expected[i].v)throw std::runtime_error("Visible series differs");
    const auto extrema=std::minmax_element(expected.begin(),expected.end(),[](const auto& a,const auto& b){return a.v<b.v;});
    if(low!=extrema.first->v||high!=extrema.second->v||latest.t!=expected.back().t||latest.v!=expected.back().v||model.last_between("missing",0,1,latest)||!model.series_between("sample",2,1).empty())throw std::runtime_error("Visible extrema/latest/bounds differ");
    std::vector<double> reference,optimized;volatile double sink=0;
    for(int run=0;run<5;++run){
        auto start=std::chrono::steady_clock::now();
        for(int n=0;n<20;++n){
            auto fit=model.get_series("sample");double minimum=1,maximum=-1;
            for(const auto& p:fit)if(p.t>=first&&p.t<=last){minimum=std::min(minimum,p.v);maximum=std::max(maximum,p.v);}
            const auto paint=model.get_series("sample");std::vector<ChannelSample> window;
            for(const auto& p:paint)if(p.t>=first&&p.t<=last)window.push_back(p);
            const auto labels=model.get_series("sample");const auto end=std::upper_bound(labels.begin(),labels.end(),last,[](double t,const auto& p){return t<p.t;});
            sink=minimum+maximum+window.front().v+(end-1)->v;
        }
        reference.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());start=std::chrono::steady_clock::now();
        for(int n=0;n<20;++n){double minimum=0,maximum=0;model.extrema_between("sample",first,last,minimum,maximum);const auto window=model.series_between("sample",first,last);ChannelSample label;model.last_between("sample",first,last,label);sink=minimum+maximum+window.front().v+label.v;}
        optimized.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
    }
    std::sort(reference.begin(),reference.end());std::sort(optimized.begin(),optimized.end());
    std::cout<<"BENCH: 20 updates x 20000 retained / "<<actual.size()<<" visible; copied samples per update "<<whole.size()*3+expected.size()<<" -> "<<actual.size()<<"; median reference_ms="<<reference[2]<<" optimized_ms="<<optimized[2]<<"; checksum="<<sink<<" (CPU access only, not GUI throughput)\n";
}
