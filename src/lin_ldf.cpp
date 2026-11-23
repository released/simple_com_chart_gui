#include "lin_protocol.h"
#include "uart_files.h"
#include <charconv>
#include <cctype>
#include <cmath>
#include <set>
#include <stdexcept>

namespace {
struct Token {std::string text;bool quoted=false;};
std::vector<Token> lex(std::string_view input) {
    if(input.empty()||input.size()>65536||input.find('\0')!=input.npos)throw std::runtime_error("LDF must be UTF-8 without NUL, at most 64 KiB.");
    (void)uart_wide(input);std::vector<Token> tokens;
    for(size_t at=0;at<input.size();) {
        const auto c=static_cast<unsigned char>(input[at]);if(std::isspace(c)){++at;continue;}
        if(input.substr(at,2)=="//"){const auto end=input.find('\n',at+2);at=end==input.npos?input.size():end+1;continue;}
        if(input.substr(at,2)=="/*"){const auto end=input.find("*/",at+2);if(end==input.npos)throw std::runtime_error("Unclosed LDF comment.");at=end+2;continue;}
        Token token;
        if(c=='\"') {
            token.quoted=true;++at;bool closed=false;
            while(at<input.size()){char next=input[at++];if(next=='\"'){closed=true;break;}
                if(next=='\\'){if(at==input.size())break;next=input[at++];if(next!='\\'&&next!='\"')throw std::runtime_error("Unsupported LDF string escape.");}
                if(static_cast<unsigned char>(next)<32)throw std::runtime_error("Control character in LDF string.");token.text+=next;
                if(token.text.size()>256)throw std::runtime_error("LDF string exceeds 256 bytes.");}
            if(!closed)throw std::runtime_error("Unclosed LDF string.");
        }else if(std::string_view("{}:;,=[]").find(static_cast<char>(c))!=std::string_view::npos){token.text+=static_cast<char>(c);++at;}
        else {
            const auto first=at;
            while(at<input.size()){const auto value=static_cast<unsigned char>(input[at]);if(!(std::isalnum(value)||value=='_'||value=='.'||value=='+'||value=='-'))break;++at;}
            if(at==first||at-first>128)throw std::runtime_error("Invalid LDF token.");token.text=std::string(input.substr(first,at-first));
        }
        tokens.push_back(std::move(token));if(tokens.size()>16384)throw std::runtime_error("LDF token limit exceeded.");
    }
    return tokens;
}
class Reader {
public:
    explicit Reader(std::string_view text):tokens_(lex(text)){}
    bool empty()const{return at_==tokens_.size();}
    bool eat(const char* value){if(!empty()&&!tokens_[at_].quoted&&tokens_[at_].text==value){++at_;return true;}return false;}
    void need(const char* value){if(!eat(value))throw std::runtime_error(std::string("Expected LDF token: ")+value);}
    std::string word(){if(empty()||tokens_[at_].quoted)throw std::runtime_error("Expected LDF identifier.");const auto out=tokens_[at_++].text;
        if(out.empty()||!(std::isalpha(static_cast<unsigned char>(out[0]))||out[0]=='_'))throw std::runtime_error("Invalid LDF identifier.");
        for(const unsigned char c:out)if(!(std::isalnum(c)||c=='_'))throw std::runtime_error("Invalid LDF identifier.");return out;}
    std::string text(){if(empty())throw std::runtime_error("Unexpected LDF end.");return tokens_[at_++].text;}
    std::string quoted(){if(empty()||!tokens_[at_].quoted)throw std::runtime_error("Expected quoted LDF text.");return tokens_[at_++].text;}
    double number(){const auto value=text();double out=0;const auto r=std::from_chars(value.data(),value.data()+value.size(),out);
        if(r.ec!=std::errc{}||r.ptr!=value.data()+value.size()||!std::isfinite(out))throw std::runtime_error("Invalid LDF number.");return out;}
    uint32_t integer(uint32_t maximum=UINT32_MAX){auto value=text();int base=10;if(value.rfind("0x",0)==0||value.rfind("0X",0)==0){base=16;value.erase(0,2);}uint32_t out=0;const auto r=std::from_chars(value.data(),value.data()+value.size(),out,base);
        if(value.empty()||r.ec!=std::errc{}||r.ptr!=value.data()+value.size()||out>maximum)throw std::runtime_error("Invalid LDF integer.");return out;}
    void ignored_group(){need("{");unsigned depth=1;while(depth){if(empty())throw std::runtime_error("Unclosed LDF group.");if(eat("{")){if(++depth>16)throw std::runtime_error("LDF nesting exceeds 16.");}else if(eat("}"))--depth;else ++at_;}}
private:
    std::vector<Token> tokens_;size_t at_=0;
};
struct BaseSignal {unsigned bits=0;uint32_t initial=0;std::string publisher;};
struct Encoding {std::optional<uint32_t> minimum,maximum;double scale=1,offset=0;std::string unit;std::map<uint32_t,std::string> logical;};
struct Frame {unsigned id=0,length=0;std::string publisher;std::string initial;};
}
LinDatabase parse_lin_ldf(std::string_view text) {
    Reader in(text);LinDatabase out;std::map<std::string,BaseSignal> base;
    std::map<std::string,Frame> frames;std::map<std::string,Encoding> encodings;
    std::map<std::string,std::string> representations;std::set<std::string> sections,nodes;
    struct ScheduleNames {std::string name;std::vector<std::pair<std::string,unsigned>> slots;};std::vector<ScheduleNames> schedules;
    in.need("LIN_description_file");in.need(";");
    while(!in.empty()) {
        const auto section=in.word();if(!sections.insert(section).second)throw std::runtime_error("Duplicate LDF section.");
        if(section=="LIN_protocol_version"||section=="LIN_language_version") {
            in.need("=");const auto version=in.quoted();in.need(";");
            if(version!="1.3"&&version!="2.0"&&version!="2.1"&&version!="2.2")throw std::runtime_error("Supported LDF versions: 1.3, 2.0, 2.1, 2.2.");
            if(section=="LIN_protocol_version")out.version=version;
        }else if(section=="LIN_speed") {
            in.need("=");const auto speed=in.number()*1000;in.need("kbps");in.need(";");
            if(speed<1000||speed>20000||speed!=std::floor(speed))throw std::runtime_error("LDF baud must be 1000..20000.");out.baud=static_cast<unsigned>(speed);
        }else if(section=="Nodes") {
            in.need("{");in.need("Master");in.need(":");out.master=in.word();nodes.insert(out.master);
            in.need(",");const auto base_time=in.number();in.need("ms");in.need(",");const auto jitter=in.number();in.need("ms");in.need(";");
            if(base_time<=0||base_time>60000||jitter<0||jitter>base_time)throw std::runtime_error("Invalid LDF master timing metadata.");
            in.need("Slaves");in.need(":");do {if(!nodes.insert(in.word()).second||nodes.size()>64)throw std::runtime_error("Duplicate LDF node or node limit.");}while(in.eat(","));in.need(";");in.need("}");
        }else if(section=="Signals") {
            in.need("{");while(!in.eat("}")) {const auto name=in.word();in.need(":");BaseSignal signal;signal.bits=in.integer(32);in.need(",");signal.initial=in.integer();in.need(",");signal.publisher=in.word();
                while(in.eat(","))(void)in.word();in.need(";");
                if(!signal.bits||signal.initial>((uint64_t(1)<<signal.bits)-1)||base.size()>=256||!base.emplace(name,signal).second)throw std::runtime_error("Invalid or duplicate scalar LDF signal.");}
        }else if(section=="Frames") {
            in.need("{");while(!in.eat("}")){const auto name=in.word();in.need(":");Frame frame;frame.id=in.integer(59);in.need(",");frame.publisher=in.word();in.need(",");frame.length=in.integer(8);in.need("{");
                if(!frame.length||frames.size()>=60||out.lengths.count(frame.id))throw std::runtime_error("Duplicate LDF frame ID or invalid length.");
                uint64_t initial=0;while(!in.eat("}")){const auto signal_name=in.word();in.need(",");const auto start=in.integer(63);in.need(";");const auto found=base.find(signal_name);
                    if(found==base.end()||start+found->second.bits>frame.length*8||out.signals.size()>=256||found->second.publisher!=frame.publisher)throw std::runtime_error("LDF signal is missing, outside its frame or has a different publisher.");
                    LinSignal signal;signal.name=signal_name;signal.id=frame.id;signal.start=start;signal.length=found->second.bits;out.signals.push_back(signal);
                    initial|=uint64_t(found->second.initial)<<start;}
                for(unsigned byte=0;byte<frame.length;++byte)frame.initial+=static_cast<char>(initial>>(8*byte));
                out.lengths.emplace(frame.id,frame.length);out.frame_names.emplace(frame.id,name);
                if(!frames.emplace(name,std::move(frame)).second)throw std::runtime_error("Duplicate LDF frame name.");}
        }else if(section=="Signal_encoding_types") {
            in.need("{");while(!in.eat("}")){const auto name=in.word();in.need("{");Encoding encoding;
                while(!in.eat("}")){const auto kind=in.word();in.need(",");
                    if(kind=="physical_value"){if(encoding.minimum)throw std::runtime_error("Only one physical LDF range is supported.");encoding.minimum=in.integer();in.need(",");encoding.maximum=in.integer();in.need(",");encoding.scale=in.number();in.need(",");encoding.offset=in.number();if(in.eat(","))encoding.unit=in.quoted();in.need(";");}
                    else if(kind=="logical_value"){const auto raw=in.integer();in.need(",");const auto label=in.quoted();in.need(";");if(encoding.logical.size()>=128||!encoding.logical.emplace(raw,label).second)throw std::runtime_error("Duplicate LDF logical value or encoding limit.");}
                    else throw std::runtime_error("Unsupported LDF encoding; only physical/logical values are accepted.");}
                if(encodings.size()>=256||!encodings.emplace(name,std::move(encoding)).second)throw std::runtime_error("Duplicate LDF encoding or limit.");}
        }else if(section=="Signal_representation") {
            in.need("{");while(!in.eat("}")){const auto encoding=in.word();in.need(":");do{if(!representations.emplace(in.word(),encoding).second)throw std::runtime_error("Duplicate LDF signal representation.");}while(in.eat(","));in.need(";");}
        }else if(section=="Schedule_tables") {
            in.need("{");while(!in.eat("}")){ScheduleNames schedule;schedule.name=in.word();in.need("{");
                while(!in.eat("}")){const auto frame=in.word();in.need("delay");const auto delay=in.number();in.need("ms");in.need(";");
                    if(delay<1||delay>60000||delay!=std::floor(delay)||schedule.slots.size()>=256)throw std::runtime_error("LDF slots require whole milliseconds, 1..60000, max 256 slots.");schedule.slots.push_back({frame,static_cast<unsigned>(delay)});}
                if(schedules.size()>=16)throw std::runtime_error("At most 16 LDF schedules.");schedules.push_back(std::move(schedule));}
        }else if(section=="Node_attributes"||section=="Node_composition") {
            in.ignored_group();out.notes.push_back(section+" retained only as source metadata; node configuration is not applied");
        }else throw std::runtime_error("Unsupported LDF section: "+section+". Unconditional scalar analysis only.");
    }
    if(out.version.empty()||out.master.empty()||frames.empty())throw std::runtime_error("LDF requires protocol version, nodes and unconditional frames.");
    for(const auto& signal:base)if(!nodes.count(signal.second.publisher))throw std::runtime_error("LDF signal publisher is not a declared node.");
    for(const auto& frame:frames)if(!nodes.count(frame.second.publisher))throw std::runtime_error("LDF frame publisher is not a declared node.");
    for(auto& signal:out.signals){const auto rep=representations.find(signal.name);if(rep==representations.end())continue;const auto encoding=encodings.find(rep->second);
        if(encoding==encodings.end())throw std::runtime_error("Unknown LDF signal encoding.");const auto& e=encoding->second;signal.minimum=e.minimum;signal.maximum=e.maximum;signal.scale=e.scale;signal.offset=e.offset;signal.unit=e.unit;signal.logical=e.logical;}
    for(const auto& representation:representations)if(!base.count(representation.first)||!encodings.count(representation.second))throw std::runtime_error("Unknown LDF signal representation.");
    std::string error;if(!validate_lin_signals(out.signals,error))throw std::runtime_error(error);
    std::set<std::string> schedule_names;size_t total_slots=0;
    for(const auto& source:schedules){LinSchedule schedule;schedule.name=source.name;if(!schedule_names.insert(source.name).second)throw std::runtime_error("Duplicate LDF schedule name.");
        for(const auto& entry:source.slots){const auto frame=frames.find(entry.first);if(frame==frames.end())throw std::runtime_error("Only unconditional named frame schedule entries are supported.");const auto& f=frame->second;
            if(!nodes.count(f.publisher))throw std::runtime_error("LDF frame publisher is not a declared node.");LinSlot slot;slot.id=f.id;slot.delay_ms=entry.second;slot.publish=f.publisher==out.master;if(slot.publish)slot.data=f.initial;schedule.slots.push_back(std::move(slot));}
        if(!validate_lin_schedule(schedule,1,error))throw std::runtime_error(error);total_slots+=schedule.slots.size();if(total_slots>1024)throw std::runtime_error("LDF total schedule slots exceed 1024.");out.schedules.push_back(std::move(schedule));}
    out.notes.push_back("Static unconditional LDF subset; no node configuration or diagnostic execution");return out;
}
