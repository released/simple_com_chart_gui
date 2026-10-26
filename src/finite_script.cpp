#include "finite_script.h"
#include "message_format.h"
#include <windows.h>
#include <thread>
#include <stdexcept>

namespace {
struct WriteWorker {
    SerialTransport& transport;
    std::thread thread;
    std::atomic<bool> done{false};
    bool success=false;
    ~WriteWorker(){if(thread.joinable()){transport.cancel_io();thread.join();}}
    void start(std::string bytes){done=false;success=false;thread=std::thread([this,bytes=std::move(bytes)]{try{std::wstring error;const auto count=transport.write_bytes(bytes,&error);success=error.empty()&&count==bytes.size();}catch(...){success=false;}done=true;});}
};
}
FiniteScriptResult execute_finite_script(SerialTransport& transport,std::vector<ScriptStep> steps,const std::atomic<bool>& cancel,uint64_t maximum_ms) {
    if(!maximum_ms||maximum_ms>3600000)throw std::runtime_error("Finite runner limit must be 1..3600000 ms.");
    StepScript script;std::string error;const auto started=GetTickCount64();
    if(!script.start(std::move(steps),1,started,error))throw std::runtime_error(error);
    WriteWorker writer{transport};
    try {
        while(script.active()) {
            const auto now=GetTickCount64();
            if(cancel||now-started>=maximum_ms){script.cancel(cancel?"STOPPED: cancelled; no retry":"STOPPED: runner duration limit; no retry");break;}
            if(!transport.is_connected()){script.cancel("STOPPED: disconnected; no reconnect/resume");break;}
            if(writer.thread.joinable()&&writer.done){writer.thread.join();script.complete(writer.success,now);}
            script.tick(now);if(!script.active())break;
            std::string bytes;if(!writer.thread.joinable()&&script.next(now,bytes))writer.start(std::move(bytes));
            std::wstring read_error;std::string raw;transport.read_lines(&read_error,&raw);
            if(!raw.empty())script.receive(raw,1,GetTickCount64());
            if(!read_error.empty()||transport.consume_rx_overflow()){script.cancel("STOPPED: RX error/overflow; no retry");break;}
            Sleep(1);
        }
    }catch(const std::exception& e){script.cancel(std::string("FAILED: ")+e.what()+"; no retry");}
    if(writer.thread.joinable()){transport.cancel_io();writer.thread.join();}
    FiniteScriptResult result;result.passed=script.status().rfind("COMPLETED",0)==0;result.status=script.status();result.csv=script.csv();return result;
}
