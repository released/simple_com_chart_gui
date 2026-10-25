#include "serial_connection.h"
#include <cwctype>
#include <algorithm>

bool same_serial_name(const std::wstring& left, const std::wstring& right) {
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(),
        [](wchar_t a, wchar_t b) { return std::towupper(a) == std::towupper(b); });
}

bool valid_serial_port(const std::wstring& port) {
    return port.size() > 3 && port.size() <= 12 && same_serial_name(port.substr(0, 3), L"COM") &&
        port[3] >= L'1' && port[3] <= L'9' && port.find_first_not_of(L"0123456789", 3) == std::wstring::npos;
}

bool valid_serial_settings(const SerialSettings& settings) {
    if (!valid_serial_controls(settings.controls)) return false;
    return valid_serial_port(settings.port.device) && settings.baud > 0 && settings.data_bits >= 5 && settings.data_bits <= 8 &&
        settings.parity >= 0 && settings.parity <= 2 && settings.stop_bits >= 0 && settings.stop_bits <= 2;
}

int find_serial_port(const std::vector<SerialPortInfo>& ports, const SerialPortInfo& wanted) {
    int found = -1;
    for (size_t i = 0; i < ports.size(); ++i) {
        const bool match = wanted.instance_id.empty() ? same_serial_name(ports[i].device, wanted.device) :
            same_serial_name(ports[i].instance_id, wanted.instance_id);
        if (!match) continue;
        if (found >= 0) return -1; // Ambiguous identity: require a manual choice.
        found = static_cast<int>(i);
    }
    return found;
}

void SerialConnectionState::begin(const SerialSettings& settings, bool automatic) {
    active_ = settings;
    mode_ = automatic ? Mode::Reopening : Mode::Opening;
}

void SerialConnectionState::connected(const SerialPortInfo& actual) {
    active_.port = actual; mode_ = Mode::Connected; delay_ = 2000; due_ = 0;
}

void SerialConnectionState::failed(uint64_t now) {
    if (mode_ != Mode::Reopening) { cancel(); return; }
    mode_ = Mode::Waiting; due_ = now + delay_; delay_ = std::min<uint64_t>(10000, delay_ * 2);
}

void SerialConnectionState::lost(uint64_t now) {
    if (mode_ != Mode::Connected || active_.port.instance_id.empty()) { cancel(); return; }
    mode_ = Mode::Waiting; delay_ = 2000; due_ = now + delay_;
}

void SerialConnectionState::cancel() { mode_ = Mode::Idle; due_ = 0; delay_ = 2000; }
bool SerialConnectionState::retry_due(uint64_t now) const { return mode_ == Mode::Waiting && now >= due_; }
