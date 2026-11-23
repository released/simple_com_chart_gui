#include "modbus_workspace.h"
#include "command_panel.h"
#include "uart_files.h"
#include <charconv>
#include <cmath>
#include <set>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace {
bool integer(std::string_view text,unsigned maximum,unsigned& out) {
    if(text.empty()||text.size()>10)return false;
    unsigned value=0;const auto p=std::from_chars(text.data(),text.data()+text.size(),value);
    if(p.ec!=std::errc{}||p.ptr!=text.data()+text.size()||value>maximum)return false;
    out=value;return true;
}
double number(const std::string& text) {
    double value=0;const auto p=std::from_chars(text.data(),text.data()+text.size(),value);
    if(text.empty()||p.ec!=std::errc{}||p.ptr!=text.data()+text.size()||!std::isfinite(value))throw std::runtime_error("Invalid finite number.");
    return value;
}
unsigned required(const std::string& text,unsigned maximum) {
    unsigned value=0;if(!integer(text,maximum,value))throw std::runtime_error("Invalid decimal integer.");return value;
}
std::string csv(std::string_view value) {
    std::string out="\"";for(const auto c:value){out+=c;if(c=='\"')out+='\"';}return out+'\"';
}
unsigned words(const std::string& type) {return type=="u16"||type=="i16"?1:type=="u32"||type=="i32"||type=="f32"?2:type=="f64"?4:0;}
}
unsigned modbus_reference(unsigned function,unsigned offset) {
    const unsigned bases[]={0,1,10001,40001,30001};return function>=1&&function<=4?bases[function]+offset:0;
}
bool modbus_address(unsigned function,std::string_view input,bool reference,unsigned& output,std::string& error) {
    unsigned value=0;
    if(function<1||function>4||!integer(input,200000,value)){error="Enter an unsigned decimal address.";return false;}
    const auto base=modbus_reference(function,0);
    if(reference){if(value<base||value-base>65535){error="Reference address is outside this function's range.";return false;}value-=base;}
    if(value>65535){error="Wire offset must be 0..65535.";return false;}output=value;return true;
}
std::vector<RtuConfig> modbus_plan(const std::vector<ModbusPoint>& points) {
    std::vector<RtuConfig> result;
    for(const auto& point:points){auto config=point.request;config.fields.clear();
        if(point.type!="raw")config.fields.push_back({point.name,point.type,point.order,0,point.scale,point.offset});
        result.push_back(std::move(config));}
    return result;
}
bool validate_modbus_points(const std::vector<ModbusPoint>& points,std::string& error) {
    if(points.empty()||points.size()>32){error="Define 1..32 Modbus read blocks.";return false;}
    std::set<std::string> names;
    for(const auto& point:points){
        std::vector<BinaryField> checked;std::string detail;
        if(point.name.empty()||point.name.size()>64||point.unit.size()>32||!names.insert(point.name).second||
           !parse_binary_fields(point.name+",0,u16be",checked,detail)||
           !std::isfinite(point.scale)||!std::isfinite(point.offset)) {error="Invalid or duplicate block name, unit or conversion.";return false;}
        const bool bits=point.request.function<=2;
        if(point.type!="raw"&&((bits&&(point.type!="bit"||point.request.count!=1))||
           (!bits&&(!words(point.type)||point.request.count!=words(point.type))))) {
            error="Typed register count must match its type; bit requires count 1. Use raw for a range.";return false;
        }
    }
    for(const auto& config:modbus_plan(points))if(!validate_rtu(config,error))return false;
    try{for(const auto& point:points)(void)uart_wide(point.unit);}catch(...){error="Invalid UTF-8 unit.";return false;}
    return true;
}
std::string encode_modbus_points(const std::vector<ModbusPoint>& points) {
    std::string error;if(!validate_modbus_points(points,error))throw std::runtime_error(error);
    std::ostringstream out;out<<std::setprecision(17)<<"SCC-MODBUS,1\nname,unit_id,function,offset,count,type,order,scale,add,unit,period_ms,timeout_ms\n";
    for(const auto& p:points)out<<csv(p.name)<<','<<p.request.unit<<','<<p.request.function<<','<<p.request.address<<','<<p.request.count<<','<<p.type<<','<<p.order<<','<<p.scale<<','<<p.offset<<','<<csv(p.unit)<<','<<p.request.period_ms<<','<<p.request.timeout_ms<<'\n';
    const auto text=out.str();if(text.size()>65536)throw std::runtime_error("Modbus definitions exceed 64 KiB.");return text;
}
std::vector<ModbusPoint> decode_modbus_points(std::string_view input) {
    if(input.empty()||input.size()>65536||input.find('\0')!=input.npos)throw std::runtime_error("Invalid Modbus definition size.");
    (void)uart_wide(std::string(input));std::vector<std::vector<std::string>> rows;std::string error;
    if(!uart_csv_rows(input,rows,error)||rows.size()<3||rows.size()>34||rows[0]!=std::vector<std::string>{"SCC-MODBUS","1"}||
       rows[1]!=std::vector<std::string>{"name","unit_id","function","offset","count","type","order","scale","add","unit","period_ms","timeout_ms"})throw std::runtime_error("Unsupported Modbus definition header.");
    std::vector<ModbusPoint> points;
    for(size_t n=2;n<rows.size();++n){const auto& r=rows[n];if(r.size()!=12)throw std::runtime_error("Expected 12 Modbus columns.");
        ModbusPoint p;p.name=r[0];p.request.unit=required(r[1],247);p.request.function=required(r[2],4);p.request.address=required(r[3],65535);p.request.count=required(r[4],2000);
        p.type=r[5];p.order=r[6];p.scale=number(r[7]);p.offset=number(r[8]);p.unit=r[9];p.request.period_ms=required(r[10],60000);p.request.timeout_ms=required(r[11],60000);points.push_back(std::move(p));}
    if(!validate_modbus_points(points,error))throw std::runtime_error(error);return points;
}
const char* rtu_outcome_name(RtuOutcome value) {
    switch(value){case RtuOutcome::Pending:return "Pending";case RtuOutcome::Success:return "Success";
    case RtuOutcome::Exception:return "Exception";case RtuOutcome::Timeout:return "Timeout";
    case RtuOutcome::SendFailure:return "Send failed / partial";default:return "Stopped";}
}
std::string modbus_transactions_csv(const std::deque<RtuTransaction>& rows) {
    std::ostringstream out;out<<"sequence,unit,function,offset,count,outcome,exception,observed_ms,request_hex,response_hex\n";
    for(const auto& r:rows)out<<r.sequence<<','<<r.unit<<','<<r.function<<','<<r.address<<','<<r.count<<','<<rtu_outcome_name(r.outcome)<<','<<r.exception<<','<<(r.finished_ms>=r.started_ms?r.finished_ms-r.started_ms:0)<<','<<uart_hex(r.request)<<','<<uart_hex(r.response)<<'\n';
    return out.str();
}
