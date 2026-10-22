#include "serial_connection.h"
#include "serial_open_job.h"
#include "runtime_serial_fixture.h"
#include <stdexcept>
#include <chrono>

namespace {
void check(bool ok, const char* name) { if (!ok) throw std::runtime_error(name); }
template <typename Predicate> bool wait(Predicate ready) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < end) {
        if (ready()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return ready();
}
}

void test_serial_connection() {
    SerialPortInfo original{L"COM6", L"Bridge", L"USB\\A", L"fake://A"};
    SerialPortInfo other{L"COM6", L"Same label", L"USB\\B", L"fake://B"};
    SerialPortInfo returned{L"COM19", L"Bridge", L"usb\\a", L"fake://A"};
    check(valid_serial_port(L"com123") && !valid_serial_port(L"COM0") && !valid_serial_port(L"C:\\file"), "Stored port validation rejects file paths");
    check(find_serial_port({other}, original) == -1, "Reused COM number cannot impersonate device identity");
    check(find_serial_port({other, returned}, original) == 1, "Stable identity survives COM renumbering and case changes");
    check(find_serial_port({original, returned}, original) == -1, "Duplicate identity is ambiguous");
    SerialSettings config; config.port = original;
    check(valid_serial_settings(config), "Default serial configuration");
    config.baud = 0; check(!valid_serial_settings(config), "Reject invalid baud"); config.baud = 230400;
    SerialConnectionState state;
    check(state.mode() == SerialConnectionState::Mode::Idle && !state.retry_due(99999), "Startup never auto-opens remembered port");
    state.begin(config, false); state.failed(0);
    check(state.mode() == SerialConnectionState::Mode::Idle, "Initial manual failure does not arm retries");
    state.begin(config, false); state.connected(original); state.lost(100);
    check(!state.retry_due(2099) && state.retry_due(2100), "Unexpected loss schedules a bounded delay");
    uint64_t now = 3000;
    for (uint64_t delay : {2000, 4000, 8000, 10000, 10000}) {
        state.begin(config, true); state.failed(now);
        check(!state.retry_due(now + delay - 1) && state.retry_due(now + delay), "Retry backoff remains bounded at ten seconds"); now += 20000;
    }
    state.cancel(); check(!state.retry_due(now + 99999), "Manual cancel disables future retries");
    state.begin(config, false); state.connected(original);
    check(state.mode() == SerialConnectionState::Mode::Connected && !state.retry_due(now), "Quiet connected stream is not a disconnect");
    auto legacy = original; legacy.instance_id.clear(); state.connected(legacy); state.lost(now);
    check(state.mode() == SerialConnectionState::Mode::Idle, "Unknown device identity never blindly reconnects");

    auto fake = std::make_shared<FakeSerialState>(); fake->hold_open = true;
    SerialOpenJob job;
    const auto factory = [fake] { return std::make_unique<FakeSerialTransport>(fake); };
    auto invalid = config; invalid.port.device = L"C:\\not-a-port";
    check(!job.start(invalid, factory) && fake->attempt_count() == 0, "Invalid configuration cannot enter the open worker");
    check(job.start(config, factory) && wait([&] { return fake->attempt_count() == 1; }), "Background open enters fake transport");
    check(!job.start(config, factory), "Only one open worker at a time");
    job.cancel(); fake->release();
    std::unique_ptr<SerialTransport> transport; std::wstring error;
    check(wait([&] { return job.poll(transport, error); }), "Cancelled job completes");
    check(!transport && !job.busy() && fake->active == 0 && fake->closed == 1, "Late successful open is closed and discarded");
    check(job.start(config, factory) && wait([&] { return job.poll(transport, error); }) && transport && transport->is_connected(), "Open worker can be reused after cancellation");
    transport.reset();
    auto closing = std::make_shared<FakeSerialState>(); closing->hold_open = true;
    {
        SerialOpenJob pending;
        check(pending.start(config, [closing] { return std::make_unique<FakeSerialTransport>(closing); }) &&
            wait([&] { return closing->attempt_count() == 1; }), "Shutdown fixture has a pending open");
    }
    closing->release();
    check(wait([&] { std::lock_guard<std::mutex> lock(closing->mutex); return closing->closed == 1 && closing->active == 0; }),
        "Job owner can close before driver completion without retaining a UI owner");
}
