#include "mfc_main_dialog.h"

namespace {
std::wstring read(const std::wstring& path, const wchar_t* key, const wchar_t* fallback) {
    wchar_t value[1024] = {}; GetPrivateProfileStringW(L"Serial", key, fallback, value, 1024, path.c_str()); return value;
}
void select(HWND control, const std::wstring& text) {
    const auto index = ::SendMessageW(control, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str()));
    ::SendMessageW(control, CB_SETCURSEL, index, 0);
}
}

void CMainDialog::load_serial_settings() {
    if (connection_.mode() != SerialConnectionState::Mode::Idle) return;
    SerialSettings value;
    value.port.device = read(settings_path_, L"Port", L""); value.port.instance_id = read(settings_path_, L"DeviceInstanceId", L"");
    value.baud = static_cast<int>(GetPrivateProfileIntW(L"Serial", L"Baud", 115200, settings_path_.c_str()));
    value.data_bits = static_cast<int>(GetPrivateProfileIntW(L"Serial", L"DataBits", 8, settings_path_.c_str()));
    const auto parity = read(settings_path_, L"Parity", L"NONE"), stop = read(settings_path_, L"StopBits", L"1");
    value.parity = parity == L"NONE" ? NOPARITY : parity == L"ODD" ? ODDPARITY : parity == L"EVEN" ? EVENPARITY : -1;
    value.stop_bits = stop == L"1" ? ONESTOPBIT : stop == L"1.5" ? ONE5STOPBITS : stop == L"2" ? TWOSTOPBITS : -1;
    if (!valid_serial_settings(value)) value = SerialSettings{};
    remembered_serial_ = value; selection_hint_ = value.port;
    ::SetWindowTextW(combo_baud_, std::to_wstring(value.baud).c_str());
    select(combo_data_, std::to_wstring(value.data_bits)); select(combo_parity_, value.parity == ODDPARITY ? L"ODD" : value.parity == EVENPARITY ? L"EVEN" : L"NONE");
    select(combo_stop_, value.stop_bits == ONE5STOPBITS ? L"1.5" : value.stop_bits == TWOSTOPBITS ? L"2" : L"1");
    ::SendMessageW(combo_port_, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    if (!known_ports_.empty()) update_port_combo(known_ports_);
}

bool CMainDialog::save_serial_settings() {
    if (!valid_serial_settings(remembered_serial_)) return true; // Never save a failed/unconfirmed choice.
    const auto write = [&](const wchar_t* key, const std::wstring& value) {
        return WritePrivateProfileStringW(L"Serial", key, value.c_str(), settings_path_.c_str()) != FALSE;
    };
    const auto& value = remembered_serial_;
    bool ok = write(L"Port", value.port.device);
    ok = write(L"DeviceInstanceId", value.port.instance_id) && ok;
    ok = write(L"Baud", std::to_wstring(value.baud)) && ok;
    ok = write(L"DataBits", std::to_wstring(value.data_bits)) && ok;
    ok = write(L"Parity", value.parity == ODDPARITY ? L"ODD" : value.parity == EVENPARITY ? L"EVEN" : L"NONE") && ok;
    return write(L"StopBits", value.stop_bits == ONE5STOPBITS ? L"1.5" : value.stop_bits == TWOSTOPBITS ? L"2" : L"1") && ok;
}
