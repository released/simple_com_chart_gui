#include "plot_envelope.h"
#include "text_telemetry.h"
#include "replay_binary.h"
#include "derived_channels.h"
#include "binary_inspection.h"
#include "run_analysis.h"
#include "serial_forwarder.h"
#include <windows.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    std::vector<ChannelSample> raw;for(int i=0;i<20000;++i)raw.push_back({i/20000.0,0});
    raw[102].v=1000;raw[104].v=-201;raw[106].v=500;
    const auto reduced=plot_envelope(raw,0,1,200);
    bool high=false,low=false;double time=-1;
    for(const auto& p:reduced){check(p.t>=time,"ordered extrema");time=p.t;high|=p.v==1000;low|=p.v==-201;}
    check(high&&low,"both narrow peaks survive");check(reduced.size()<=804,"bounded output");
    check(raw.size()==20000&&raw[102].v==1000,"raw unchanged");
    const auto spans=plot_pixel_spans(raw,0,1,200);bool span_high=false,span_low=false;for(const auto& span:spans){span_high|=span.high.v==1000;span_low|=span.low.v==-201;}check(span_high&&span_low&&spans.size()<=201,"pixel envelope preserves both extrema with bounded strokes");
    check(plot_envelope(raw,2,3,100).empty(),"out of range");
    check(plot_envelope(raw,0,1,0).empty(),"zero width");
    const auto begin=std::chrono::steady_clock::now();size_t points=0;
    for(int i=0;i<200;++i)points+=plot_envelope(raw,0,1,1000).size();
    std::cout<<"PASS: ordered peak envelope; 200x20k samples "
        <<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()
        <<" ms; emitted "<<points<<" points\n";
    TextTelemetry parser;TextTelemetryConfig config;config.enabled=true;config.include="fan";config.exclude="bad";std::string error;
    check(parser.configure(config,error),"B parser configuration");
    parser.receive({1,0,false,"fan,rpm:20",10,1});check(parser.take().empty(),"split line waits");
    parser.receive({2,0,false,"1\nfan,bad,rpm:999\n",11,1});auto frames=parser.take();
    check(frames.size()==1&&frames[0].values.at("rpm")==201&&frames[0].seconds==11,"B values/filter/receipt time");
    parser.receive({3,0,false,"fan,rpm:9",12,1});parser.receive({4,0,false,"fan,rpm:1000\n",13,2});frames=parser.take();
    check(frames.size()==1&&frames[0].values.at("rpm")==1000,"no cross-generation partials");
    parser.receive({5,0,true,"fan,rpm:999\n",14,2});check(parser.take().empty(),"TX never parsed as RX");
    config.extraction.mode=ExtractionConfig::Csv;config.extraction.columns="rpm,temp";config.include="";check(parser.configure(config,error),"B CSV config");
    parser.receive({6,0,false,"201,-2.5\n",15,2});frames=parser.take();check(frames.size()==1&&frames[0].values.at("temp")==-2.5,"B CSV extraction");
    config.include="[";config.regex=true;check(!parser.configure(config,error),"invalid B filter rejected");
    std::cout<<"PASS: independent bounded Port B text extraction\n";
    ReplayBinary replay;BinaryFrameConfig binary;binary.header="H";binary.fixed_bytes=3;binary.fields={{"v","u16le",1,1,0}};
    check(replay.decoder.configure(binary,error),"replay schema");ReplayRecord record;record.seconds=2;record.generation=1;
    check(replay.feed(record,"H").empty(),"partial replay frame");record.seconds=3;auto decoded=replay.feed(record,std::string("\xE9\0",2));
    check(decoded.size()==1&&decoded[0].values[0].second==233&&decoded[0].sample_seconds==4,"replay frame completion clock");
    replay.feed(record,"H");record.generation=2;check(replay.feed(record,std::string("\xE9\0",2)).empty(),"no frame across recorded reconnect");
    std::cout<<"PASS: binary replay partials/receipt clocks/generations\n";
    DerivedChannels derived;check(derived.configure("power = v*i\nerror = target-actual\nsmooth = ema(v,0.5)\nnext = calc.power + abs(-2)",error),"formulas configure");
    auto result=derived.evaluate({{"v",12},{"i",2},{"target",1000},{"actual",201}});check(result.at("calc.power")==24&&result.at("calc.error")==799&&result.at("calc.next")==26,"arithmetic and dependency");
    result=derived.evaluate({{"v",20}});check(result.count("calc.power")==0&&result.at("calc.smooth")==16,"no stale inputs; EMA");
    check(!derived.configure("bad = calc.bad+1",error),"cycle rejection retains definitions");check(!derived.configure("bad = system(1)",error),"no executable functions");
    check(derived.configure("a=1/0\nb=sqrt(-1)",error)&&derived.evaluate({}).empty(),"invalid arithmetic omitted");
    std::cout<<"PASS: bounded derived expressions/EMA/missing data/cycles\n";
    std::vector<BitField> bits;check(parse_bit_fields("state,1,u16le,0,2,0=OFF|1=ON\r\n",bits,error),"bit schema");
    auto rows=inspect_binary(std::string("H\1\0",3),binary,bits);check(rows.size()==2&&rows.back().valid&&rows.back().value=="1 = ON"&&rows.back().offset==1,"bit span and enum");
    check(!inspect_binary("H",binary,bits).front().valid,"inspector truncated field");
    check(!parse_bit_fields("x,0,u8,7,2,\r\n",bits,error),"bit width overflow");
    binary.checksum=FrameChecksum::Sum8;binary.checksum_from=0;check(inspect_checksum(std::string("\1\2\3",3),binary).find("PASS")!=std::string::npos,"checksum computed/received match");
    check(inspect_checksum(std::string("\1\2\4",3),binary).find("FAIL")!=std::string::npos,"checksum mismatch visible");
    std::cout<<"PASS: read-only binary spans/bits/enums/CRC diagnostics\n";
    CaptureStore xy;xy.append("",{{"x",1},{"y",2}},10);xy.append("",{{"x",3}},11);xy.append("",{{"y",4}},12);xy.append("",{{"x",5},{"y",6}},13);
    const auto pairs=xy_pairs(xy.snapshot(),"x","y");check(pairs.size()==2&&pairs.back()==std::make_pair(5.0,6.0),"XY same row only");check(xy_pairs(xy.snapshot(),"missing","y").empty(),"XY missing field");
    const auto folder=std::filesystem::path(__FILE__).parent_path().parent_path()/L"build"/L"verify"/L"analysis-core"/std::to_wstring(GetCurrentProcessId());std::filesystem::create_directories(folder);std::atomic<bool> cancel{false};const auto csv=folder/L"run.csv";
    const std::string csv_bytes="\xEF\xBB\xBF\"Row\",\"Time (s)\",\"v\"\r\n1,0,201\r\n2,1,202\r\n";uart_write_file(csv,csv_bytes,false,cancel);RunOptions options;auto run_a=load_recorded_run(csv,options,cancel);auto run_b=run_a;
    auto comparison=compare_runs(run_a,run_b,"v",0,0,0);check(comparison.matches.size()==2&&comparison.passed==2&&comparison.unmatched_a==0,"exact comparison no interpolation");comparison=compare_runs(run_a,run_b,"v",0.1,0.01,0);check(comparison.matches.empty()&&comparison.unmatched_a==2&&comparison.unmatched_b==2,"comparison respects time tolerance");check(comparison_csv(comparison).find("Unmatched_A,2")!=std::string::npos,"comparison structured CSV");check(uart_read_file(csv,10000,cancel)==csv_bytes,"run source unchanged");
    const auto log=folder/L"run.log";uart_write_file(log,"v:1\nv:2\n",false,cancel);auto imported=load_recorded_run(log,options,cancel);check(imported.data.rows.size()==2&&imported.provenance.find("SYNTHETIC")!=std::string::npos,"log import explicit synthetic clock");
    std::cout<<"PASS: sparse XY/read-only run import/alignment/tolerance/report\n";
    SerialForwarder forward;ForwardOptions forwarding;forwarding.seconds=1;forwarding.byte_limit=3;
    check(forward.start(forwarding,{41,42},0,error),"forward start");forward.receive(0,{1,0,false,std::string("A\0B",3),0,41},0);std::string outgoing;check(forward.next(1,outgoing)&&outgoing==std::string("A\0B",3)&&!forward.next(1,outgoing),"forward bytes and one in-flight per port");forward.complete(1,true,3);check(!forward.active()&&forward.accepted()==3,"forward finite budget drains");
    forwarding.direction=ForwardOptions::Both;forwarding.byte_limit=1024;check(forward.start(forwarding,{41,42},0,error),"bidirectional start");forward.receive(0,{1,0,false,"PING",0,41},0);check(forward.next(1,outgoing),"forward A to B");forward.receive(1,{2,0,false,"PING",0,42},1);check(!forward.active()&&forward.status().find("loop")!=std::string::npos,"possible echoed loop stops");
    check(forward.start(forwarding,{41,42},0,error),"restart explicit only");forward.receive(0,{1,0,false,"ABC",0,41},0);forward.next(1,outgoing);forward.complete(1,false,1);check(!forward.active()&&forward.accepted()==1&&!forward.next(1,outgoing),"partial forwarding stops without retry");
    forward.start(forwarding,{41,42},0,error);forward.receive(0,{1,0,true,"TX",0,41},0);check(!forward.next(1,outgoing),"accepted TX not echoed as RX");forward.receive(0,{2,0,false,"NEW",0,99},0);check(!forward.active(),"forward generation change stops");forward.start(forwarding,{41,42},0,error);forward.tick(1000);check(!forward.active(),"forward time limit");
    std::cout<<"PASS: bounded forwarding/exclusive in-flight/echo/partial/generation/deadline\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
