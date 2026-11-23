#include "modbus_rtu.h"
#include "command_panel.h"
#include <charconv>
#include <cmath>
#include <set>
#include <algorithm>
namespace {
size_t width(const std::string& type){return type=="u16"||type=="i16"?2:type=="u32"||type=="i32"||type=="f32"?4:type=="f64"?8:0;}
bool scalar(std::string_view s,double& out){const auto r=std::from_chars(s.data(),s.data()+s.size(),out);return r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(out);}
std::string crc(std::string data){const auto value=frame_checksum(data,FrameChecksum::Modbus16);data.push_back(static_cast<char>(value));data.push_back(static_cast<char>(value>>8));return data;}
bool crc_ok(std::string_view bytes){return bytes.size()>=3&&frame_checksum(bytes.substr(0,bytes.size()-2),FrameChecksum::Modbus16)==static_cast<uint32_t>(static_cast<unsigned char>(bytes[bytes.size()-2])|(static_cast<unsigned char>(bytes.back())<<8));}
}
bool parse_rtu_fields(std::string_view text,std::vector<RtuField>& fields,std::string& error){std::vector<std::vector<std::string>> rows;if(!uart_csv_rows(text,rows,error))return false;std::vector<RtuField> next;for(const auto& row:rows){if(row.size()<3||row.size()>6){error="Fields: name,word_offset,type[,ABCD/CDAB/BADC/DCBA[,scale[,add]]].";return false;}RtuField field;field.name=row[0];field.type=row[2];double offset=0;if(!scalar(row[1],offset)||offset<0||offset>124||offset!=std::floor(offset)){error="Register offset must be an integer 0..124.";return false;}field.reg=static_cast<size_t>(offset);if(row.size()>3)field.order=row[3];if((row.size()>4&&!scalar(row[4],field.scale))||(row.size()>5&&!scalar(row[5],field.add))){error="Scale/add must be finite.";return false;}next.push_back(std::move(field));}fields=std::move(next);return true;}
bool validate_rtu(const RtuConfig& c,std::string& error) {
    const bool bits=c.function==1||c.function==2;
    const auto maximum=bits?2000U:125U;
    if(c.unit<1||c.unit>247||c.function<1||c.function>4||c.address>65535||
       c.count<1||c.count>maximum||c.address+c.count>65536||c.period_ms<100||
       c.period_ms>60000||c.timeout_ms<100||c.timeout_ms>60000||
       c.idle_ms<2||c.idle_ms>1000||c.fields.size()>64) {
        error="Unit 1..247; read function 01..04; offset 0..65535; bits 1..2000 or registers 1..125; interval/timeout 100..60000 ms; max 64 fields.";return false;
    }
    std::set<std::string> names;
    for(const auto& f:c.fields) {
        std::vector<BinaryField> check;std::string detail;
        const bool valid_layout=bits?f.type=="bit"&&f.reg<c.count:
            width(f.type)&&f.reg+width(f.type)/2<=c.count;
        if(!valid_layout||(f.order!="ABCD"&&f.order!="CDAB"&&f.order!="BADC"&&f.order!="DCBA")||
           !std::isfinite(f.scale)||!std::isfinite(f.add)||!names.insert(f.name).second||
           !parse_binary_fields(f.name+",0,u16be",check,detail)) {
            error="Invalid/duplicate RTU field, type, order or range.";return false;
        }
    }
    return true;
}
std::string rtu_request(const RtuConfig& c) {
    std::string out;out.push_back(static_cast<char>(c.unit));out.push_back(static_cast<char>(c.function));
    out.push_back(static_cast<char>(c.address>>8));out.push_back(static_cast<char>(c.address));
    out.push_back(static_cast<char>(c.count>>8));out.push_back(static_cast<char>(c.count));return crc(std::move(out));
}
bool RtuPoller::start(RtuConfig config,uint64_t generation,uint64_t now,std::string& error) {
    return start_plan({std::move(config)},generation,now,error);
}
bool RtuPoller::start_plan(std::vector<RtuConfig> plan,uint64_t generation,uint64_t now,std::string& error) {
    if(active_){error="Stop the current poller first.";return false;}
    if(plan.empty()||plan.size()>32){error="Read plan requires 1..32 blocks.";return false;}
    for(const auto& config:plan)if(!validate_rtu(config,error))return false;
    plan_=std::move(plan);due_times_.assign(plan_.size(),now);config_=plan_.front();
    generation_=generation;block_=next_block_=0;observed_ms_=last_rx_=now;waiting_=sending_=false;
    active_=true;pending_.clear();result_.reset();errors_=0;transactions_.clear();diagnostics_={};
    ++revision_;status_="Polling ready; FC01..04 read-only";return true;
}
void RtuPoller::end_transaction(RtuOutcome outcome,uint64_t now,unsigned exception) {
    if(transactions_.empty()||transactions_.back().outcome!=RtuOutcome::Pending)return;
    auto& row=transactions_.back();row.outcome=outcome;row.finished_ms=std::max(now,row.started_ms);row.exception=exception;
    if(pending_.size()<=512)row.response=pending_;++revision_;
}
void RtuPoller::stop(std::string why) {
    if(active_&&(sending_||waiting_))end_transaction(RtuOutcome::Stopped,observed_ms_);
    active_=waiting_=sending_=false;status_=std::move(why);++revision_;
}
bool RtuPoller::next(uint64_t now,std::string& bytes) {
    tick(now);
    if(!active_||waiting_||sending_||result_)return false;
    bool selected=false;
    for(size_t n=0;n<plan_.size();++n){const auto at=(next_block_+n)%plan_.size();
        if(now>=due_times_[at]&&now>=last_rx_&&now-last_rx_>=plan_[at].idle_ms){block_=at;selected=true;break;}}
    if(!selected)return false;
    config_=plan_[block_];next_block_=(block_+1)%plan_.size();pending_.clear();
    bytes=rtu_request(config_);sending_=true;status_="RTU request pending";
    RtuTransaction row;row.sequence=++diagnostics_.requests;row.started_ms=now;row.block=block_;
    row.unit=config_.unit;row.function=config_.function;row.address=config_.address;row.count=config_.count;row.request=bytes;
    transactions_.push_back(std::move(row));if(transactions_.size()>128)transactions_.pop_front();++revision_;return true;
}
void RtuPoller::complete(bool ok,uint64_t now) {
    observed_ms_=std::max(observed_ms_,now);
    if(!active_||!sending_)return;sending_=false;
    if(!ok){end_transaction(RtuOutcome::SendFailure,now);stop("STOPPED: RTU write failed/partial; no retry");return;}
    waiting_=true;deadline_=now+config_.timeout_ms;status_="RTU waiting for matching CRC response";parse(now);
}
void RtuPoller::receive(std::string_view bytes,uint64_t now,double seconds) {
    last_rx_=now;seconds_=seconds;tick(now);if(!active_||(!sending_&&!waiting_))return;
    if(bytes.size()>512||pending_.size()+bytes.size()>512){stop("STOPPED: RTU response exceeds bounded buffer");return;}
    pending_.append(bytes);if(waiting_)parse(now);
}
void RtuPoller::parse(uint64_t now) {
    size_t at=0;const bool bits=config_.function<=2;
    const auto expected=bits?(config_.count+7)/8:config_.count*2;
    while(waiting_&&at+3<=pending_.size()) {
        const auto unit=static_cast<unsigned char>(pending_[at]);
        const auto function=static_cast<unsigned char>(pending_[at+1]);
        const auto count=static_cast<unsigned char>(pending_[at+2]);
        if(unit!=config_.unit||(function!=config_.function&&function!=(config_.function|128))){++at;++diagnostics_.unmatched_bytes;continue;}
        const bool exception=(function&128)!=0;const size_t length=exception?5:static_cast<size_t>(count)+5;
        if(!exception&&count!=expected){++at;++errors_;++diagnostics_.length_errors;continue;}
        if(pending_.size()-at<length)break;const auto frame=std::string_view(pending_).substr(at,length);
        if(!crc_ok(frame)){++at;++errors_;++diagnostics_.crc_errors;continue;}
        if(exception){end_transaction(RtuOutcome::Exception,now,count);stop("Modbus exception "+std::to_string(count)+"; polling stopped");return;}
        RtuResult out;out.generation=generation_;out.raw=config_.fields.empty();out.seconds=seconds_;out.unit=config_.unit;out.function=config_.function;
        out.address=config_.address;out.block=block_;out.observed_ms=now;const auto data=frame.substr(3,count);
        if(bits){for(unsigned n=0;n<config_.count;++n)out.registers.push_back((static_cast<unsigned char>(data[n/8])>>(n%8))&1);}
        else for(size_t n=0;n<data.size();n+=2)out.registers.push_back((static_cast<unsigned char>(data[n])<<8)|static_cast<unsigned char>(data[n+1]));
        for(const auto& f:config_.fields) {
            if(bits){const double value=out.registers.at(f.reg)*f.scale+f.add;if(std::isfinite(value))out.values.push_back({f.name,value});continue;}
            const auto n=width(f.type);std::string ordered(data.substr(f.reg*2,n));
            if(f.order=="CDAB"||f.order=="DCBA")for(size_t i=0;i<n/2;++i)if(i<n/2-1-i){std::swap(ordered[i*2],ordered[(n/2-1-i)*2]);std::swap(ordered[i*2+1],ordered[(n/2-1-i)*2+1]);}
            if(f.order=="BADC"||f.order=="DCBA")for(size_t i=0;i<n;i+=2)std::swap(ordered[i],ordered[i+1]);
            BinaryFrameConfig config;config.fixed_bytes=n;config.fields={{f.name,f.type+"be",0,f.scale,f.add}};
            BinaryFrameDecoder decoder;std::string error;decoder.configure(config,error);const auto frames=decoder.feed(ordered,0,seconds_);
            if(!frames.empty())for(const auto& value:frames.front().values)out.values.push_back(value);
        }
        out.bytes=std::string(frame);result_=std::move(out);end_transaction(RtuOutcome::Success,now);
        if(!transactions_.empty())transactions_.back().response=std::string(frame);
        ++diagnostics_.responses;waiting_=false;due_times_[block_]=now+config_.period_ms;
        status_="RTU response OK; "+std::to_string(config_.count)+(bits?" bits":" registers")+" | CRC/layout errors "+std::to_string(errors_);
        pending_.clear();++revision_;return;
    }
    if(at)pending_.erase(0,at);
}
void RtuPoller::tick(uint64_t now) {
    observed_ms_=std::max(observed_ms_,now);
    if(active_&&waiting_&&now>=deadline_){end_transaction(RtuOutcome::Timeout,now);stop("TIMEOUT: polling stopped; no automatic retry of ambiguous RTU response");}
}
