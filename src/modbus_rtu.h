#pragma once
#include "binary_frame.h"
#include <optional>
struct RtuField {std::string name,type="u16",order="ABCD";size_t reg=0;double scale=1,add=0;};
struct RtuConfig {unsigned unit=1,function=3,address=0,count=1,period_ms=1000,timeout_ms=1000,idle_ms=2;std::vector<RtuField> fields;};
bool parse_rtu_fields(std::string_view,std::vector<RtuField>&,std::string& error);
bool validate_rtu(const RtuConfig&,std::string& error);
std::string rtu_request(const RtuConfig&);
struct RtuResult {std::vector<unsigned> registers;std::vector<std::pair<std::string,double>> values;std::string bytes;double seconds=0;};
class RtuPoller {
public:
    bool start(RtuConfig,uint64_t generation,uint64_t now,std::string& error);
    bool next(uint64_t now,std::string& bytes);
    void complete(bool ok,uint64_t now);
    void receive(std::string_view,uint64_t now,double seconds);
    void tick(uint64_t now);
    void stop(std::string why){active_=false;status_=std::move(why);}
    bool active()const{return active_;}
    std::optional<RtuResult> take(){auto out=std::move(result_);result_.reset();return out;}
    std::string status()const{return status_;}
    uint64_t errors()const{return errors_;}
private:
    void parse(uint64_t now);
    RtuConfig config_;bool active_=false,sending_=false,waiting_=false;
    uint64_t generation_=0,due_=0,deadline_=0,last_rx_=0,errors_=0;
    double seconds_=0;
    std::string pending_,status_="Idle";
    std::optional<RtuResult> result_;
};
