#include "workbench_session.h"
#include "message_format.h"
#include <algorithm>
#include <chrono>

bool serial_ports_conflict(const SerialPortInfo& a,const SerialPortInfo& b){
    return (!a.device.empty()&&!b.device.empty()&&same_serial_name(a.device,b.device)) ||
        (!a.instance_id.empty()&&!b.instance_id.empty()&&same_serial_name(a.instance_id,b.instance_id));
}
WorkbenchSession::WorkbenchSession():factory_([]{return std::make_unique<SerialManager>();}){}
WorkbenchSession::~WorkbenchSession(){
    disconnect();
    if(reader_.joinable())reader_.join();if(writer_.joinable())writer_.join();
    if(transport_)transport_->disconnect();
}
bool WorkbenchSession::idle()const{return !reserved()&&!reader_.joinable()&&!writer_.joinable();}
bool WorkbenchSession::begin(const SerialSettings& settings,bool automatic){
    if(open_.busy()||stopping_)return false;
    state_.begin(settings,automatic);
    if(!open_.start(settings,factory_)){state_.failed(GetTickCount64());return false;}
    notice_=automatic?L"Reconnecting same device...":L"Opening...";return true;
}
bool WorkbenchSession::connect(const SerialSettings& settings,std::wstring& error){
    if(!idle()){error=L"Previous B connection is still active or stopping.";return false;}
    if(!valid_serial_settings(settings)){error=L"Invalid serial settings.";return false;}
    if(!begin(settings,false)){error=L"Cannot start B open worker.";return false;}return true;
}
void WorkbenchSession::stop_workers(){
    running_=false;operations_.cancel();if(transport_)transport_->cancel_io();
    for(auto* thread:{&reader_,&writer_})if(thread->joinable())CancelSynchronousIo(thread->native_handle());
}
bool WorkbenchSession::finish_workers(){
    bool done=true;
    const auto finish=[&](std::thread& t,bool ready){if(!t.joinable())return;if(ready&&WaitForSingleObject(t.native_handle(),0)==WAIT_OBJECT_0)t.join();else done=false;};
    finish(reader_,reader_done_);finish(writer_,writer_done_);return done;
}
void WorkbenchSession::disconnect(){
    state_.cancel();open_.cancel();unexpected_=false;stopping_=true;notice_=L"Stopping...";stop_workers();
}
std::wstring WorkbenchSession::take_error(){auto value=std::move(error_);error_.clear();return value;}
void WorkbenchSession::start_workers(){
    operations_.begin();running_=true;reader_done_=writer_done_=false;failed_=false;
    const auto fail=[this](const std::wstring& message){std::lock_guard<std::mutex> lock(error_mutex_);worker_error_=message;failed_=true;};
    try{
        const auto generation=operations_.generation();
        reader_=std::thread([this,fail,generation]{
            struct Done{std::atomic<bool>& flag;~Done(){flag=true;}}done{reader_done_};
            try{while(running_){std::wstring error;std::string bytes;transport_->read_lines(&error,&bytes);
                if(!bytes.empty()){const auto now=message_unix_ms();traffic.append(bytes,now,false,0,generation);recorder.append(bytes,now);}
                if(!error.empty()){if(running_)fail(error);break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }}catch(...){if(running_)fail(L"B receive worker failed.");}
        });
        writer_=std::thread([this,fail]{
            struct Done{std::atomic<bool>& flag;~Done(){flag=true;}}done{writer_done_};
            try{while(running_){SerialOperation operation;
                if(!operations_.take(operation)){std::this_thread::sleep_for(std::chrono::milliseconds(5));continue;}
                SerialOperationResult result;result.operation=operation;
                if(!operations_.current(operation.generation)||!running_){result.cancelled=true;operations_.complete(std::move(result));continue;}
                result.accepted=paced_write(operation.bytes,operation.pacing,[&](std::string_view bytes,std::wstring& error){return transport_->write_bytes(bytes,&error);},[&]{return running_&&operations_.current(operation.generation)&&(!operation.cancel||!operation.cancel->load());},result.error);
                result.success=result.error.empty()&&result.accepted==operation.bytes.size();
                if(result.accepted){const auto now=message_unix_ms();const auto bytes=std::string_view(operation.bytes).substr(0,result.accepted);traffic.append(bytes,now,true,0,operation.generation);recorder.append(bytes,now,true);}
                operations_.complete(std::move(result));
            }}catch(...){if(running_)fail(L"B transmit worker failed; no automatic retry.");}
        });
    }catch(...){if(!reader_.joinable())reader_done_=true;if(!writer_.joinable())writer_done_=true;fail(L"Cannot start B UART workers.");}
}
bool WorkbenchSession::send(std::string bytes,std::wstring& error){
    uint64_t id=0;return submit(std::move(bytes),id,error);
}
bool WorkbenchSession::submit(std::string bytes,uint64_t& id,std::wstring& error,bool unpaced){
    if(!connected()){error=L"B is not connected.";return false;}
    SerialOperation operation;operation.bytes=std::move(bytes);operation.target=state_.active().port.device;operation.pacing_override=unpaced;
    return operations_.submit(std::move(operation),id,error);
}
void WorkbenchSession::poll(const std::vector<SerialPortInfo>& ports,const SerialPortInfo* reserved){
    const auto now=GetTickCount64();
    if(failed_.exchange(false)){
        {std::lock_guard<std::mutex> lock(error_mutex_);notice_=worker_error_;}
        unexpected_=true;stopping_=true;stop_workers();
    }
    if(stopping_&&finish_workers()){
        if(transport_){transport_->disconnect();transport_.reset();}
        stopping_=false;
        if(unexpected_){state_.lost(now);notice_=state_.retrying()?L"Device lost; waiting for the same B device.":L"Disconnected after I/O failure.";unexpected_=false;}
        else notice_=L"Disconnected";
    }
    std::unique_ptr<SerialTransport> opened;std::wstring error;
    if(open_.poll(opened,error)){
        if(state_.opening()&&!stopping_){
            const auto requested=state_.active();const auto index=find_serial_port(ports,requested.port);
            if(opened&&(!opened->is_connected()||index<0||(!requested.port.open_path.empty()&&!same_serial_name(requested.port.open_path,ports[index].open_path))||(reserved&&serial_ports_conflict(*reserved,ports[index])))){opened.reset();error=L"Device identity/path changed or Port A now owns the device; Scan and retry.";}
            if(opened){const bool manual=state_.mode()==SerialConnectionState::Mode::Opening;transport_=std::move(opened);state_.connected(state_.active().port);if(manual)traffic.clear();notice_=L"Connected: "+state_.active().port.device;recorder.event("B connected",message_unix_ms());start_workers();}
            else {const bool automatic=state_.mode()==SerialConnectionState::Mode::Reopening;state_.failed(now);notice_=L"Open failed: "+(error.empty()?L"port unavailable":error);if(!automatic)error_=notice_;}
        }
    }
    for(auto& result:operations_.results()){
        if(!result.cancelled)notice_=result.success?L"TX accepted by driver (not target ACK)":L"TX failed/partial; not retried: "+result.error;
        if(results_.size()<64)results_.push_back(std::move(result));
    }
    if(!stopping_&&!open_.busy()&&state_.retry_due(now)){
        const auto index=find_serial_port(ports,state_.active().port);
        if(index>=0&&!ports[index].instance_id.empty()&&!ports[index].open_path.empty()&&(!reserved||!serial_ports_conflict(*reserved,ports[index]))){
            auto settings=state_.active();settings.port=ports[index];begin(settings,true);
        }
    }
}
