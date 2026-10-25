#include "uart_preferences.h"
#include <windows.h>
#include <algorithm>

namespace {
std::wstring read(const std::wstring& ini, const wchar_t* section, const std::wstring& key) {
    std::wstring value(32768, 0);
    const auto n = GetPrivateProfileStringW(section, key.c_str(), L"", value.data(), static_cast<DWORD>(value.size()), ini.c_str());
    if (n >= value.size() - 1) return L"<truncated>"; value.resize(n); return value;
}
}
std::wstring encode_tx_entry(const TxEntry& value) {
    std::string payload; std::wstring error; if (!make_tx_payload(value, payload, error)) return {};
    std::wstring text = value.text;
    if (value.hex) {
        const size_t suffix = value.ending == TxEnding::CRLF ? 2 : value.ending == TxEnding::None ? 0 : 1;
        payload.resize(payload.size() - suffix); text = tx_preview(payload, 4096);
    }
    const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string encoded(n, 0); if (n) WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), encoded.data(), n, nullptr, nullptr);
    static const wchar_t digits[] = L"0123456789ABCDEF";
    std::wstring result = value.hex ? L"H" : L"T"; result += static_cast<wchar_t>(L'0' + static_cast<int>(value.ending)); result += L':';
    for (const auto c : encoded) { const auto b = static_cast<unsigned char>(c); result += digits[b >> 4]; result += digits[b & 15]; }
    return result.size() < 32760 ? result : L"";
}
bool decode_tx_entry(const std::wstring& encoded, TxEntry& value) {
    if (encoded.size() < 3 || (encoded[0] != L'H' && encoded[0] != L'T') || encoded[1] < L'0' || encoded[1] > L'3' || encoded[2] != L':') return false;
    TxEntry hex; hex.hex = true; hex.text = encoded.substr(3);
    std::string bytes; std::wstring error;
    // Storage can exceed the wire-payload limit (whitespace in a HEX editor).
    if (hex.text.size() > 32756 || hex.text.size() % 2) return false;
    for (size_t i = 0; i < hex.text.size(); i += 2) {
        const auto nibble = [](wchar_t c) { return c >= L'0' && c <= L'9' ? c - L'0' : c >= L'A' && c <= L'F' ? c - L'A' + 10 : -1; };
        const int a = nibble(hex.text[i]), b = nibble(hex.text[i + 1]); if (a < 0 || b < 0) return false;
        bytes += static_cast<char>((a << 4) | b);
    }
    const int n = bytes.empty() ? 0 : MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (!bytes.empty() && !n) return false;
    TxEntry result; result.hex = encoded[0] == L'H'; result.ending = static_cast<TxEnding>(encoded[1] - L'0'); result.text.resize(n);
    if (n) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), result.text.data(), n);
    if (result.text.find(L'\0') != std::wstring::npos) return false;
    if (!make_tx_payload(result, bytes, error)) return false; value = std::move(result); return true;
}
SerialControls load_uart_controls(const std::wstring& ini, std::wstring& error) {
    error.clear(); SerialControls value;
    const DWORD attributes = GetFileAttributesW(ini.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) error = L"Cannot read serial controls INI: " + ini;
        return value;
    }
    const HANDLE file = CreateFileW(ini.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file == INVALID_HANDLE_VALUE) { error = L"Cannot read serial controls INI: " + ini; return value; }
    CloseHandle(file);
    const auto encoded = read(ini, L"SerialControls", L"Configuration");
    if (encoded.empty()) return value;
    if (encoded.size() != 8 || encoded.substr(0, 3) != L"v1:" || encoded[3] < L'0' || encoded[3] > L'3' || encoded[4] != ':' ||
        (encoded[5] != '0' && encoded[5] != '1') || encoded[6] != ':' || (encoded[7] != '0' && encoded[7] != '1')) {
        error = L"Invalid SerialControls configuration; using None / DTR on / RTS on. Review before connecting."; return value;
    }
    value.flow = static_cast<SerialFlow>(encoded[3] - L'0'); value.dtr = encoded[5] == L'1'; value.rts = encoded[7] == L'1'; return value;
}
bool save_uart_controls(const std::wstring& ini, const SerialControls& value) {
    if (!valid_serial_controls(value)) return false;
    const auto encoded = L"v1:" + std::to_wstring(static_cast<int>(value.flow)) + L":" + (value.dtr ? L"1" : L"0") + L":" + (value.rts ? L"1" : L"0");
    return WritePrivateProfileStringW(L"SerialControls", L"Configuration", encoded.c_str(), ini.c_str()) != FALSE;
}
std::vector<TxEntry> load_tx_history(const std::wstring& ini) {
    std::vector<TxEntry> result;
    const auto count = std::min<UINT>(32, GetPrivateProfileIntW(L"UartConsole", L"HistoryCount", 0, ini.c_str()));
    for (UINT i = 0; i < count; ++i) { TxEntry value; if (decode_tx_entry(read(ini, L"UartConsole", L"History" + std::to_wstring(i)), value)) result.push_back(std::move(value)); }
    return result;
}
bool save_tx_history(const std::wstring& ini, const std::vector<TxEntry>& history) {
    if (history.size() > 32) return false; bool ok = true;
    for (size_t i = 0; i < 32; ++i) {
        const auto key = L"History" + std::to_wstring(i); const auto encoded = i < history.size() ? encode_tx_entry(history[i]) : L"";
        if (i < history.size() && encoded.empty()) return false;
        ok = WritePrivateProfileStringW(L"UartConsole", key.c_str(), i < history.size() ? encoded.c_str() : nullptr, ini.c_str()) && ok;
    }
    return WritePrivateProfileStringW(L"UartConsole", L"HistoryCount", std::to_wstring(history.size()).c_str(), ini.c_str()) && ok;
}
std::array<TxEntry, 8> load_tx_presets(const std::wstring& ini) {
    std::array<TxEntry, 8> result;
    for (size_t i = 0; i < result.size(); ++i) decode_tx_entry(read(ini, L"UartConsole", L"Preset" + std::to_wstring(i)), result[i]);
    return result;
}
bool save_tx_preset(const std::wstring& ini, size_t slot, const TxEntry* value) {
    if (slot >= 8) return false; const auto encoded = value ? encode_tx_entry(*value) : L"";
    if (value && encoded.empty()) return false;
    return WritePrivateProfileStringW(L"UartConsole", (L"Preset" + std::to_wstring(slot)).c_str(), value ? encoded.c_str() : nullptr, ini.c_str()) != FALSE;
}
