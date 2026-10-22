#pragma once
#include "serial_manager.h"
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

// The worker owns only its shared result/transport; it never references a window.
class SerialOpenJob {
public:
    using Factory = std::function<std::unique_ptr<SerialTransport>()>;
    ~SerialOpenJob();
    bool start(const SerialSettings& settings, Factory factory);
    bool poll(std::unique_ptr<SerialTransport>& transport, std::wstring& error);
    void cancel();
    bool busy() const { return static_cast<bool>(state_); }
private:
    struct State {
        std::atomic<bool> done{false}, cancelled{false};
        std::unique_ptr<SerialTransport> transport;
        std::wstring error;
    };
    std::shared_ptr<State> state_;
    std::thread thread_;
};
