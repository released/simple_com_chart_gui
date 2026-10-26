#include "transaction_analysis.h"
#include "uart_files.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
namespace {
std::string span(const TrafficRecord& f){return std::to_string(f.first_sequence?f.first_sequence:f.sequence)+":"+std::to_string(f.first_offset)+".."+std::to_string(f.sequence)+":"+std::to_string(f.last_offset?f.last_offset:f.first_offset+f.bytes.size());}
}
TransactionResult analyze_transactions(const TrafficBatch& batch,const TransactionOptions& options,double now,const std::atomic<bool>& cancel){
    if(options.source>1||(options.width!=1&&options.width!=2&&options.width!=4)||options.request_id>65536-options.width||options.reply_id>65536-options.width||options.request_prefix.size()>32||options.reply_prefix.size()>32||!std::isfinite(options.timeout)||options.timeout<=0||options.timeout>60||!std::isfinite(now)||batch.records.size()>2048)throw std::runtime_error("Transaction ID/prefix/timeout/snapshot limit is invalid.");
    size_t input_bytes=0;for(const auto& f:batch.records){if(f.bytes.size()>256*1024-input_bytes)throw std::runtime_error("Transaction snapshot exceeds 256 KiB.");input_bytes+=f.bytes.size();}
    TrafficFramer framer;std::string error;if(!framer.configure(options.framing,error))throw std::runtime_error(error);
    TransactionResult result;struct Pending{std::vector<size_t> rows;double until=0;bool ambiguous=false;};std::map<uint32_t,Pending> pending;
    size_t frames=0,ignored=0,invalid=0,matched=0;uint64_t generation=UINT64_MAX;double observed=now;
    const auto boundary=[&](uint64_t next){if(generation==next)return;for(const auto& p:pending)for(auto n:p.second.rows)if(result.rows[n].status=="PENDING")result.rows[n].status="GENERATION ENDED";pending.clear();generation=next;};
    const auto expire=[&](double time){for(auto i=pending.begin();i!=pending.end();){if(time>=i->second.until){for(auto n:i->second.rows)if(result.rows[n].status=="PENDING")result.rows[n].status="TIMEOUT";i=pending.erase(i);}else ++i;}};
    const auto consume=[&](TrafficRecord frame){
        if(cancel)throw std::runtime_error("Transaction analysis cancelled.");if(++frames>8192){result.truncated=true;return;}
        if(!std::isfinite(frame.sample_seconds)){++invalid;return;}observed=std::max(observed,frame.sample_seconds);
        boundary(frame.generation);
        expire(frame.sample_seconds);const auto& prefix=frame.tx?options.request_prefix:options.reply_prefix;
        if(frame.bytes.compare(0,prefix.size(),prefix)!=0){++ignored;return;}const auto offset=frame.tx?options.request_id:options.reply_id;
        if(offset>frame.bytes.size()||options.width>frame.bytes.size()-offset){++invalid;return;}
        uint32_t id=0;for(size_t n=0;n<options.width;++n){const auto p=options.big?n:options.width-1-n;id=(id<<8)|static_cast<unsigned char>(frame.bytes[offset+p]);}
        if(frame.tx){if(result.rows.size()>=512){result.truncated=true;return;}TransactionRow row;row.id=id;row.request=std::move(frame);row.status="PENDING";const auto n=result.rows.size();result.rows.push_back(std::move(row));auto& state=pending[id];
            if(!state.rows.empty()){state.ambiguous=true;for(auto prior:state.rows)result.rows[prior].status="AMBIGUOUS ID OVERLAP";result.rows[n].status="AMBIGUOUS ID OVERLAP";}
            state.rows.push_back(n);state.until=std::max(state.until,result.rows[n].request->sample_seconds+options.timeout);
        } else {const auto found=pending.find(id);if(found==pending.end()||found->second.ambiguous){if(result.rows.size()>=512){result.truncated=true;return;}TransactionRow row;row.id=id;row.reply=std::move(frame);row.status=found==pending.end()?"ORPHAN REPLY":"AMBIGUOUS REPLY";result.rows.push_back(std::move(row));}
            else {auto& row=result.rows[found->second.rows.front()];row.reply=std::move(frame);const auto seconds=row.reply->sample_seconds-row.request->sample_seconds;if(seconds<0){row.status="PC CLOCK ORDER UNCERTAIN";}else{row.status="RULE MATCH";row.milliseconds=seconds*1000;++matched;}pending.erase(found);}}
    };
    for(const auto& record:batch.records){if(cancel)throw std::runtime_error("Transaction analysis cancelled.");if(result.truncated)break;
        boundary(record.generation);
        if(!std::isfinite(record.sample_seconds)){framer.reset();for(const auto& p:pending)for(auto n:p.second.rows)if(result.rows[n].status=="PENDING")result.rows[n].status="INVALID CLOCK GAP";pending.clear();++invalid;continue;}for(auto& frame:framer.feed(record)){if(result.truncated)break;consume(std::move(frame));}}
    for(size_t n=0;n<8192&&!result.truncated;++n){auto ready=framer.drain(observed);if(ready.empty())break;for(auto& f:ready){if(result.truncated)break;consume(std::move(f));}}
    expire(observed);std::ostringstream csv,report;csv<<std::setprecision(17)<<"Port,Row,ID,Status,PC_ms,Request_span,Reply_span\r\n";
    for(size_t n=0;n<result.rows.size();++n){const auto& r=result.rows[n];csv<<(options.source?'B':'A')<<','<<n<<','<<r.id<<','<<r.status<<',';if(r.status=="RULE MATCH")csv<<r.milliseconds;csv<<','<<(r.request?span(*r.request):"")<<','<<(r.reply?span(*r.reply):"")<<"\r\n";}
    report<<"PORT "<<(options.source?'B':'A')<<" passive snapshot: "<<result.rows.size()<<" rows, "<<matched<<" rule matches; "<<ignored<<" prefix-filtered / "<<invalid<<" invalid spans/clocks. Framing discarded "<<framer.discarded()<<" bytes; history expired "<<batch.expired<<" bytes.\r\n"<<(result.truncated?"LIMIT: report is partial (512 rows / 8192 frames).\r\n":"")<<"TX means driver-accepted bytes, not device ACK. PC intervals include host scheduling; no wire timing/loss claim. Duplicate IDs remain ambiguous until timeout. Span endpoints are original retained traffic sequence:byte offset, end-exclusive.\r\n\r\n";
    result.csv=csv.str();result.report=report.str()+result.csv;return result;
}
std::string transaction_detail(const TransactionResult& data,size_t row,bool reply){
    if(row>=data.rows.size())throw std::runtime_error("Select a valid result row.");const auto& r=data.rows[row];const auto& f=reply?r.reply:r.request;if(!f)throw std::runtime_error(reply?"This row has no identified reply.":"This row has no identified request.");
    return std::string(reply?"REPLY":"REQUEST")+" | row "+std::to_string(row)+" | ID "+std::to_string(r.id)+" | "+r.status+"\r\nTraffic span "+span(*f)+"; generation "+std::to_string(f->generation)+"; receipt Unix ms "+std::to_string(f->receipt_ms)+"\r\nHEX: "+uart_hex(std::string_view(f->bytes).substr(0,4096))+(f->bytes.size()>4096?"\r\nPreview limited to first 4096 bytes.":"");
}
