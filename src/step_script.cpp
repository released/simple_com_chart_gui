#include "step_script.h"
#include "uart_files.h"
#include <charconv>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "log_parser.h"
namespace {
bool integer(const std::string& s,unsigned& out){const auto r=std::from_chars(s.data(),s.data()+s.size(),out);return r.ec==std::errc{}&&r.ptr==s.data()+s.size();}
}
bool validate_script(const std::vector<ScriptStep>& steps,std::string& error){
    if(steps.empty()||steps.size()>128){error="A script needs 1..128 steps.";return false;}
    const bool numeric=std::any_of(steps.begin(),steps.end(),[](const auto& step){return !step.numeric_key.empty();});
    size_t serialized=14;for(const auto& step:steps){if(step.expect.size()>1024||step.delay_ms>60000||step.timeout_ms<100||step.timeout_ms>60000){error="Expect <=1024 bytes; delay 0..60000 ms; timeout 100..60000 ms.";return false;}
        if(step.numeric_key.size()>128||!std::isfinite(step.minimum)||!std::isfinite(step.maximum)||step.minimum>step.maximum||step.numeric_key.find_first_of("\r\n\0",0,3)!=std::string::npos){error="Invalid numeric assertion key/range.";return false;}
        try{uart_wide(step.numeric_key);}catch(...){error="Invalid numeric key UTF-8.";return false;}
        if(!validate_panel({step.command},error))return false;
        if(numeric){std::ostringstream range;range<<std::setprecision(17)<<step.minimum<<','<<step.maximum;serialized+=step.numeric_key.size()*2+range.str().size()+4;}
        serialized+=encode_panel({step.command}).size()-std::string_view("SCC_COMMANDS_V1\r\n").size()+step.expect.size()*2+5+std::to_string(step.delay_ms).size()+std::to_string(step.timeout_ms).size();if(serialized>128*1024){error="Script exceeds 128 KiB; previous steps retained.";return false;}}return true;
}
std::string encode_script(const std::vector<ScriptStep>& steps){
    std::string error;if(!validate_script(steps,error))throw std::runtime_error(error);
    const bool numeric=std::any_of(steps.begin(),steps.end(),[](const auto& step){return !step.numeric_key.empty();});
    std::string out=numeric?"SCC_STEPS_V2\r\n":"SCC_STEPS_V1\r\n";
    for(const auto& step:steps){auto row=encode_panel({step.command});row.erase(0,std::string("SCC_COMMANDS_V1\r\n").size());row.resize(row.size()-2);
        out+=row+','+uart_csv_cell(uart_hex(step.expect))+','+std::to_string(step.delay_ms)+','+std::to_string(step.timeout_ms);
        if(numeric){std::ostringstream range;range<<std::setprecision(17)<<step.minimum<<','<<step.maximum;out+=','+uart_csv_cell(uart_hex(step.numeric_key))+','+range.str();}out+="\r\n";
    }if(out.size()>128*1024)throw std::runtime_error("Script exceeds 128 KiB.");return out;
}
bool decode_script(std::string_view text,std::vector<ScriptStep>& steps,std::string& error){
    constexpr std::string_view old="SCC_STEPS_V1\r\n",next="SCC_STEPS_V2\r\n";const bool numeric=text.substr(0,next.size())==next;
    if(text.size()>128*1024||(!numeric&&text.substr(0,old.size())!=old)){error="Unsupported/oversized script.";return false;}
    std::vector<std::vector<std::string>> rows;if(!uart_csv_rows(text.substr(old.size()),rows,error))return false;std::vector<ScriptStep> staged;
    for(const auto& row:rows){if(row.size()!=(numeric?17:14)){error="Invalid script column count.";return false;}
        std::string definition="SCC_COMMANDS_V1\r\n";for(size_t i=0;i<11;++i){if(i)definition+=',';definition+=uart_csv_cell(row[i]);}definition+="\r\n";
        std::vector<PanelCommand> commands;ScriptStep step;
        if(!decode_panel(definition,commands,error)||commands.size()!=1||!uart_unhex(row[11],step.expect)||!integer(row[12],step.delay_ms)||!integer(row[13],step.timeout_ms)){error="Invalid step command/expect/timing.";return false;}
        if(numeric){if(!uart_unhex(row[14],step.numeric_key)){error="Invalid numeric key HEX.";return false;}const auto parse=[](const std::string& text,double& value){const auto p=std::from_chars(text.data(),text.data()+text.size(),value);return p.ec==std::errc{}&&p.ptr==text.data()+text.size()&&std::isfinite(value);};if(!parse(row[15],step.minimum)||!parse(row[16],step.maximum)){error="Invalid numeric range.";return false;}}
        step.command=std::move(commands.front());staged.push_back(std::move(step));
    }if(!validate_script(staged,error))return false;steps=std::move(staged);return true;
}
bool StepScript::start(std::vector<ScriptStep> steps,uint64_t generation,uint64_t now,std::string& error){if(active()){error="Stop the active script first.";return false;}if(!validate_script(steps,error))return false;steps_=std::move(steps);results_.assign(steps_.size(),"PENDING");observed_.assign(steps_.size(),{});elapsed_.assign(steps_.size(),{});generation_=generation;started_=now;index_=0;deadline_=now+steps_[0].delay_ms;state_=Delay;status_="Waiting before step 1";return true;}
bool StepScript::next(uint64_t now,std::string& bytes){tick(now);if(state_!=Delay||now<deadline_)return false;std::string error;if(!panel_payload(steps_[index_].command,steps_[index_].command.value,bytes,error)){cancel(error);return false;}
    const auto& pattern=steps_[index_].expect;prefix_.assign(pattern.size(),0);for(size_t i=1,k=0;i<pattern.size();++i){while(k&&pattern[i]!=pattern[k])k=prefix_[k-1];if(pattern[i]==pattern[k])++k;prefix_[i]=k;}
    matched_=0;found_=pattern.empty();numeric_found_=steps_[index_].numeric_key.empty();numeric_lines_=SourceLineTracker{};step_started_=now;state_=Sending;status_="TX step "+std::to_string(index_+1);results_[index_]="TX pending";return true;
}
void StepScript::complete(bool success,uint64_t now){if(state_!=Sending)return;if(!success){cancel("FAILED: partial/error/cancelled TX; no retry");return;}if(found_&&numeric_found_)pass(now);else{state_=Expect;deadline_=now+steps_[index_].timeout_ms;status_="Expect step "+std::to_string(index_+1);results_[index_]="Waiting for raw RX match";}}
void StepScript::pass(uint64_t now){elapsed_[index_]=now-step_started_;results_[index_]=steps_[index_].expect.empty()?"PASS: driver accepted (no target ACK)":"PASS: raw RX matched (not protocol validation)";if(!steps_[index_].numeric_key.empty())results_[index_]="PASS: reported numeric value in range; "+steps_[index_].numeric_key+"="+std::to_string(*observed_[index_]);if(++index_==steps_.size()){state_=Idle;status_="COMPLETED: "+std::to_string(steps_.size())+" steps";}else{state_=Delay;deadline_=now+steps_[index_].delay_ms;status_="Waiting before step "+std::to_string(index_+1);}}
void StepScript::receive(std::string_view bytes,uint64_t generation,uint64_t now){
    tick(now);if(!active())return;if(generation!=generation_){cancel("STOPPED: connection changed; no automatic resume");return;}
    if(state_!=Expect&&state_!=Sending)return;const auto& step=steps_[index_];
    if(!step.numeric_key.empty()&&!numeric_found_){for(const auto& line:numeric_lines_.feed(bytes)){const auto values=log_parser::parse_kv_log(line.text);const auto it=values.find(step.numeric_key);if(it==values.end())continue;observed_[index_]=it->second;
        if(it->second<step.minimum||it->second>step.maximum){elapsed_[index_]=now-step_started_;cancel("FAIL: reported numeric value outside range: "+step.numeric_key+"="+std::to_string(it->second));return;}numeric_found_=true;break;}}
    if(!found_){const auto& pattern=step.expect;for(const char c:bytes){while(matched_&&c!=pattern[matched_])matched_=prefix_[matched_-1];if(c==pattern[matched_])++matched_;if(matched_==pattern.size()){found_=true;break;}}}
    if(found_&&numeric_found_&&state_==Expect)pass(now);
}
void StepScript::tick(uint64_t now){if(!active())return;if(now-started_>=3600000){cancel("STOPPED: one-hour execution limit");return;}if(state_==Expect&&now>=deadline_)cancel("TIMEOUT: expect did not match; no retry");}
void StepScript::cancel(std::string reason){if(active()&&index_<results_.size())results_[index_]=reason;state_=Idle;status_=std::move(reason);}

