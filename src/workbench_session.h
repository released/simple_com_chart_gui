#pragma once
#include "serial_open_job.h"
#include "traffic_store.h"
#include "uart_recorder.h"
#include <functional>
#include <thread>

bool serial_ports_conflict(const SerialPortInfo&,const SerialPortInfo&);
// UI-owned lifecycle; only receive/TX workers touch the connected transport.
class WorkbenchSession {
public:
    WorkbenchSession();
    ~WorkbenchSession();
    bool connect(const SerialSettings&,std::wstring& error);
    void disconnect();
    void poll(const std::vector<SerialPortInfo>& ports,const SerialPortInfo* reserved=nullptr);
    bool send(std::string bytes,std::wstring& error);
    bool submit(std::string bytes,uint64_t& id,std::wstring& error,bool unpaced=false);
    void cancel(uint64_t id){operations_.cancel_pending(id);}
    uint64_t generation()const{return operations_.generation();}
    bool tx_busy()const{return operations_.busy();}
    SerialOperationQueue& queue(){return operations_;}
    void pacing(TxPacing p){operations_.pacing(p);}
    std::vector<SerialOperationResult> results(){auto result=std::move(results_);results_.clear();return result;}
    bool idle()const;
    bool connected()const{return state_.mode()==SerialConnectionState::Mode::Connected&&!stopping_;}
    bool reserved()const{return state_.mode()!=SerialConnectionState::Mode::Idle||stopping_||open_.busy();}
    const SerialSettings& settings()const{return state_.active();}
    std::wstring status()const{return notice_;}
    std::wstring take_error();
    void set_factory(SerialOpenJob::Factory factory){factory_=std::move(factory);}
    TrafficStore traffic;
    UartRecorder recorder;
private:
    bool begin(const SerialSettings&,bool automatic);
    void start_workers();
    void stop_workers();
    bool finish_workers();
    SerialConnectionState state_;
    SerialOpenJob open_;
    SerialOpenJob::Factory factory_;
    std::unique_ptr<SerialTransport> transport_;
    SerialOperationQueue operations_;
    std::thread reader_,writer_;
    std::atomic<bool> running_{false},reader_done_{true},writer_done_{true},failed_{false};
    std::mutex error_mutex_;
    std::wstring worker_error_,error_,notice_=L"Disconnected";
    bool stopping_=false,unexpected_=false;
    std::vector<SerialOperationResult> results_;
};
