#include "serial_manager.h"
#include "serial_control_config.h"
#include "serial_native_io.h"

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <winioctl.h>

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
    if (port.rfind(L"\\\\.\\", 0) == 0 || port.rfind(L"\\\\?\\", 0) == 0) {
        return port;
    }
    if (port.size() > 3) {
        return L"\\\\.\\" + port;
    }
    return port;
}

std::wstring instance_id(HDEVINFO devices, SP_DEVINFO_DATA& info) {
    wchar_t value[1024] = {};
    return SetupDiGetDeviceInstanceIdW(devices, &info, value, 1024, nullptr) ? value : L"";
}

void attach_interface_paths(std::vector<SerialPortInfo>& ports) {
    HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) return;
    std::vector<unsigned> matches(ports.size(), 0);
    SP_DEVICE_INTERFACE_DATA item{}; item.cbSize = sizeof(item);
    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(devices, nullptr, &GUID_DEVINTERFACE_COMPORT, index, &item); ++index) {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(devices, &item, nullptr, 0, &size, nullptr);
        if (size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || size > 65536) continue;
        std::vector<unsigned char> buffer(size);
        auto detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data()); detail->cbSize = sizeof(*detail);
        SP_DEVINFO_DATA info{}; info.cbSize = sizeof(info);
        if (!SetupDiGetDeviceInterfaceDetailW(devices, &item, detail, size, nullptr, &info)) continue;
        const auto id = instance_id(devices, info);
        if (id.empty()) continue;
        for (size_t i = 0; i < ports.size(); ++i) if (same_serial_name(ports[i].instance_id, id)) {
            if (++matches[i] == 1) ports[i].open_path = detail->DevicePath;
            else ports[i].open_path.clear();
        }
    }
    SetupDiDestroyDeviceInfoList(devices);
}
} // namespace

SerialManager::SerialManager() = default;
SerialManager::~SerialManager() {
    disconnect();
}

std::vector<SerialPortInfo> SerialManager::scan_ports(bool* success) {
    if (success) *success = false;
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
        info_item.instance_id = instance_id(devs, info);
        ports.push_back(info_item);
    }
    const bool complete = GetLastError() == ERROR_NO_MORE_ITEMS;
    SetupDiDestroyDeviceInfoList(devs);
    if (!complete) return {};
    attach_interface_paths(ports);
    if (success) *success = true;

    std::sort(ports.begin(), ports.end(), [](const SerialPortInfo& a, const SerialPortInfo& b) {
        return a.device < b.device;
    });

    return ports;
}

bool SerialManager::is_connected() const {
    return connected_.load();
}

void SerialManager::close_handle() {
    connected_ = false;
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    disconnect();

    std::wstring path = format_port_path(port);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (error) {
            const DWORD code = GetLastError();
            *error = L"Open failed (" + std::to_wstring(code) + L")";
            wchar_t description[512] = {};
            if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                code, 0, description, 512, nullptr)) *error += L": " + trim_ws(description);
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
    dcb.fParity = parity != NOPARITY;
    configure_serial_dcb(dcb, controls_);

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
    timeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(h, &timeouts)) {
        if (error) *error = L"SetCommTimeouts failed";
        CloseHandle(h); return false;
    }

    SetupComm(h, kMaxRxBuffer, kMaxRxBuffer);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle_ = h;
    connected_ = true; pending_errors_ = 0; io_cancelled_ = false;
    rx_buffer_.clear();
    rx_overflow_ = 0;
    return true;
}

void SerialManager::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    close_handle();
    rx_buffer_.clear();
    rx_overflow_ = 0;
}

std::vector<std::string> SerialManager::read_lines(std::wstring* error, std::string* raw_bytes) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (raw_bytes) raw_bytes->clear();
    std::vector<std::string> lines;
    if (!is_connected()) {
        return lines;
    }

    DWORD errors = 0;
    COMSTAT stat = {};
    if (!ClearCommError(handle_, &errors, &stat)) {
        if (error) {
            *error = L"COM error (" + std::to_wstring(GetLastError()) + L")";
        }
        connected_ = false;
        return lines;
    }
    pending_errors_ |= errors;

    if (!stat.cbInQue) return lines;

    DWORD to_read = stat.cbInQue > 0 ? std::min<DWORD>(stat.cbInQue, 65536) : 1;
    std::string buffer;
    buffer.resize(to_read);

    DWORD read = 0, io_error = 0;
    if (!serial_transfer(handle_, false, buffer.data(), to_read, read, io_cancelled_, io_error)) {
        if (error) {
            *error = L"Read failed (" + std::to_wstring(io_error) + L")";
        }
        connected_ = false;
    }
    read = std::min(read, to_read);
    if (read == 0) {
        return lines;
    }

    buffer.resize(read);
    if (raw_bytes) *raw_bytes = buffer;
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
    return rx_overflow_.exchange(0);
}
