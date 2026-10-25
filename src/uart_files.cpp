#include "uart_files.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

std::string uart_read_file(const std::filesystem::path& path, size_t limit, const std::atomic<bool>& cancel) {
    if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Input must be a regular disk file, not a device or directory");
    std::ifstream stream(path, std::ios::binary); if (!stream) throw std::runtime_error("Cannot open input file");
    std::string result; char chunk[65536];
    while (stream) {
        if (cancel) throw std::runtime_error("Cancelled");
        stream.read(chunk, sizeof(chunk)); const auto size = static_cast<size_t>(stream.gcount());
        if (size > limit - result.size()) throw std::runtime_error("Input exceeds the documented size limit");
        result.append(chunk, size);
    }
    if (!stream.eof()) throw std::runtime_error("Input read failed");
    return result;
}
void uart_write_file(const std::filesystem::path& path, std::string_view bytes, bool replace, const std::atomic<bool>& cancel) {
    static std::atomic<unsigned> sequence{0};
    const auto temp = path.wstring() + L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence);
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create temporary output beside destination");
    bool ok = true;
    for (size_t offset = 0; ok && offset < bytes.size();) {
        if (cancel) { ok = false; break; }
        const auto count = static_cast<DWORD>(std::min<size_t>(65536, bytes.size() - offset)); DWORD written = 0;
        ok = WriteFile(file, bytes.data() + offset, count, &written, nullptr) && written == count;
        offset += written;
    }
    ok = ok && !cancel && FlushFileBuffers(file); CloseHandle(file);
    if (ok && !cancel) ok = MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0)) != FALSE;
    else ok = false;
    if (!ok) { DeleteFileW(temp.c_str()); throw std::runtime_error("Save cancelled or failed; destination not replaced"); }
}
std::string uart_hex(std::string_view value) {
    static const char digits[] = "0123456789ABCDEF"; std::string result; result.reserve(value.size() * 2);
    for (const unsigned char c : value) { result += digits[c >> 4]; result += digits[c & 15]; } return result;
}
bool uart_unhex(std::string_view value, std::string& result) {
    if (value.size() % 2) return false;
    const auto digit = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; };
    std::string output;
    for (size_t i = 0; i < value.size(); i += 2) { const int a = digit(value[i]), b = digit(value[i+1]); if (a < 0 || b < 0) return false; output += static_cast<char>((a << 4) | b); }
    result = std::move(output); return true;
}
std::wstring uart_wide(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Invalid UTF-8 text");
    std::wstring result(size, 0); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), size); return result;
}
bool uart_number(const std::wstring& text, double low, double high, double& value) {
    if (text.empty() || text.size() > 64) return false;
    wchar_t* end = nullptr; value = wcstod(text.c_str(), &end);
    return end == text.c_str() + text.size() && std::isfinite(value) && value >= low && value <= high;
}
