#include "packet_builder.h"
#include "lab_metrics.h"
#include "binary_frame.h"
#include "uart_files.h"
#include "line_filter.h"
#include "binary_inspection.h"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string hex(const char* s){std::string out;require(uart_unhex(s,out),"fixture HEX");return out;}
void codec_tests() {
    require(wire_encode(hex("110022"),WireCodec::Cobs)==hex("0211022200"),"COBS reference vector");
    require(wire_encode(hex("C0DB00"),WireCodec::Slip)==hex("C0DBDCDBDD00C0"),"SLIP escaping vector");
    for(auto mode:{WireCodec::Cobs,WireCodec::Slip})for(size_t n:{size_t(1),size_t(254),size_t(255),size_t(1024),size_t(65536)}) {
        std::string input;for(size_t i=0;i<n;++i)input+=static_cast<char>((i*37)%256);
        const auto encoded=wire_encode(input,mode);WireDecoder decoder;decoder.configure(mode);std::vector<std::string> result;
        for(size_t at=0;at<encoded.size();at+=31){auto part=decoder.feed(std::string_view(encoded).substr(at,31));result.insert(result.end(),part.begin(),part.end());}
        require(result.size()==1&&result[0]==input&&decoder.errors()==0,"split codec exact round trip");
    }
    WireDecoder bad;bad.configure(WireCodec::Slip);require(bad.feed(hex("DB0100C0")).empty(),"invalid SLIP discarded");require(bad.errors()==1,"SLIP error observable");
    require(bad.feed(wire_encode("OK",WireCodec::Slip))==std::vector<std::string>{"OK"},"SLIP resynchronizes");
}
void packet_tests() {
    PacketDefinition p;p.command.hex=true;p.command.body="AA {address:u8} {target:u16le} {length:u8} {sequence:u8}";
    p.parameters="address,1,0,255\r\ntarget,1200,0,65535\r\n";p.sequence=7;p.command.checksum=FrameChecksum::Sum8;
    std::string bytes,error;require(packet_payload(p,bytes,error),"multi parameter packet");require(bytes==hex("AA01B00407076D"),"length and final checksum order");
    PacketDefinition restored;require(decode_packet(encode_packet(p),restored,error),"packet private definition round trip");std::string again;require(packet_payload(restored,again,error)&&again==bytes,"packet round trip exact bytes");
    p.parameters="address,256,0,255\r\ntarget,1,0,2";const auto before=bytes;require(!packet_payload(p,bytes,error)&&bytes==before,"invalid parameter preserves prior output");
    p.parameters="address,1,0,255\r\ntarget,1200,0,65535";p.codec=WireCodec::Cobs;require(packet_payload(p,bytes,error),"explicit wire encoded packet");WireDecoder d;d.configure(WireCodec::Cobs);require(d.feed(bytes)==std::vector<std::string>{before},"wire encode after checksum");
}
void schema_tests() {
    BinaryFrameConfig c;c.header=hex("AA");c.fixed_bytes=4;std::string error;
    require(parse_binary_layout("@id,1,1,Little\n@case,1,temperature,2,u16le\n@case,2,count,2,u16be",c,error),"type dispatch schema");
    BinaryFrameDecoder decoder;require(decoder.configure(c,error),"configure dispatch");const auto frames=decoder.feed(hex("AA010102AA020102AA030102"),42,1);
    require(frames.size()==3&&frames[0].values[0].first=="temperature"&&frames[0].values[0].second==513&&frames[1].values[0].second==258,"per type fields and endian");
    require(frames[2].values.empty()&&decoder.unknown_types()==1,"unknown type raw preserved");
    const auto previous=c;require(!parse_binary_layout("@id,1,1,Little\n@case,1,same,2,u8\n@case,1,same,3,u8",c,error)&&c.variants.size()==previous.variants.size(),"duplicate case fields rejected atomically");
    c=previous;c.codec=WireCodec::Cobs;require(decoder.configure(c,error),"COBS schema");const auto encoded=wire_encode(hex("AA010102"),WireCodec::Cobs);
    require(decoder.feed(encoded.substr(0,2),20).empty(),"packet partial waits");auto decoded=decoder.feed(encoded.substr(2),30);require(decoded.size()==1&&decoded[0].receipt_ms==30,"codec completion receipt time");
    decoder.feed(encoded.substr(0,2),40);decoder.interrupt();require(decoder.feed(encoded.substr(2),50).empty(),"no codec partial across generation");
    BinaryFrameConfig short_frame;short_frame.fixed_bytes=1;short_frame.fields={{"v","u8",0,1,0}};short_frame.codec=WireCodec::Cobs;require(decoder.configure(short_frame,error),"short encoded frames");
    std::string many;for(int i=0;i<600;++i)many+=wire_encode("Q",WireCodec::Cobs);auto first=decoder.feed(many,1000,1);auto second=decoder.feed(wire_encode("Z",WireCodec::Cobs),2000,2);auto third=decoder.feed({},0);
    require(first.size()==256&&second.size()==256&&third.size()==89&&decoder.codec_errors()==0,"codec budget defers queued packets without dropping normal bursts");
    require(second.back().receipt_ms==1000&&third.front().receipt_ms==1000&&third.back().receipt_ms==2000&&third.back().values[0].second==90,"deferred codec packets retain their actual receipt stamps");
    const auto inspected=inspect_binary(hex("AA010102"),previous,{});require(inspected.size()==2&&inspected[0].name=="@id"&&inspected[1].name=="temperature"&&inspected[1].value=="513","inspector selects the matching message layout");
}
void metrics_tests() {
    SequenceTracker t;std::string error;require(t.configure(256,1,error),"sequence options");
    for(double v:{254.,255.,0.,0.,3.,2.})t.receive(v,1);t.receive(10,2);t.receive(2.5,2);
    const auto s=t.stats();require(s.samples==7&&s.wraps==1&&s.duplicates==1&&s.jumps==1&&s.backwards==1&&s.resets==1&&s.invalid==1,"sequence classifications");
    const auto l=latency_stats({1,2,3,100});require(l.count==4&&l.p50==2&&l.p95==100&&l.p99==100&&l.mean==26.5,"nearest rank response statistics");
    PipelineDiagnostics p;p.observe(PipelineDiagnostics::Excluded,"fan:1\r\n");p.observe(PipelineDiagnostics::NoValues,"text");require(p.report().find("Excluded: 1")!=std::string::npos,"diagnostic counters");
    const auto r=check_parser_cases(ExtractionConfig{},"Input,Expected,Seconds\r\n\"x:1,y:2\",\"x:1,y:2\",\r\n\"x:3\",\"x:4\",\r\n",0);
    require(r.passed==1&&r.failed==1,"shared parser case success and mismatch");
    bool rejected=false;try{check_parser_cases(ExtractionConfig{},"Input,Expected,Seconds\r\ny:1,\"x:bad,y:1\",\r\n",0);}catch(...){rejected=true;}require(rejected,"malformed assertions are rejected, never silently ignored");
    std::vector<DigitalLane> lanes;require(parse_digital_lanes("ready,flags,0,1\nmode,flags,1,3\nstate,state,0,0",lanes,error),"digital lane configuration");
    require(digital_value(13,lanes[0])==1u&&digital_value(13,lanes[1])==6u&&digital_value(20,lanes[2])==20u&&!digital_value(-1,lanes[0]),"bit/enum projection and invalid gaps");
    CaptureStore capture;capture.append("",{{"flags",0}},10,UINT64_MAX,0,1,0);capture.append("",{{"B.other",5}},10.5,UINT64_MAX,0,1,1);capture.append("",{{"flags",1}},11,UINT64_MAX,0,1,0);
    capture.append("",{{"flags",0}},12,UINT64_MAX,0,2,0);capture.append("",{{"flags",1}},13,UINT64_MAX,0,2,0);
    capture.append("",{{"B.other",6}},12.5,UINT64_MAX,0,1,1);
    DigitalLane ready{"Ready","flags",0,1,false};const auto history=digital_history(capture.snapshot(),ready,30,2);
    require(history.segments.size()==2&&history.segments[0].last==1&&history.segments[1].first==2,"digital lanes skip other sources and do not join generations");
    require(digital_history(capture.snapshot(),ready,30,.1).segments.empty(),"digital maximum hold leaves gaps");
    LineFilter filter;require(filter.configure("fan","error",true,error),"diagnostic filter config");require(filter.evaluate("fan error")==LineFilter::Excluded&&filter.evaluate("other")==LineFilter::IncludeMiss&&filter.evaluate("fan")==LineFilter::Accept,"filter stage reasons preserve exclusion priority");
}
}
void lab_files_tests();
void lab_runner_tests();
int main(){try{codec_tests();packet_tests();schema_tests();metrics_tests();lab_files_tests();lab_runner_tests();std::cout<<"PASS: UART laboratory codec/packet/schema/metrics/parser/files/finite runner\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
