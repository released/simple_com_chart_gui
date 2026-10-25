#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "uart_operations.h"

struct SerialPortInfo {
    std::wstring device, description, instance_id, open_path;
};

struct SerialSettings {
    SerialPortInfo port;
    int baud = 115200, data_bits = 8, parity = 0, stop_bits = 0;
    SerialControls controls;
};

bool same_serial_name(const std::wstring& left, const std::wstring& right);
bool valid_serial_port(const std::wstring& port);
bool valid_serial_settings(const SerialSettings& settings);
// An instance ID never falls back to another device occupying the same COM number.
int find_serial_port(const std::vector<SerialPortInfo>& ports, const SerialPortInfo& wanted);

class SerialConnectionState {
public:
    enum class Mode { Idle, Opening, Connected, Waiting, Reopening };
    Mode mode() const { return mode_; }
    bool opening() const { return mode_ == Mode::Opening || mode_ == Mode::Reopening; }
    bool retrying() const { return mode_ == Mode::Waiting || mode_ == Mode::Reopening; }
    const SerialSettings& active() const { return active_; }
    void begin(const SerialSettings& settings, bool automatic);
    void connected(const SerialPortInfo& actual);
    void failed(uint64_t now);
    void lost(uint64_t now);
    void cancel();
    void update_controls(const SerialControls& value) { active_.controls = value; }
    bool retry_due(uint64_t now) const;
private:
    Mode mode_ = Mode::Idle;
    SerialSettings active_;
    uint64_t due_ = 0, delay_ = 2000;
};
