#pragma once
#include <windows.h>
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include "error_message.h"

// Only the owner calls start/take. Destruction is the final lifetime barrier.
template<class T> class BackgroundJob {
public:
    ~BackgroundJob() { cancel(); if (thread_.joinable()) thread_.join(); }
    bool busy() const { return thread_.joinable(); }
    void cancel() { cancelled_ = true; }
    bool start(std::function<T(const std::atomic<bool>&)> work) {
        if (busy()) return false;
        cancelled_ = false; done_ = false; error_.clear();
        try { thread_ = std::thread([this, work = std::move(work)] {
            try { value_ = work(cancelled_); }
            catch (const std::exception& ex) { error_=error_message(ex.what()); }
            catch (...) { error_ = L"Background operation failed"; }
            done_ = true;
        }); } catch (...) { error_ = L"Cannot start background worker"; return false; }
        return true;
    }
    bool take(T& value, std::wstring& error, bool discard_cancelled = true) {
        if (!busy() || !done_ || WaitForSingleObject(thread_.native_handle(), 0) != WAIT_OBJECT_0) return false;
        thread_.join();
        error = cancelled_ && discard_cancelled ? L"Cancelled; prior data retained" : error_;
        if (error.empty()) value = std::move(value_);
        value_ = T{}; return true;
    }
private:
    std::thread thread_;
    std::atomic<bool> cancelled_{false}, done_{false};
    T value_{};
    std::wstring error_;
};
