#pragma once

#include <windows.h>

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include "serial_connection.h"

class SerialTransport {
public:
    virtual ~SerialTransport() = default;
    virtual bool is_connected() const = 0;
    virtual bool connect(const std::wstring&, int, int, int, int, std::wstring*) = 0;
    virtual bool open(const SerialSettings& settings, std::wstring* error) {
        const auto& path = settings.port.open_path.empty() ? settings.port.device : settings.port.open_path;
        return connect(path, settings.baud, settings.data_bits, settings.parity, settings.stop_bits, error);
    }
    virtual void disconnect() = 0;
    virtual std::vector<std::string> read_lines(std::wstring*, std::string* = nullptr) = 0;
    virtual int consume_rx_overflow() = 0;
    virtual size_t write_bytes(std::string_view, std::wstring* error) { if (error) *error = L"TX is unsupported by this transport"; return 0; }
    virtual bool set_controls(const SerialControls&, std::wstring* error) { if (error) *error = L"Control lines are unsupported by this transport"; return false; }
    virtual bool send_break(std::wstring* error) { if (error) *error = L"Break is unsupported by this transport"; return false; }
    virtual SerialLineStatus sample_status() { return {}; }
    virtual void cancel_io() {}
};

class SerialManager : public SerialTransport {
public:
    SerialManager();
    ~SerialManager() override;

    static std::vector<SerialPortInfo> scan_ports(bool* success = nullptr);
    bool is_connected() const override;
    bool open(const SerialSettings&, std::wstring*) override;
    size_t write_bytes(std::string_view, std::wstring*) override;
    bool set_controls(const SerialControls&, std::wstring*) override;
    bool send_break(std::wstring*) override;
    SerialLineStatus sample_status() override;
    void cancel_io() override;

    bool connect(const std::wstring& port,
                 int baud,
                 int data_bits,
                 int parity,
                 int stop_bits,
                 std::wstring* error) override;

    void disconnect() override;

    std::vector<std::string> read_lines(std::wstring* error, std::string* raw_bytes = nullptr) override;

    int consume_rx_overflow() override;

private:
    void close_handle();

    HANDLE handle_ = INVALID_HANDLE_VALUE;
    mutable std::recursive_mutex mutex_;
    std::string rx_buffer_;
    std::atomic<int> rx_overflow_{0};
    std::atomic<bool> connected_{false};
    SerialControls controls_;
    uint32_t pending_errors_ = 0;
    std::atomic<bool> io_cancelled_{false};
};
