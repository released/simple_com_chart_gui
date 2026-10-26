#include "packet_builder.h"
#include "field_extractor.h"
#include "text_telemetry.h"
#include "device_catalog.h"
#include "time_calculus.h"
#include "session_metadata.h"
#include "settings_fields.h"
#include <cmath>
#include <stdexcept>
#include <sstream>
void session_check(bool,const char*);
void session_algorithms_tests(){
    session_check(CustomCrc(parse_crc_spec("8:07:00:00:0:0")).calculate("123456789")==0xF4&&CustomCrc(parse_crc_spec("16:1021:FFFF:0:0:0")).calculate("123456789")==0x29B1&&CustomCrc(parse_crc_spec("16:8005:FFFF:0:1:1")).calculate("123456789")==0x4B37&&CustomCrc(parse_crc_spec("32:04C11DB7:FFFFFFFF:FFFFFFFF:1:1")).calculate("123456789")==0xCBF43926,"CRC8/16/32 and reflected known check vectors");
    std::string bytes,error;PacketDefinition p;p.command.name="CRC test";p.command.hex=true;p.command.body="31 32 33 34 35 36 37 38 39";p.custom_crc=parse_crc_spec("8:07:0:0:0:0");
    session_check(packet_payload(p,bytes,error)&&bytes==std::string("123456789")+char(-12),"packet uses shared custom CRC before ending/codec");
    PacketDefinition q;session_check(decode_packet(encode_packet(p),q,error)&&q.custom_crc.has_value()&&packet_payload(q,bytes,error),"custom packet V2 round trip");
    p.custom_crc.reset();session_check(encode_packet(p).rfind("SCC_PACKET_V1",0)==0,"legacy packet V1 remains unchanged");
    p.custom_crc=parse_crc_spec("16:1021:FFFF:0:0:0");p.command.body="AA 31 32 33 34 35 36 37 38 39 BB";p.command.checksum_big=true;p.checksum_from=1;p.checksum_count=9;p.command.ending=TxEnding::CRLF;p.codec=WireCodec::Cobs;
    session_check(packet_payload(p,bytes,error),"custom range/ending/codec packet builds");WireDecoder codec;codec.configure(p.codec);const auto decoded_frames=codec.feed(bytes);session_check(decoded_frames.size()==1,"custom CRC packet survives COBS decoding");const auto& decoded=decoded_frames[0];
    session_check(decoded.size()==15&&static_cast<unsigned char>(decoded[11])==0x29&&static_cast<unsigned char>(decoded[12])==0xB1&&decoded.substr(13)=="\r\n","CRC range excludes surrounding bytes and precedes ending/wire encoding");
    p.codec=WireCodec::None;p.command.body="{length:u8} 31 32 33 34 35 36 37 38 39";p.custom_crc=parse_crc_spec("8:07:0:0:0:0");p.command.ending=TxEnding::LF;
    session_check(packet_payload(p,bytes,error)&&bytes.size()==12&&bytes[0]==12&&static_cast<unsigned char>(bytes[10])==0xF4,"length convergence includes custom CRC/ending");
    p.checksum_count=10;session_check(!packet_payload(p,bytes,error),"custom CRC rejects range beyond expanded payload");
    BinaryFrameConfig c;c.fixed_bytes=10;c.header="1";session_check(parse_binary_layout("@crc,8,07,00,00,0,0\nv,0,u8",c,error),"native binary layout accepts bounded @crc directive");
    BinaryFrameDecoder decoder;session_check(decoder.configure(c,error),"custom-CRC decoder configures once");auto frame=decoder.feed(std::string("123456789")+char(-12),1000,1);
    session_check(frame.size()==1&&frame[0].values[0].second==49,"live/offline binary core validates custom CRC");
    session_check(decoder.feed(std::string("123456789")+char(0),1010,1).empty()&&decoder.feed(std::string("123456789")+char(-12),1020,1).size()==1,"corrupt custom CRC is rejected and later valid frame resynchronizes");
    const auto before=c;session_check(!parse_binary_layout("@crc,7,07,00,00,0,0\nv,0,u8",c,error)&&c.custom_crc->width==before.custom_crc->width,"invalid CRC does not replace valid schema");
    SourceLineTracker lines;auto mapped=lines.feed("a:1\r\nb:2\n",1);session_check(mapped.size()==2&&mapped[0].offset==0&&mapped[0].count==5&&mapped[1].offset==5,"compatible LF/CRLF mapping retains exact byte ranges");
    TextFraming f;f.mode=TextFraming::Delimiter;f.delimiter=std::string("\0END",4);lines.configure(f);const auto offset=lines.end();
    session_check(lines.feed(std::string("v:2\0E",5),2).empty(),"custom binary-safe delimiter waits across reads");mapped=lines.feed("ND",3);session_check(mapped.size()==1&&mapped[0].text=="v:2"&&mapped[0].offset==offset&&mapped[0].count==7&&mapped[0].seconds==3,"split delimiter emits exact offset/count and completion receipt");
    f.mode=TextFraming::Idle;f.idle_ms=20;lines.configure(f);session_check(lines.feed("a:1",10).empty(),"idle text does not emit early");mapped=lines.feed("b:2",10.05);session_check(mapped.size()==1&&mapped[0].text=="a:1"&&mapped[0].seconds==10&&lines.drain(10.06).empty(),"idle framing preserves previous chunk clock");mapped=lines.drain(10.08);session_check(mapped.size()==1&&mapped[0].text=="b:2"&&mapped[0].seconds==10.05,"quiet input drains independently of new bytes");
    session_check(decode_text_framing(encode_text_framing(f)).idle_ms==20,"framing preferences round trip");
    f.mode=TextFraming::CR;lines.configure(f);mapped=lines.feed("a:1\r\nb:2\r",11);session_check(mapped.size()==2&&mapped[1].text=="b:2"&&mapped[1].offset==mapped[0].offset+5,"CR framing skips paired LF without losing following byte offsets");
    f.mode=TextFraming::CRLF;lines.configure(f);session_check(lines.feed("a:1\r",12).empty()&&lines.feed("\nb:2\r\n",13).size()==2,"CRLF delimiter survives a split read");
    f.mode=TextFraming::Delimiter;f.delimiter="END";lines.configure(f);mapped=lines.feed(std::string(65537,'X')+"ENDv:3END",20);session_check(mapped.size()==1&&mapped[0].text=="v:3"&&lines.skipped()==1,"oversize input resynchronizes at selected delimiter");
    TextTelemetry b;TextTelemetryConfig config;config.enabled=true;session_check(b.configure(config,error),"B parser configures");f.mode=TextFraming::Idle;b.framing(f);b.receive({1,1000,false,"old:1",30,1});b.receive({2,2000,false,"new:2",31,2});b.drain(32);auto values=b.take();session_check(values.size()==1&&values[0].values.count("new")&&!values[0].values.count("old"),"B partial text never crosses generations");
    FieldExtractor extractor;ExtractionConfig json;json.mode=ExtractionConfig::Json;json.columns="old,csv,columns";session_check(extractor.configure(json,error)&&extractor.extract("{\"v\":201,\"nested\":{\"x\":1}}").values.size()==1,"legacy flat JSON ignores old unused column names");
    json.columns="ptr:rpm=/fan/rpm,t=/temps/1,escaped=/a~1b/~0,\"comma=/a,b\"";session_check(extractor.configure(json,error),"explicit JSON Pointer selectors configure");
    auto result=extractor.extract(R"({"fan":{"rpm":2340},"temps":[1,201],"a/b":{"~":7},"a,b":9})");session_check(result.error.empty()&&result.values.size()==4&&result.values.at("rpm")==2340&&result.values.at("t")==201&&result.values.at("escaped")==7&&result.values.at("comma")==9,"nested object/array/escaped/comma paths extract numeric values only");
    session_check(extractor.extract(R"({"fan":null})").values.empty()&&!extractor.extract(R"({"fan":{"rpm":1,"rpm":2}})").error.empty(),"missing JSON values are blank and ambiguous keys rejected");
    auto invalid_json=json;invalid_json.columns="ptr:x=/bad~2";session_check(!extractor.configure(invalid_json,error)&&extractor.config().columns==json.columns,"malformed pointer preserves previous validated parser");
    const auto deep=std::string(20,'[')+"1"+std::string(20,']');std::string broad="[0";for(unsigned i=0;i<4097;++i)broad+=",0";broad+=']';session_check(!extractor.extract(deep).error.empty()&&!extractor.extract(broad).error.empty(),"selected JSON depth/work bounds reject excessive nesting and arrays");
    const auto saved=decode_extraction(encode_extraction(json));session_check(saved.columns==json.columns&&encode_extraction(json).rfind("v2:",0)==0&&encode_extraction(ExtractionConfig{}).rfind("v1:",0)==0,"explicit JSON paths get v2 while legacy parser preferences stay v1");
    const std::vector<std::wstring> b_fields{L"Enabled",L"",L"",L"Text",L"JSON",L"ptr:v=/nested/v",L"",L"1",L"2"};const auto packed=pack_text_settings(b_fields);bool old_rejects=false;try{unpack_settings(packed,9);}catch(...){old_rejects=true;}
    session_check(packed.rfind(L"v2:",0)==0&&unpack_text_settings(packed)==b_fields&&old_rejects,"Port B JSON path settings round-trip and old reader rejects instead of widening");
    CaptureStore capture;capture.append("",{{"v",2}},100,0,0,1,0);capture.append("",{{"v",4}},102,0,0,1,0);capture.append("",{{"v",100}},103,0,0,1,1);capture.append("",{{"other",1}},104,0,0,1,0);capture.append("",{{"v",8}},105,0,0,1,0);capture.append("",{{"v",10}},107,0,0,2,0);capture.append("",{{"v",12}},108,0,0,2,0);
    std::atomic<bool> cancel{false};auto derivative=time_calculus(capture.snapshot(),"v",0,false,3,cancel),integral=time_calculus(capture.snapshot(),"v",0,true,3,cancel);
    session_check(derivative.points.size()==6&&!derivative.points[0].valid&&derivative.points[1].value==1&&derivative.points.back().value==2,"time derivative ignores other-source rows and resets at gaps/generations");
    session_check(integral.points[0].valid&&integral.points[1].value==6&&!integral.points[2].valid&&integral.points.back().value==11,"trapezoidal integral has explicit segment origins and no gap bridge");
    CaptureStore extremes;extremes.append("",{{"v",1e308}},100,0,0,1,0);extremes.append("",{{"v",-1e308}},104,0,0,1,0);const auto huge=time_calculus(extremes.snapshot(),"v",0,false,5,cancel);
    session_check(huge.points.back().valid&&std::abs(huge.points.back().value/(-5e307)-1)<1e-12,"finite derivative survives intermediate subtraction overflow");
    DeviceDetails device;device.port=L"COM201";device.name=L"Fixture bridge";device.hardware=L"USB\\VID_1234&PID_5678";
    session_check(device_catalog_report({device},L"vid_1234").find(L"COM201")!=std::wstring::npos&&device_catalog_report({device},L"missing").find(L"Shown: 0")!=std::wstring::npos,"device report searches properties without opening hardware");
    UartProfile profile;profile.workspace=true;for(const auto& key:workspace_keys())profile.workspace_settings[key]="";profile.serial.port.device=L"COM201";profile.serial.port.instance_id=L"PRIVATE";
    profile.workspace_settings["Workbench.Text.B/Configuration"]=message_utf8(packed);
    std::istringstream old_input(encode_profile(profile));std::string line,old_profile;while(std::getline(old_input,line)){if(line.rfind("Workspace.InputFraming/",0)!=0&&line.rfind("Workspace.Recording/",0)!=0)old_profile+=line+'\n';}const auto legacy=decode_profile(old_profile);session_check(legacy.workspace_settings.at("InputFraming/A").empty()&&legacy.workspace_settings.at("Recording/Limits").empty(),"older workspace profiles default new framing/recording keys compatibly");
    const auto meta=encode_capture_metadata(profile,"1.0.1.029","A");const auto restored=decode_capture_metadata(meta+"Changed=1\r\n");session_check(restored.profile.serial.port.device.empty()&&restored.profile.serial.port.instance_id.empty()&&restored.changed,"capture-start snapshot excludes device IDs and marks later settings changes");
}
