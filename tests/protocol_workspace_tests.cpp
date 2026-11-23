#include "modbus_workspace.h"
#include "lin_protocol.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>

namespace {
unsigned checks=0;
void check(bool value,const char* name){++checks;if(!value)throw std::runtime_error(name);}
template<class F>void rejects(F f,const char* name){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,name);}
std::string crc(std::string data){const auto n=frame_checksum(data,FrameChecksum::Modbus16);data+=static_cast<char>(n);data+=static_cast<char>(n>>8);return data;}
struct FakeLin : LinTransport {
    LinCapabilities caps{"test-only",true,true,true,true,true};uint64_t gen=7,ticket=0;unsigned starts=0,cancels=0;bool ready=false;LinCompletion completion{true,false,false,7};
    LinCapabilities capabilities()const override{return caps;}uint64_t generation()const override{return gen;}
    bool submit(const LinSlot&,uint64_t& out,std::string&)override{out=++ticket;++starts;return true;}
    bool schedule(const LinSchedule&,unsigned,uint64_t& out,std::string&)override{out=++ticket;++starts;return true;}
    bool take(uint64_t,LinCompletion& out)override{if(!ready)return false;out=completion;ready=false;return true;}
    void cancel(uint64_t)override{++cancels;}
};
const char* ldf=R"(LIN_description_file;
LIN_protocol_version = "2.2"; LIN_language_version = "2.2"; LIN_speed = 19.2 kbps;
Nodes { Master: MasterNode, 5 ms, 0.1 ms; Slaves: Sensor; }
Signals { Temp: 12, 0, Sensor, MasterNode; Flag: 1, 1, MasterNode, Sensor; }
Frames { SensorFrame: 0x10, Sensor, 2 { Temp, 0; }
         MasterFrame: 0x11, MasterNode, 1 { Flag, 0; } }
