#pragma once
#include "block_protocol.h"
#include "background_job.h"
#include "traffic_store.h"
#include "uart_operations.h"
#include <filesystem>
#include <map>
#include <memory>
class ProtocolTransferJob {
public:
    using Submit=std::function<bool(std::string,uint64_t&,std::wstring&,std::function<void(const SerialOperationResult&)>)>;
    using Current=std::function<bool()>;
    using Cancel=std::function<void(uint64_t)>;
    struct Options {BlockOptions protocol;bool receive=false,replace=false;std::vector<std::filesystem::path> paths;std::optional<uint64_t> x_size;};
    bool start(Options,TrafficStore&,uint64_t generation,Submit,Current,Cancel);
    bool busy()const{return job_.busy();}
    void cancel(){job_.cancel();}
    bool poll(std::wstring& result);
    std::wstring progress()const;
private:
    struct State {std::mutex mutex;std::map<uint64_t,SerialOperationResult> results;std::atomic<uint64_t> bytes{0};std::atomic<unsigned> retries{0},files{0};uint64_t generation=0;};
    std::shared_ptr<State> state_;
    BackgroundJob<unsigned> job_;
};
