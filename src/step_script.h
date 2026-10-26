#pragma once
#include "command_panel.h"
#include "source_mapping.h"
#include <optional>
struct ScriptStep {PanelCommand command;std::string expect;unsigned delay_ms=0,timeout_ms=1000;std::string numeric_key;double minimum=0,maximum=0;};
bool validate_script(const std::vector<ScriptStep>&,std::string& error);
std::string encode_script(const std::vector<ScriptStep>&);
bool decode_script(std::string_view,std::vector<ScriptStep>&,std::string& error);
class StepScript {
public:
    bool start(std::vector<ScriptStep>,uint64_t generation,uint64_t now,std::string& error);
    bool next(uint64_t now,std::string& bytes);
    void complete(bool success,uint64_t now);
    void receive(std::string_view,uint64_t generation,uint64_t now);
    void tick(uint64_t now);
    void cancel(std::string reason);
    bool active()const{return state_!=Idle;}
    uint64_t generation()const{return generation_;}
    size_t index()const{return index_;}
    const std::vector<std::string>& results()const{return results_;}
    std::string status()const{return status_;}
    std::string csv()const;
    std::vector<uint64_t> matched_timings()const;
    size_t timeouts()const;
private:
    void pass(uint64_t now);
    enum State{Idle,Delay,Sending,Expect}state_=Idle;
    std::vector<ScriptStep> steps_;
    std::vector<std::string> results_;
    std::vector<size_t> prefix_;
    size_t index_=0,matched_=0;
    bool found_=false;
    SourceLineTracker numeric_lines_;
    bool numeric_found_=false;
    uint64_t step_started_=0;
    std::vector<std::optional<double>> observed_;
    std::vector<std::optional<uint64_t>> elapsed_;
    uint64_t generation_=0,deadline_=0,started_=0;
    std::string status_="Idle";
};
