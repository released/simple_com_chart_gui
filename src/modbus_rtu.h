#pragma once
#include "binary_frame.h"
#include <optional>
#include <deque>
struct RtuField {std::string name,type="u16",order="ABCD";size_t reg=0;double scale=1,add=0;};
struct RtuConfig {unsigned unit=1,function=3,address=0,count=1,period_ms=1000,timeout_ms=1000,idle_ms=2;std::vector<RtuField> fields;};
bool parse_rtu_fields(std::string_view,std::vector<RtuField>&,std::string& error);
bool validate_rtu(const RtuConfig&,std::string& error);
std::string rtu_request(const RtuConfig&);
struct RtuResult {
    std::vector<unsigned> registers;
    std::vector<std::pair<std::string,double>> values;
    std::string bytes;
    double seconds=0;
    unsigned unit=0,function=0,address=0;
    size_t block=0;
    uint64_t observed_ms=0;
    uint64_t generation=0;
    bool raw=true;
};
enum class RtuOutcome { Pending, Success, Exception, Timeout, SendFailure, Stopped };
struct RtuTransaction {
    uint64_t sequence=0,started_ms=0,finished_ms=0;
    size_t block=0;
    unsigned unit=0,function=0,address=0,count=0,exception=0;
    RtuOutcome outcome=RtuOutcome::Pending;
    std::string request,response;
};
struct RtuDiagnostics {uint64_t requests=0,responses=0,crc_errors=0,length_errors=0,unmatched_bytes=0;};
class RtuPoller {
public:
    bool start(RtuConfig,uint64_t generation,uint64_t now,std::string& error);
    bool start_plan(std::vector<RtuConfig>,uint64_t generation,uint64_t now,std::string& error);
    bool next(uint64_t now,std::string& bytes);
    void complete(bool ok,uint64_t now);
    void receive(std::string_view,uint64_t now,double seconds);
    void tick(uint64_t now);
    void stop(std::string why);
    bool active()const{return active_;}
    std::optional<RtuResult> take(){auto out=std::move(result_);result_.reset();return out;}
    std::string status()const{return status_;}
    uint64_t errors()const{return errors_;}
    const std::deque<RtuTransaction>& transactions()const{return transactions_;}
    const RtuDiagnostics& diagnostics()const{return diagnostics_;}
    size_t plan_size()const{return plan_.size();}
    uint64_t revision()const{return revision_;}
    const std::vector<RtuConfig>& plan()const{return plan_;}
private:
    void parse(uint64_t now);
    void end_transaction(RtuOutcome,uint64_t,unsigned exception=0);
    RtuConfig config_;bool active_=false,sending_=false,waiting_=false;
    uint64_t generation_=0,observed_ms_=0,deadline_=0,last_rx_=0,errors_=0;
    double seconds_=0;
    std::string pending_,status_="Idle";
    std::optional<RtuResult> result_;
    std::vector<RtuConfig> plan_;
    std::vector<uint64_t> due_times_;
    size_t block_=0,next_block_=0;
    uint64_t revision_=0;
    std::deque<RtuTransaction> transactions_;
    RtuDiagnostics diagnostics_;
};
