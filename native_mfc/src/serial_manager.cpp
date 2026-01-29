#include "serial_manager.h"

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>

#include <algorithm>
#include <string>

#pragma comment(lib, "setupapi.lib")

namespace {
constexpr int kMaxRxBuffer = 4096;

std::wstring trim_ws(const std::wstring& input) {
    size_t start = input.find_first_not_of(L" \t\r\n");
    size_t end = input.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) {
        return L"";
    }
    return input.substr(start, end - start + 1);
}

std::wstring extract_com_port(const std::wstring& friendly) {
    size_t pos = friendly.find(L"(COM");
    if (pos == std::wstring::npos) {
        return L"";
    }
    size_t end = friendly.find(L")", pos);
    if (end == std::wstring::npos) {
        end = friendly.size();
    }
    std::wstring token = friendly.substr(pos + 1, end - pos - 1); // COM3
    if (token.find(L"COM") == 0) {
        return token;
    }
    return L"";
}

std::wstring format_port_path(const std::wstring& port) {
    if (port.rfind(L"\\.\\", 0) == 0) {
        return port;
    }
    if (port.size() > 3) {
        return L"\\\\.\\" + port;
    }
    return port;
}
} // namespace

SerialManager::SerialManager() = default;
SerialManager::~SerialManager() {
    disconnect();
}

std::vector<SerialPortInfo> SerialManager::scan_ports() {
    std::vector<SerialPortInfo> ports;

    HDEVINFO devs = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (devs == INVALID_HANDLE_VALUE) {
        return ports;
    }

    SP_DEVINFO_DATA info = {};
    info.cbSize = sizeof(info);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(devs, i, &info); ++i) {
        wchar_t buf[512] = {};
        DWORD size = 0;
        if (!SetupDiGetDeviceRegistryPropertyW(devs, &info, SPDRP_FRIENDLYNAME, nullptr,
                                               reinterpret_cast<PBYTE>(buf), sizeof(buf), &size)) {
            continue;
        }
        std::wstring friendly = trim_ws(buf);
        if (friendly.empty()) {
            continue;
        }

        std::wstring port = extract_com_port(friendly);
        if (port.empty()) {
            continue;
        }

        SerialPortInfo info_item;
        info_item.device = port;
        info_item.description = friendly;
        ports.push_back(info_item);
    }

    SetupDiDestroyDeviceInfoList(devs);

    std::sort(ports.begin(), ports.end(), [](const SerialPortInfo& a, const SerialPortInfo& b) {
        return a.device < b.device;
    });

    return ports;
}

bool SerialManager::is_connected() const {
    return handle_ != INVALID_HANDLE_VALUE;
}

void SerialManager::close_handle() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
    }
    handle_ = INVALID_HANDLE_VALUE;
}

bool SerialManager::connect(const std::wstring& port,
                            int baud,
                            int data_bits,
                            int parity,
                            int stop_bits,
                            std::wstring* error) {
    disconnect();

    std::wstring path = format_port_path(port);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (error) {
            *error = L"Open failed";
        }
        return false;
    }

    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        if (error) {
            *error = L"GetCommState failed";
        }
        CloseHandle(h);
        return false;
    }

    dcb.BaudRate = static_cast<DWORD>(baud);
    dcb.ByteSize = static_cast<BYTE>(data_bits);
    dcb.Parity = static_cast<BYTE>(parity);
    dcb.StopBits = static_cast<BYTE>(stop_bits);
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(h, &dcb)) {
        if (error) {
            *error = L"SetCommState failed";
        }
        CloseHandle(h);
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(h, &timeouts);

    SetupComm(h, kMaxRxBuffer, kMaxRxBuffer);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle_ = h;
    rx_buffer_.clear();
    rx_overflow_ = 0;
    return true;
}

void SerialManager::disconnect() {
    close_handle();
    rx_buffer_.clear();
    rx_overflow_ = 0;
}

std::vector<std::string> SerialManager::read_lines(std::wstring* error) {
    std::vector<std::string> lines;
    if (!is_connected()) {
        return lines;
    }

    DWORD errors = 0;
    COMSTAT stat = {};
    if (!ClearCommError(handle_, &errors, &stat)) {
        if (error) {
            *error = L"COM error";
        }
        disconnect();
        return lines;
    }

    DWORD to_read = stat.cbInQue > 0 ? stat.cbInQue : 1;
    std::string buffer;
    buffer.resize(to_read);

    DWORD read = 0;
    if (!ReadFile(handle_, buffer.data(), to_read, &read, nullptr)) {
        if (error) {
            *error = L"Read failed";
        }
        disconnect();
        return lines;
    }
    if (read == 0) {
        return lines;
    }

    buffer.resize(read);
    rx_buffer_ += buffer;
    if (static_cast<int>(rx_buffer_.size()) > kMaxRxBuffer) {
        int overflow = static_cast<int>(rx_buffer_.size()) - kMaxRxBuffer;
        rx_overflow_ += overflow;
        rx_buffer_.erase(0, overflow);
    }

    size_t pos = 0;
    while ((pos = rx_buffer_.find('\n')) != std::string::npos) {
        std::string line = rx_buffer_.substr(0, pos);
        rx_buffer_.erase(0, pos + 1);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

int SerialManager::consume_rx_overflow() {
    int count = rx_overflow_;
    rx_overflow_ = 0;
    return count;
}
