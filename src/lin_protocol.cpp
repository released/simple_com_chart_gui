#include "lin_protocol.h"
#include "command_panel.h"
#include "uart_files.h"
#include "binary_frame.h"
#include <charconv>
#include <cmath>
#include <set>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace {
unsigned integer(std::string_view text,unsigned maximum,int base=10) {
    unsigned value=0;const auto r=std::from_chars(text.data(),text.data()+text.size(),value,base);
    if(text.empty()||text.size()>10||r.ec!=std::errc{}||r.ptr!=text.data()+text.size()||value>maximum)
        throw std::runtime_error("Invalid LIN integer or range.");
    return value;
}
double number(const std::string& text) {
    double value=0;const auto r=std::from_chars(text.data(),text.data()+text.size(),value);
    if(text.empty()||r.ec!=std::errc{}||r.ptr!=text.data()+text.size()||!std::isfinite(value))throw std::runtime_error("Invalid finite LIN number.");return value;
}
std::string quote(std::string_view text) {std::string result="\"";for(const auto c:text){result+=c;if(c=='\"')result+=c;}return result+'\"';}
std::string hex(unsigned value) {const char digits[]="0123456789ABCDEF";return {digits[(value>>4)&15],digits[value&15]};}
std::string bytes(std::string_view input) {
    if(input.size()>64)throw std::runtime_error("LIN data exceeds eight bytes.");
    std::string clean;for(const unsigned char c:input)if(c!=' '&&c!='\t')clean+=static_cast<char>(c);
    if(clean.size()>16||clean.size()%2)throw std::runtime_error("LIN data requires complete HEX bytes.");
    std::string result;for(size_t at=0;at<clean.size();at+=2)result+=static_cast<char>(integer(std::string_view(clean).substr(at,2),255,16));return result;
}
std::vector<std::vector<std::string>> rows(std::string_view input,size_t maximum) {
    if(input.empty()||input.size()>maximum||input.find('\0')!=input.npos)throw std::runtime_error("Invalid LIN file size or NUL.");
    (void)uart_wide(std::string(input));std::vector<std::vector<std::string>> out;std::string error;
    if(!uart_csv_table(input,out,error,maximum,10002,8))throw std::runtime_error(error);return out;
}
}
unsigned lin_pid(unsigned id) {
    if(id>63)throw std::runtime_error("LIN ID must be 00..3F HEX.");
    const auto bit=[&](unsigned n){return (id>>n)&1;};
    return id|((bit(0)^bit(1)^bit(2)^bit(4))<<6)|((1^(bit(1)^bit(3)^bit(4)^bit(5)))<<7);
}
unsigned lin_checksum(unsigned id,std::string_view data,LinChecksum type) {
    if(data.size()>8||id>63||(type!=LinChecksum::Classic&&type!=LinChecksum::Enhanced))throw std::runtime_error("LIN checksum needs ID, bytes and an explicit type.");
    unsigned sum=type==LinChecksum::Enhanced&&id!=60&&id!=61?lin_pid(id):0;
    for(const unsigned char byte:data){sum+=byte;if(sum>255)sum-=255;}return (~sum)&255;
}
LinValidation validate_lin_frame(const LinFrame& frame) {
    LinValidation result;
    if(frame.id>63||frame.data.size()>8||frame.pid< -1||frame.pid>255||frame.checksum< -1||frame.checksum>255||
       !std::isfinite(frame.seconds)||frame.seconds<0||frame.seconds>1e12||
       (frame.direction!="RX"&&frame.direction!="TX"&&frame.direction!="Unknown")||
       (frame.checksum_type!=LinChecksum::Unknown&&frame.checksum_type!=LinChecksum::Classic&&frame.checksum_type!=LinChecksum::Enhanced)||
       (frame.break_check!=LinCheck::Unknown&&frame.break_check!=LinCheck::Valid&&frame.break_check!=LinCheck::Invalid)) {
        result.malformed=true;result.detail="Invalid LIN frame fields.";return result;
    }
    if(frame.pid>=0)result.pid=static_cast<unsigned>(frame.pid)==lin_pid(frame.id)?LinCheck::Valid:LinCheck::Invalid;
    if(frame.checksum>=0&&!frame.data.empty()&&frame.checksum_type!=LinChecksum::Unknown) {
        const bool diagnostic=frame.id==60||frame.id==61;
        result.checksum=diagnostic&&frame.checksum_type==LinChecksum::Enhanced?LinCheck::Invalid:
            static_cast<unsigned>(frame.checksum)==lin_checksum(frame.id,frame.data,frame.checksum_type)?LinCheck::Valid:LinCheck::Invalid;
    }
    result.detail=frame.data.empty()?"Header only":result.pid==LinCheck::Invalid?"PID parity mismatch":
        result.checksum==LinCheck::Invalid?"Checksum mismatch":frame.break_check==LinCheck::Invalid?"Reported Break error":
        result.pid==LinCheck::Unknown||result.checksum==LinCheck::Unknown?"Unverified metadata":"PID and checksum valid";
    return result;
}
bool LinValidation::chartable(const LinFrame& frame,bool allow_unknown)const {
    return !malformed&&!frame.data.empty()&&pid!=LinCheck::Invalid&&checksum!=LinCheck::Invalid&&frame.break_check!=LinCheck::Invalid&&
        (allow_unknown||(pid==LinCheck::Valid&&checksum==LinCheck::Valid));
}
bool validate_lin_signals(const std::vector<LinSignal>& signals,std::string& error) {
    if(signals.size()>256){error="At most 256 LIN signals.";return false;}
    std::set<std::string> names;std::map<unsigned,uint64_t> occupied;
    for(const auto& s:signals){std::vector<BinaryField> field;std::string detail;
        if(s.id>63||s.length<1||s.length>32||s.start>=64||s.start+s.length>64||s.name.empty()||s.name.size()>64||s.unit.size()>32||
           !std::isfinite(s.scale)||!std::isfinite(s.offset)||!names.insert(std::to_string(s.id)+"/"+s.name).second||
           !parse_binary_fields(s.name+",0,u16be",field,detail)||s.logical.size()>128) {error="Invalid or duplicate LIN signal definition.";return false;}
        const auto maximum=(uint64_t(1)<<s.length)-1,mask=maximum<<s.start;
        if((occupied[s.id]&mask)||(s.minimum&&*s.minimum>maximum)||(s.maximum&&*s.maximum>maximum)||
           (s.minimum&&s.maximum&&*s.minimum>*s.maximum)){error="Overlapping LIN signals or invalid raw range.";return false;}
        occupied[s.id]|=mask;
        try{(void)uart_wide(s.unit);for(const auto& label:s.logical){(void)uart_wide(label.second);if(label.first>maximum||label.second.size()>128){error="Invalid LIN logical encoding.";return false;}}}
        catch(...){error="Invalid UTF-8 LIN unit.";return false;}
    }
    return true;
}
std::vector<LinValue> decode_lin_values(const LinFrame& frame,const std::vector<LinSignal>& signals) {
    std::vector<LinValue> values;uint64_t packed=0;
    for(size_t n=0;n<frame.data.size()&&n<8;++n)packed|=static_cast<uint64_t>(static_cast<unsigned char>(frame.data[n]))<<(n*8);
    for(const auto& signal:signals)if(signal.id==frame.id&&signal.start<64&&signal.length>=1&&signal.length<=32&&signal.start+signal.length<=frame.data.size()*8){
        const auto raw=static_cast<uint32_t>((packed>>signal.start)&((uint64_t(1)<<signal.length)-1));
        const auto text=signal.logical.find(raw);const bool logical=text!=signal.logical.end();
        if(!logical&&((signal.minimum&&raw<*signal.minimum)||(signal.maximum&&raw>*signal.maximum)))continue;
        const int64_t signed_raw=signal.signed_value&&(uint64_t(raw)&(uint64_t(1)<<(signal.length-1)))?
            static_cast<int64_t>(raw)-static_cast<int64_t>(uint64_t(1)<<signal.length):static_cast<int64_t>(raw);
        const double value=logical?static_cast<double>(raw):static_cast<double>(signed_raw)*signal.scale+signal.offset;
        if(!std::isfinite(value))continue;LinValue out;out.key="LIN."+hex(frame.id)+"."+signal.name;out.unit=logical?"":signal.unit;out.value=value;out.raw=raw;
        if(logical)out.text=text->second;values.push_back(std::move(out));
    }
    return values;
}
const char* lin_check_name(LinCheck check) {return check==LinCheck::Valid?"Valid":check==LinCheck::Invalid?"Invalid":"Unknown";}
std::vector<LinFrame> decode_lin_trace(std::string_view input) {
    const auto parsed=rows(input,2*1024*1024);
    if(parsed.size()<3||parsed.size()>10002||parsed[0]!=std::vector<std::string>{"SCC-LIN-TRACE","1"}||
       parsed[1]!=std::vector<std::string>{"seconds","id","pid","data","checksum","checksum_type","direction","break"})throw std::runtime_error("Unsupported LIN trace header or record limit.");
    std::vector<LinFrame> result;double last=0;
    for(size_t n=2;n<parsed.size();++n){const auto& r=parsed[n];if(r.size()!=8)throw std::runtime_error("Expected eight LIN trace columns.");
        LinFrame f;f.seconds=number(r[0]);f.id=integer(r[1],63,16);f.pid=r[2].empty()?-1:static_cast<int>(integer(r[2],255,16));
        f.data=bytes(r[3]);f.checksum=r[4].empty()?-1:static_cast<int>(integer(r[4],255,16));
        if(r[5]!="Unknown"&&r[5]!="Classic"&&r[5]!="Enhanced")throw std::runtime_error("Unknown LIN checksum type.");
        f.checksum_type=r[5]=="Classic"?LinChecksum::Classic:r[5]=="Enhanced"?LinChecksum::Enhanced:LinChecksum::Unknown;f.direction=r[6];
        if(r[7]!="Unknown"&&r[7]!="Valid"&&r[7]!="Invalid")throw std::runtime_error("Invalid LIN Break provenance.");
        f.break_check=r[7]=="Valid"?LinCheck::Valid:r[7]=="Invalid"?LinCheck::Invalid:LinCheck::Unknown;
        if(validate_lin_frame(f).malformed||f.seconds<last)throw std::runtime_error("Invalid or backwards LIN source time.");last=f.seconds;result.push_back(std::move(f));
    }
    return result;
}
std::string encode_lin_trace(const std::vector<LinFrame>& frames) {
    if(frames.empty()||frames.size()>10000)throw std::runtime_error("LIN trace requires 1..10000 records.");
    std::ostringstream out;out<<std::setprecision(17)<<"SCC-LIN-TRACE,1\nseconds,id,pid,data,checksum,checksum_type,direction,break\n";double last=0;
    for(const auto& f:frames){if(validate_lin_frame(f).malformed||f.seconds<last)throw std::runtime_error("Invalid or backwards LIN source time.");last=f.seconds;
        out<<f.seconds<<','<<hex(f.id)<<','<<(f.pid<0?"":hex(static_cast<unsigned>(f.pid)))<<','<<uart_hex(f.data)<<','<<(f.checksum<0?"":hex(static_cast<unsigned>(f.checksum)))<<','
           <<(f.checksum_type==LinChecksum::Classic?"Classic":f.checksum_type==LinChecksum::Enhanced?"Enhanced":"Unknown")<<','<<f.direction<<','<<lin_check_name(f.break_check)<<'\n';}
    return out.str();
}
std::vector<LinSignal> decode_lin_signals(std::string_view input) {
    const auto parsed=rows(input,65536);
    if(parsed.size()<2||parsed.size()>258||parsed[0]!=std::vector<std::string>{"SCC-LIN-SIGNALS","1"}||
       parsed[1]!=std::vector<std::string>{"name","id","start_bit","bits","signed","scale","add","unit"})throw std::runtime_error("Unsupported LIN signal header.");
    std::vector<LinSignal> result;
    for(size_t n=2;n<parsed.size();++n){const auto& r=parsed[n];if(r.size()!=8)throw std::runtime_error("Expected eight LIN signal columns.");
        LinSignal s;s.name=r[0];s.id=integer(r[1],63,16);s.start=integer(r[2],63);s.length=integer(r[3],32);s.signed_value=integer(r[4],1)!=0;s.scale=number(r[5]);s.offset=number(r[6]);s.unit=r[7];result.push_back(std::move(s));}
    std::string error;if(!validate_lin_signals(result,error))throw std::runtime_error(error);return result;
}
std::string encode_lin_signals(const std::vector<LinSignal>& signals) {
    std::string error;if(!validate_lin_signals(signals,error))throw std::runtime_error(error);
    std::ostringstream out;out<<std::setprecision(17)<<"SCC-LIN-SIGNALS,1\nname,id,start_bit,bits,signed,scale,add,unit\n";
    for(const auto& s:signals){if(s.minimum||s.maximum||!s.logical.empty())throw std::runtime_error("LDF encoding metadata requires retaining the original LDF.");
        out<<quote(s.name)<<','<<hex(s.id)<<','<<s.start<<','<<s.length<<','<<s.signed_value<<','<<s.scale<<','<<s.offset<<','<<quote(s.unit)<<'\n';}
    return out.str();
}
