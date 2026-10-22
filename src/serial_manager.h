#pragma once

#include <windows.h>

#include <string>
#include <vector>
#include <mutex>
#include "serial_connection.h"

class SerialTransport {
public:
    virtual ~SerialTransport() = default;
    virtual bool is_connected() const = 0;
    virtual bool connect(const std::wstring&, int, int, int, int, std::wstring*) = 0;
    virtual void disconnect() = 0;
    virtual std::vector<std::string> read_lines(std::wstring*, std::string* = nullptr) = 0;
    virtual int consume_rx_overflow() = 0;
};

class SerialManager : public SerialTransport {
public:
    SerialManager();
    ~SerialManager() override;

    static std::vector<SerialPortInfo> scan_ports(bool* success = nullptr);
    bool is_connected() const override;

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
    int rx_overflow_ = 0;
};
