#include "ini_settings.h"
#include "uart_files.h"
#include <windows.h>
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace {
constexpr size_t kIniLimit = 1024 * 1024;
std::mutex settings_mutex;
std::atomic<unsigned> sequence{0};
struct File {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~File() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    void close() {
        if (value != INVALID_HANDLE_VALUE && !CloseHandle(value)) throw std::runtime_error("Cannot close INI input");
        value = INVALID_HANDLE_VALUE;
    }
};
bool same_file(const BY_HANDLE_FILE_INFORMATION& x, HANDLE b) {
    BY_HANDLE_FILE_INFORMATION y{};
    return GetFileInformationByHandle(b, &y) &&
        x.dwVolumeSerialNumber == y.dwVolumeSerialNumber && x.nFileIndexHigh == y.nFileIndexHigh &&
        x.nFileIndexLow == y.nFileIndexLow && x.nFileSizeHigh == y.nFileSizeHigh &&
        x.nFileSizeLow == y.nFileSizeLow && CompareFileTime(&x.ftLastWriteTime, &y.ftLastWriteTime) == 0;
}
}

bool save_ini_patch(const std::wstring& path, const std::vector<IniSetting>& values,
                    std::wstring& error, const std::function<void(size_t)>& before_step) {
    std::lock_guard<std::mutex> lock(settings_mutex);
    error.clear();
    if (path.empty()) { error = L"INI path is empty."; return false; }
    if (values.empty()) return true;
    const auto staging = path + L".settings-stage-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence);
    const std::atomic<bool> cancel{false};
    bool staged = false;
    try {
        WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
        File original;
        original.value = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        const auto open_error = GetLastError();
        if (original.value == INVALID_HANDLE_VALUE && open_error != ERROR_FILE_NOT_FOUND)
            throw std::runtime_error("Cannot read existing INI (Win32 " + std::to_string(open_error) + ")");
        std::string bytes = "\xff\xfe";
        const bool existed = original.value != INVALID_HANDLE_VALUE;
        BY_HANDLE_FILE_INFORMATION identity{};
        if (original.value != INVALID_HANDLE_VALUE) {
            if (!GetFileInformationByHandle(original.value, &identity)) throw std::runtime_error("Cannot identify existing INI");
            LARGE_INTEGER size{};
            if (!GetFileSizeEx(original.value, &size) || size.QuadPart < 0 || size.QuadPart > kIniLimit)
                throw std::runtime_error("INI exceeds 1 MiB");
            bytes.resize(static_cast<size_t>(size.QuadPart));
            DWORD read = 0;
            if (!ReadFile(original.value, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size())
                throw std::runtime_error("Cannot read complete INI");
        }
        const auto original_bytes = bytes;
        original.close();
        if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xff && static_cast<unsigned char>(bytes[1]) == 0xfe && bytes.size() % 2)
            throw std::runtime_error("Truncated UTF-16 INI; preserve and review encoding");
        if (bytes.rfind("\xfe\xff", 0) == 0) throw std::runtime_error("Unsupported big-endian INI; preserve and review encoding");
        if (bytes.size() < 2 || static_cast<unsigned char>(bytes[0]) != 0xff || static_cast<unsigned char>(bytes[1]) != 0xfe) {
            // Existing profile files without a Unicode BOM use the Windows ANSI code page.
            if (bytes.rfind("\xef\xbb\xbf", 0) == 0) throw std::runtime_error("Unsupported UTF-8 INI; preserve and review encoding");
            const int count = MultiByteToWideChar(CP_ACP, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
            if (!bytes.empty() && !count) throw std::runtime_error("Cannot decode existing ANSI INI");
            std::wstring decoded(static_cast<size_t>(count), L'\0');
            if (count) MultiByteToWideChar(CP_ACP, 0, bytes.data(), static_cast<int>(bytes.size()), decoded.data(), count);
            bytes = "\xff\xfe"; bytes.append(reinterpret_cast<const char*>(decoded.data()), decoded.size() * sizeof(wchar_t));
        }
        uart_write_file(staging, bytes, false, cancel); staged = true;
        for (size_t i = 0; i < values.size(); ++i) {
            if (before_step) before_step(i);
            const auto& value = values[i];
            if (value.section.empty() || value.key.empty() || value.value.find_first_of(L"\r\n") != std::wstring::npos ||
                !WritePrivateProfileStringW(value.section.c_str(), value.key.c_str(), value.value.c_str(), staging.c_str()))
                throw std::runtime_error("Cannot stage INI setting");
        }
        // Cache invalidation also returns zero on success; disk flush is checked below.
        WritePrivateProfileStringW(nullptr, nullptr, nullptr, staging.c_str());
        (void)uart_read_file(staging, kIniLimit, cancel);
        File output;
        output.value = CreateFileW(staging.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output.value == INVALID_HANDLE_VALUE || !FlushFileBuffers(output.value))
            throw std::runtime_error("Cannot flush staged INI file");
        if (!CloseHandle(output.value)) throw std::runtime_error("Cannot close staged INI file");
        output.value = INVALID_HANDLE_VALUE;
        if (before_step) before_step(values.size());
        File current;
        current.value = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (!existed) {
            if (current.value != INVALID_HANDLE_VALUE || GetLastError() != ERROR_FILE_NOT_FOUND)
                throw std::runtime_error("INI changed during save");
        } else if (current.value == INVALID_HANDLE_VALUE || !same_file(identity, current.value)) {
            throw std::runtime_error("INI changed during save");
        } else {
            std::string latest(original_bytes.size(), '\0'); DWORD read = 0;
            if (!ReadFile(current.value, latest.data(), static_cast<DWORD>(latest.size()), &read, nullptr) ||
                read != latest.size() || latest != original_bytes) throw std::runtime_error("INI changed during save");
        }
        // Windows replacement requires target handles released. Reject detected edits before publication.
        current.close();
        if (!MoveFileExW(staging.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH |
            (existed ? MOVEFILE_REPLACE_EXISTING : 0)))
            throw std::runtime_error("Cannot replace INI atomically (Win32 " + std::to_string(GetLastError()) + ")");
        staged = false;
        WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
        return true;
    } catch (const std::exception& failure) {
        const std::string detail = failure.what();
        error = L"Cannot save INI: " + path + L" (" + std::wstring(detail.begin(), detail.end()) + L").";
    }
    if (staged && !DeleteFileW(staging.c_str())) error += L" Staging file retained: " + staging;
    return false;
}

CaptureSettingsInput load_capture_settings(const std::wstring& path) {
    const auto read = [&](const wchar_t* key, const wchar_t* fallback) {
        std::wstring value(4096, L'\0');
        value.resize(GetPrivateProfileStringW(L"Capture", key, fallback, value.data(), static_cast<DWORD>(value.size()), path.c_str()));
        return value;
    };
    return {read(L"Include", L""), read(L"Exclude", L""), read(L"Regex", L"0"), read(L"MaxChannels", L"256")};
}

bool validate_capture_settings(const CaptureSettingsInput& input, CaptureSettings& output, std::wstring& error) {
    if (input.include.size() > 256 || input.exclude.size() > 256) { error = L"Saved filter is too long; review Capture settings."; return false; }
    if (input.regex != L"0" && input.regex != L"1") { error = L"Invalid saved Regex mode; review Capture settings."; return false; }
    if (input.max_channels.empty() || input.max_channels.size() > 4 || input.max_channels.find_first_not_of(L"0123456789") != std::wstring::npos) {
        error = L"Max CH must be 1..1024. Previous filter remains active."; return false;
    }
    const auto limit = std::stoi(input.max_channels);
    if (limit < 1 || limit > 1024) { error = L"Max CH must be 1..1024. Previous filter remains active."; return false; }
    output = {input.include, input.exclude, input.regex == L"1", limit};
    error.clear(); return true;
}

std::vector<IniSetting> CaptureSettings::patch() const {
    return {{L"Capture", L"Include", include}, {L"Capture", L"Exclude", exclude},
        {L"Capture", L"Regex", regex ? L"1" : L"0"}, {L"Capture", L"MaxChannels", std::to_wstring(max_channels)}};
}