Signal_encoding_types { Temperature { physical_value, 0, 4000, 0.1, -40, "degC"; logical_value, 4095, "Fault"; } }
Signal_representation { Temperature: Temp; }
Schedule_tables { Normal { SensorFrame delay 10 ms; MasterFrame delay 20 ms; } }
)";
}
int main(){try{
    std::string error,request;unsigned offset=99;
    check(modbus_address(3,"40001",true,offset,error)&&offset==0,"40001 maps to wire offset zero");
    check(modbus_address(4,"30010",true,offset,error)&&offset==9,"input reference uses correct table");
    check(!modbus_address(3,"30001",true,offset,error)&&offset==9,"invalid table preserves address result");
    check(!modbus_address(1,"-1",false,offset,error)&&!modbus_address(3,"1.0",false,offset,error),"decimal address is strict");
    ModbusPoint point;point.name="temperature";point.request.count=2;point.type="f32";point.scale=0.1;point.unit="degC";
    const auto saved=encode_modbus_points({point});const auto loaded=decode_modbus_points(saved);
    check(loaded.size()==1&&loaded[0].type=="f32"&&loaded[0].unit=="degC"&&loaded[0].scale==0.1,"typed definition round trip");
    rejects([&]{decode_modbus_points(saved+saved);},"extra definition headers rejected");
    auto invalid=point;invalid.request.count=1;check(!validate_modbus_points({invalid},error),"typed word count mismatch rejected");
    check(!validate_modbus_points({point,point},error),"duplicate point names rejected");
    RtuConfig bits;bits.function=1;bits.count=10;
    const auto wire=crc(std::string("\x01\x01\x02\x55\x03",5));
    for(size_t split=0;split<=wire.size();++split){RtuPoller poll;check(poll.start(bits,5,0,error)&&poll.next(2,request),"coil poll starts");poll.complete(true,3);
        poll.receive(std::string_view(wire).substr(0,split),4,0.1);poll.receive(std::string_view(wire).substr(split),5,0.2);const auto result=poll.take();
        check(result&&result->registers==std::vector<unsigned>({1,0,1,0,1,0,1,0,1,1}),"coil bits use LSB-first and discard padding bits");}
    auto discrete=bits;discrete.function=2;discrete.unit=2;RtuPoller plan;
    check(plan.start_plan({bits,discrete},8,0,error)&&plan.next(2,request)&&request[0]==1,"multi-unit starts first block");plan.complete(true,3);plan.receive(wire,4,1);
    check(!plan.next(10,request),"unconsumed response prevents overwrite");check(plan.take()->block==0,"first block identity retained");
    check(plan.next(10,request)&&request[0]==2&&request[1]==2,"second block rotates fairly");plan.complete(true,11);plan.tick(1011);
    check(!plan.active()&&plan.transactions().back().outcome==RtuOutcome::Timeout&&!plan.next(99999,request),"timeout stops entire multi-unit plan without retry");
    const auto csv=modbus_transactions_csv(plan.transactions());check(csv.find("Timeout")!=csv.npos&&plan.transactions().front().outcome==RtuOutcome::Success,"typed transaction diagnostics keep success and timeout");
    RtuPoller stopped;check(stopped.start(discrete,9,0,error)&&stopped.next(200,request),"late first request starts after idle");stopped.complete(true,201);stopped.tick(250);stopped.stop("Stopped");
    check(stopped.transactions().back().finished_ms==250&&modbus_transactions_csv(stopped.transactions()).find(",50,")!=std::string::npos,"Stop duration uses latest observation, never old RX time or unsigned underflow");
    RtuConfig converted;converted.fields={{"scaled","u16","ABCD",0,std::numeric_limits<double>::max(),0}};RtuPoller overflow;
    check(overflow.start(converted,10,0,error)&&overflow.next(2,request),"finite large scale is accepted");overflow.complete(true,3);overflow.receive(crc(std::string("\x01\x03\x02\x00\x02",5)),4,1);
    const auto nonfinite=overflow.take();check(nonfinite&&!nonfinite->raw&&nonfinite->values.empty(),"nonfinite typed conversion never becomes raw plotted value");
    bits.count=2000;check(validate_rtu(bits,error),"maximum coil count accepted");bits.count=2001;check(!validate_rtu(bits,error),"over-limit coil count rejected");
    check(lin_pid(0)==0x80&&lin_pid(0x10)==0x50&&lin_pid(0x3D)==0x7D,"independent protected identifier vectors");
    check(lin_checksum(0x10,std::string("\x01\x02\x03",3),LinChecksum::Classic)==0xF9,"classic checksum known vector");
    check(lin_checksum(0x10,std::string("\x01\x02\x03",3),LinChecksum::Enhanced)==0xA9,"enhanced checksum known vector");
    check(lin_checksum(0x3D,std::string("\xFF\x01",2),LinChecksum::Enhanced)==0xFE,"diagnostic checksum always classic with end-around carry");
    LinFrame frame;frame.id=0x10;frame.pid=0x50;frame.data=std::string("\x01\x02\x03",3);frame.checksum=0xA9;frame.checksum_type=LinChecksum::Enhanced;
    check(validate_lin_frame(frame).chartable(frame),"valid LIN frame may feed chart");
    auto invalid_enum=frame;invalid_enum.break_check=static_cast<LinCheck>(99);check(validate_lin_frame(invalid_enum).malformed,"unknown enum values reject frame");
    rejects([&]{lin_checksum(0,{},static_cast<LinChecksum>(99));},"invalid checksum type rejected");
    auto unknown=frame;unknown.pid=unknown.checksum=-1;check(!validate_lin_frame(unknown).chartable(unknown)&&validate_lin_frame(unknown).chartable(unknown,true),"unknown provenance needs explicit chart permission");
    auto bad=frame;bad.pid^=0x40;check(!validate_lin_frame(bad).chartable(bad,true),"bad PID cannot be enabled by unknown permission");bad=frame;bad.checksum^=1;
    check(!validate_lin_frame(bad).chartable(bad,true),"bad checksum never charts");bad=frame;bad.break_check=LinCheck::Invalid;check(!validate_lin_frame(bad).chartable(bad,true),"reported Break error never charts");
    LinSignal signal;signal.name="signed";signal.id=0x10;signal.start=4;signal.length=12;signal.signed_value=true;signal.scale=0.5;signal.offset=1;
    auto cross=frame;cross.data=std::string("\xF0\xFF",2);const auto values=decode_lin_values(cross,{signal});check(values.size()==1&&values[0].value==0.5,"cross-byte sign extension and conversion known value");
    auto overlap=signal;overlap.name="overlap";check(!validate_lin_signals({signal,overlap},error),"overlapping bits rejected");
    const auto signal_text=encode_lin_signals({signal});check(decode_lin_signals(signal_text)[0].length==12,"manual signal definitions round trip");
    auto bad_text=signal;bad_text.logical.emplace(1,std::string(1,'\xFF'));check(!validate_lin_signals({bad_text},error),"malformed logical UTF-8 rejected before native display");
    const auto trace=encode_lin_trace({frame,unknown,bad});const auto parsed=decode_lin_trace(trace);check(parsed.size()==3&&parsed[1].pid==-1&&parsed[2].break_check==LinCheck::Invalid,"trace retains unknown and invalid source metadata");
    rejects([&]{decode_lin_trace(trace+"garbage\n");},"malformed trace rejected atomically");
    auto backwards=frame;frame.seconds=1;backwards.seconds=0;rejects([&]{encode_lin_trace({frame,backwards});},"backwards time rejected without guessing");
    auto db=parse_lin_ldf(ldf);check(db.signals.size()==2&&db.lengths.at(0x10)==2&&db.baud==19200,"LDF scalar frame and speed parsed");
    auto temperature=frame;temperature.data=std::string("\xE8\x03",2);check(std::abs(decode_lin_values(temperature,db.signals)[0].value-60)<1e-10,"LDF physical encoding independent expected value");
    temperature.data=std::string("\xFF\x0F",2);check(decode_lin_values(temperature,db.signals)[0].text=="Fault","logical encoding outside physical range retained");
    check(db.schedules.size()==1&&!db.schedules[0].slots[0].publish&&db.schedules[0].slots[1].data==std::string(1,'\x01'),"LDF publisher roles and initial payload preserved");
    rejects([&]{parse_lin_ldf(std::string(ldf)+"Event_triggered_frames {}\n");},"unsupported critical LDF section rejected");
    rejects([&]{parse_lin_ldf(std::string(ldf)+"Frames {}\n");},"duplicate LDF section rejected");
    auto bad_ldf=std::string(ldf);bad_ldf.replace(bad_ldf.find("Temp, 0;"),8,"Temp, 8;");rejects([&]{parse_lin_ldf(bad_ldf);},"LDF out-of-frame signal rejected");
    rejects([&]{parse_lin_ldf(std::string(65537,' '));},"LDF byte cap enforced before parse");
    LinScheduler scheduler;FakeLin fake;
    check(!scheduler.start(db.schedules[0],1,nullptr,0,error),"missing adapter is visible and cannot transmit");
    fake.caps.hardware_schedule=false;check(!scheduler.start(db.schedules[0],1,&fake,0,error)&&fake.starts==0,"Windows timers do not impersonate hardware schedule");
    fake.caps.hardware_schedule=true;check(scheduler.start(db.schedules[0],2,&fake,0,error)&&fake.starts==1,"finite hardware program submitted once");
    scheduler.poll(10);check(scheduler.active()&&fake.starts==1,"pending hardware program is not duplicated");++fake.gen;scheduler.poll(11);
    check(!scheduler.active()&&scheduler.draining()&&fake.cancels==1,"generation change cancels but retains pending ownership");
    check(!scheduler.start(db.schedules[0],1,&fake,12,error),"new schedule blocked while cancel drains");
    fake.ready=true;scheduler.poll(13);check(!scheduler.draining()&&!scheduler.active(),"late completion drained without resume");
    auto too_long=db.schedules[0];too_long.slots[0].delay_ms=60000;check(!validate_lin_schedule(too_long,10000,error),"one-hour scheduler cap enforced");
    check(!validate_lin_schedule(db.schedules[0],0,error),"zero cycles rejected");
    std::cout<<"PASS: protocol workspace core checks="<<checks<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<" checks="<<checks<<"\n";return 1;}}
