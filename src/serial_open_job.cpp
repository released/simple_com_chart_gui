#include "serial_open_job.h"

SerialOpenJob::~SerialOpenJob() {
    cancel();
    if (thread_.joinable()) {
        if (state_->done.load()) thread_.join();
        else thread_.detach(); // A blocked driver open cannot retain the dialog.
    }
}

bool SerialOpenJob::start(const SerialSettings& settings, Factory factory) {
    if (busy() || !valid_serial_settings(settings)) return false;
    auto result = std::make_shared<State>();
    try {
        thread_ = std::thread([result, settings, factory = std::move(factory)] {
            try {
                auto transport = factory();
                if (transport && !result->cancelled && transport->open(settings, &result->error)) {
                    if (!result->cancelled) result->transport = std::move(transport);
                }
            } catch (...) { result->error = L"Cannot open serial transport"; }
            result->done.store(true);
        });
    } catch (...) { return false; }
    state_ = std::move(result); return true;
}

bool SerialOpenJob::poll(std::unique_ptr<SerialTransport>& transport, std::wstring& error) {
    if (!state_ || !state_->done.load()) return false;
    thread_.join();
    if (!state_->cancelled) { transport = std::move(state_->transport); error = std::move(state_->error); }
    state_.reset(); return true;
}

void SerialOpenJob::cancel() { if (state_) state_->cancelled.store(true); }
