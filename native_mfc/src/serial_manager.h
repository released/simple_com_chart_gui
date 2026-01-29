#pragma once

#include <windows.h>

#include <string>
#include <vector>

struct SerialPortInfo {
    std::wstring device;
    std::wstring description;
};

class SerialManager {
public:
    SerialManager();
    ~SerialManager();

    std::vector<SerialPortInfo> scan_ports();
    bool is_connected() const;

    bool connect(const std::wstring& port,
                 int baud,
                 int data_bits,
                 int parity,
                 int stop_bits,
                 std::wstring* error);

    void disconnect();

    std::vector<std::string> read_lines(std::wstring* error);

    int consume_rx_overflow();

private:
    void close_handle();

    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::string rx_buffer_;
    int rx_overflow_ = 0;
};
