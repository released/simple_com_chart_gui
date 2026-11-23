#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>

enum class LinCheck { Unknown, Valid, Invalid };
enum class LinChecksum { Unknown, Classic, Enhanced };
struct LinFrame {
    double seconds=0;
    unsigned id=0;
    int pid=-1,checksum=-1;
    std::string data,direction="RX";
    LinChecksum checksum_type=LinChecksum::Unknown;
    LinCheck break_check=LinCheck::Unknown;
};
struct LinValidation {
    LinCheck pid=LinCheck::Unknown,checksum=LinCheck::Unknown;
    bool malformed=false;
    std::string detail;
    bool chartable(const LinFrame&,bool allow_unknown=false)const;
};
struct LinSignal {
    std::string name,unit;
    unsigned id=0,start=0,length=1;
    bool signed_value=false;
    double scale=1,offset=0;
    std::optional<uint32_t> minimum,maximum;
    std::map<uint32_t,std::string> logical;
};
struct LinValue {std::string key,unit,text;double value=0;uint32_t raw=0;};
struct LinSlot {unsigned id=0,delay_ms=10;bool publish=false;std::string data;};
struct LinSchedule {std::string name;std::vector<LinSlot> slots;};
struct LinDatabase {
    std::string version,master;
    unsigned baud=19200;
    std::vector<LinSignal> signals;
    std::map<unsigned,unsigned> lengths;
    std::map<unsigned,std::string> frame_names;
    std::vector<LinSchedule> schedules;
    std::vector<std::string> notes;
};
unsigned lin_pid(unsigned id);
unsigned lin_checksum(unsigned id,std::string_view,LinChecksum);
LinValidation validate_lin_frame(const LinFrame&);
bool validate_lin_signals(const std::vector<LinSignal>&,std::string&);
std::vector<LinValue> decode_lin_values(const LinFrame&,const std::vector<LinSignal>&);
std::vector<LinFrame> decode_lin_trace(std::string_view);
std::string encode_lin_trace(const std::vector<LinFrame>&);
std::vector<LinSignal> decode_lin_signals(std::string_view);
std::string encode_lin_signals(const std::vector<LinSignal>&);
LinDatabase parse_lin_ldf(std::string_view);
const char* lin_check_name(LinCheck);

struct LinCapabilities {
    std::string name="Offline files";
    bool receive=false,header=false,publish=false,hardware_schedule=false,hardware_clock=false;
};
struct LinCompletion {bool success=false,partial=false,cancelled=false;uint64_t generation=0;};
class LinTransport {
public:
    virtual ~LinTransport()=default;
    virtual LinCapabilities capabilities()const=0;
    virtual uint64_t generation()const=0;
    // Nonblocking ownership contract: one ticket until completion or drained cancel.
    virtual bool submit(const LinSlot&,uint64_t& ticket,std::string&)=0;
    virtual bool schedule(const LinSchedule&,unsigned cycles,uint64_t& ticket,std::string&)=0;
    virtual bool take(uint64_t ticket,LinCompletion&)=0;
    virtual void cancel(uint64_t ticket)=0;
};
class LinScheduler {
public:
    bool start(const LinSchedule&,unsigned cycles,LinTransport*,uint64_t now,std::string&);
    void poll(uint64_t now);
    void stop();
    bool active()const{return active_;}
    bool draining()const{return ticket_!=0;}
    const std::string& status()const{return status_;}
private:
    LinSchedule schedule_;
    LinTransport* transport_=nullptr;
    uint64_t generation_=0,ticket_=0,due_=0,deadline_=0,started_=0;
    size_t at_=0;
    unsigned cycles_=0,completed_=0;
    bool active_=false;
    std::string status_="Idle";
};
bool validate_lin_schedule(const LinSchedule&,unsigned cycles,std::string&);