std::string StepScript::csv()const{
    std::string out="Generation,Step,Name,Result,NumericKey,Minimum,Maximum,Observed,PC_step_ms\r\n";
    const auto safe=[](std::string s){if(!s.empty()&&(s[0]=='='||s[0]=='+'||s[0]=='-'||s[0]=='@'))s.insert(s.begin(),'\'');return uart_csv_cell(s);};
    for(size_t i=0;i<results_.size();++i){std::ostringstream range;range<<std::setprecision(17)<<steps_[i].minimum<<','<<steps_[i].maximum;
        out+=std::to_string(generation_)+','+std::to_string(i+1)+','+safe(steps_[i].command.name)+','+safe(results_[i])+','+safe(steps_[i].numeric_key)+',';
        if(!steps_[i].numeric_key.empty())out+=range.str();else out+=',';out+=',';
        if(observed_[i]){std::ostringstream value;value<<std::setprecision(17)<<*observed_[i];out+=value.str();}out+=',';if(elapsed_[i])out+=std::to_string(*elapsed_[i]);out+="\r\n";
    }return out;
}
std::vector<uint64_t> StepScript::matched_timings()const {
    std::vector<uint64_t> values;
    for(size_t i=0;i<results_.size();++i)if(results_[i].rfind("PASS",0)==0&&elapsed_[i]&&(!steps_[i].expect.empty()||!steps_[i].numeric_key.empty()))values.push_back(*elapsed_[i]);
    return values;
}
size_t StepScript::timeouts()const {return static_cast<size_t>(std::count_if(results_.begin(),results_.end(),[](const auto& s){return s.rfind("TIMEOUT",0)==0;}));}
