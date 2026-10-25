#include "serial_manager.h"
#include "serial_control_config.h"
#include "serial_native_io.h"

namespace {
std::wstring error_text(const wchar_t* operation, DWORD code = GetLastError()) {
    return std::wstring(operation) + L" failed (Windows error " + std::to_wstring(code) + L")";
}
}

bool SerialManager::open(const SerialSettings& settings, std::wstring* error) {
    if (!valid_serial_settings(settings)) { if (error) *error = L"Invalid serial settings or flow control"; return false; }
    controls_ = settings.controls;
    const auto& path = settings.port.open_path.empty() ? settings.port.device : settings.port.open_path;
    return connect(path, settings.baud, settings.data_bits, settings.parity, settings.stop_bits, error);
}

size_t SerialManager::write_bytes(std::string_view bytes, std::wstring* error) {
    if (!connected_ || bytes.empty() || bytes.size() > 4096) { if (error) *error = L"TX requires a live connection and 1..4096 bytes"; return 0; }
    // The session owner never closes the handle until BOTH RX and operation workers exit.
    // Do not hold the receive/configuration mutex during a potentially flow-controlled TX.
    DWORD written = 0, io_error = 0;
    const bool ok = serial_transfer(handle_, true, const_cast<char*>(bytes.data()), static_cast<DWORD>(bytes.size()), written, io_cancelled_, io_error);
    if (!ok || written != bytes.size()) {
        if (error) *error = error_text(L"TX", ok ? ERROR_TIMEOUT : io_error) + L"; " + std::to_wstring(written) +
            L" bytes reported accepted; delivery may be incomplete. Nothing is retried automatically.";
    }
    return written;
}

void SerialManager::cancel_io() {
    io_cancelled_ = true;
    // Called by the session owner, repeatedly while stopping; handle lifetime is stable.
    if (handle_ != INVALID_HANDLE_VALUE) CancelIoEx(handle_, nullptr);
}

bool SerialManager::set_controls(const SerialControls& value, std::wstring* error) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    DCB dcb{}; dcb.DCBlength = sizeof(dcb);
    if (!connected_ || io_cancelled_ || !valid_serial_controls(value)) { if (error) *error = L"No connection, stopping, or invalid flow control"; return false; }
    if (!GetCommState(handle_, &dcb)) { if (error) *error = error_text(L"GetCommState"); return false; }
    if (io_cancelled_) { if (error) *error = L"UART control update cancelled before SetCommState"; return false; }
    configure_serial_dcb(dcb, value);
    if (!SetCommState(handle_, &dcb)) { if (error) *error = error_text(L"SetCommState"); return false; }
    controls_ = value; return true;
}

bool SerialManager::send_break(std::wstring* error) {
    if (!connected_ || io_cancelled_) { if (error) *error = L"UART is disconnected or stopping"; return false; }
    if (!SetCommBreak(handle_)) { if (error) *error = error_text(L"SetCommBreak"); return false; }
    Sleep(250); // Explicit one-shot signal only; always attempt to release it.
    if (!ClearCommBreak(handle_)) { if (error) *error = error_text(L"ClearCommBreak"); return false; }
    return true;
}

SerialLineStatus SerialManager::sample_status() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    SerialLineStatus status; status.errors = pending_errors_; pending_errors_ = 0;
    DWORD modem = 0; status.modem_valid = connected_ && GetCommModemStatus(handle_, &modem);
    status.modem = modem; return status;
}
