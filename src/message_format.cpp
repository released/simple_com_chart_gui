#include "message_format.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cwchar>
#include <regex>

int64_t message_unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string message_utf8(std::wstring_view text) {
    if (text.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), n, nullptr, nullptr);
    return result;
}

namespace {
void escaped(std::wstring& out, unsigned char byte) {
    static const wchar_t hex[] = L"0123456789ABCDEF";
    out += L"\\x"; out += hex[byte >> 4]; out += hex[byte & 15];
}
std::wstring timestamp(int64_t ms, int64_t origin, int64_t previous, MessageClock mode) {
    wchar_t value[96]{};
    if (mode == MessageClock::Local) {
        ULARGE_INTEGER ticks{}; ticks.QuadPart = static_cast<uint64_t>(ms + 11644473600000LL) * 10000;
        FILETIME ft{ticks.LowPart, ticks.HighPart}, local{}; SYSTEMTIME st{};
        FileTimeToLocalFileTime(&ft, &local); FileTimeToSystemTime(&local, &st);
        swprintf_s(value, L"[%02u:%02u:%02u.%03u] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    } else if (mode != MessageClock::None) {
        const double seconds = static_cast<double>(ms - (mode == MessageClock::Delta ? previous : origin)) / 1000.0;
        swprintf_s(value, mode == MessageClock::Delta ? L"[dt %+.3fs] " : L"[%.3fs] ", seconds);
    }
    return value;
}
}

FormattedMessage format_message(std::string_view bytes, uint64_t offset,
    const std::vector<MessageReceipt>& receipts, const MessageDisplayOptions& options, int64_t origin) {
    FormattedMessage result;
    if (bytes.empty()) return result;
    size_t receipt = 0;
    const auto time_at = [&](size_t i) {
        while (receipt + 1 < receipts.size() && receipts[receipt + 1].offset <= offset + i) ++receipt;
        return receipts.empty() ? origin : receipts[receipt].unix_ms;
    };
    if (!origin) origin = time_at(0);
    int64_t previous = time_at(0);
    bool line_start = true;
    COLORREF color = RGB(0, 255, 0);
    size_t color_start = 0;
    const auto change_color = [&](COLORREF next) {
        if (result.text.size() > color_start) result.colors.push_back({color_start, result.text.size() - color_start, color});
        color_start = result.text.size(); color = next;
    };
    const COLORREF palette[] = {RGB(0,0,0), RGB(220,70,70), RGB(0,255,0), RGB(230,210,70),
        RGB(100,140,255), RGB(220,100,220), RGB(80,220,220), RGB(220,220,220)};
    for (size_t i = 0; i < bytes.size();) {
        const auto now = time_at(i);
        if (line_start) { result.text += timestamp(now, origin, previous, options.clock); previous = now; line_start = false; }
        if (options.encoding == MessageEncoding::Hex) {
            wchar_t address[40]{}; swprintf_s(address, L"%08llX  ", static_cast<unsigned long long>(offset + i)); result.text += address;
            static const wchar_t hex[] = L"0123456789ABCDEF";
            const size_t end = std::min(bytes.size(), i + 16);
            for (; i < end; ++i) { const auto b = static_cast<unsigned char>(bytes[i]); result.text += hex[b >> 4]; result.text += hex[b & 15]; if (i + 1 < end) result.text += L' '; }
            result.text += L"\r\n"; line_start = true; continue;
        }
        const auto b = static_cast<unsigned char>(bytes[i]);
        if (b == 27 && options.ansi_sgr && i + 1 < bytes.size() && bytes[i + 1] == '[') {
            size_t end = i + 2;
            while (end < bytes.size() && end - i < 48 && ((bytes[end] >= '0' && bytes[end] <= '9') || bytes[end] == ';')) ++end;
            if (end < bytes.size() && bytes[end] == 'm') {
                unsigned code = 0;
                for (size_t k = i + 2; k <= end; ++k) {
                    if (bytes[k] >= '0' && bytes[k] <= '9') code = code * 10 + bytes[k] - '0';
                    else { if (code == 0 || code == 39) change_color(RGB(0,255,0)); else if (code >= 30 && code <= 37) change_color(palette[code - 30]); else if (code >= 90 && code <= 97) change_color(palette[code - 90]); code = 0; }
                }
                i = end + 1; continue;
            }
            // Keep an incomplete sequence pending until the next receive chunk.
            if (end == bytes.size() && end - i < 48) break;
        }
        const bool crlf = b == '\r' && i + 1 < bytes.size() && bytes[i + 1] == '\n';
        const bool newline = options.newline == MessageNewline::Auto ? (b == '\r' || b == '\n') :
            options.newline == MessageNewline::LF ? b == '\n' : options.newline == MessageNewline::CR ? b == '\r' : crlf;
        if (newline) { result.text += L"\r\n"; i += (crlf && (options.newline == MessageNewline::Auto || options.newline == MessageNewline::CRLF)) ? 2 : 1; line_start = true; continue; }
        if (b == '\r' && options.newline == MessageNewline::CRLF && i + 1 == bytes.size()) break;
        if (b == '\t' || (b >= 32 && b < 127)) { result.text += static_cast<wchar_t>(b); ++i; continue; }
        if (b >= 128 && options.encoding != MessageEncoding::Ascii) {
            size_t length = 1;
            if (options.encoding == MessageEncoding::Utf8) {
                length = b >= 0xC2 && b <= 0xDF ? 2 : b >= 0xE0 && b <= 0xEF ? 3 : b >= 0xF0 && b <= 0xF4 ? 4 : 1;
                if (length > bytes.size() - i) break;
            } else if (IsDBCSLeadByteEx(options.code_page, b)) { length = 2; if (i + 1 == bytes.size()) break; }
            wchar_t decoded[4]{};
            const unsigned cp = options.encoding == MessageEncoding::Utf8 ? CP_UTF8 : options.code_page;
            const int n = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, bytes.data() + i, static_cast<int>(length), decoded, 4);
            if (n > 0) { result.text.append(decoded, n); i += length; continue; }
        }
        escaped(result.text, b); ++i;
    }
    change_color(color);
    return result;
}

std::vector<MessageMatch> find_message_matches(const std::wstring& text, const std::wstring& pattern,
    bool regex, bool match_case, std::wstring& error) {
    error.clear(); std::vector<MessageMatch> matches;
    if (pattern.empty()) return matches;
    if (pattern.size() > 256) { error = L"Search is limited to 256 characters."; return matches; }
    try {
        if (regex) {
            // Deliberately limit the interactive subset: no quantified groups/backreferences,
            // at most one variable quantifier per alternative. Avoid pathological UI searches.
            unsigned quantifiers = 0, depth = 0; bool escaped_char = false, in_class = false;
            wchar_t prior = 0;
            for (const auto c : pattern) {
                if (escaped_char) { if (c >= L'1' && c <= L'9') throw std::regex_error(std::regex_constants::error_backref); escaped_char = false; prior = c; continue; }
                if (c == L'\\') { escaped_char = true; continue; }
                if (c == L'[') in_class = true;
                if (c == L']') in_class = false;
                if (!in_class && c == L'(') ++depth;
                if (!in_class && c == L')' && depth) --depth;
                if (!in_class && c == L'|') quantifiers = 0;
                if (!in_class && (c == L'*' || c == L'+' || c == L'?' || c == L'{')) {
                    if (depth || prior == L')' || ++quantifiers > 1 || c == L'{') {
                        error = L"Search regex: no quantifiers in/on groups or backreferences; one quantifier per alternative. Use Text for complex input."; return {};
                    }
                }
                prior = c;
            }
            const std::wregex expression(pattern, match_case ? std::regex_constants::ECMAScript :
                std::regex_constants::ECMAScript | std::regex_constants::icase);
            // Search bounded line slices, not one unbounded stream or multiline backtracking.
            for (size_t begin = 0; begin < text.size() && matches.size() < 4096;) {
                const auto newline = text.find_first_of(L"\r\n", begin);
                const auto end = std::min({text.size(), begin + 2048, newline == std::wstring::npos ? text.size() : newline});
                const auto line = text.substr(begin, end - begin);
                for (std::wsregex_iterator it(line.begin(), line.end(), expression), last; it != last && matches.size() < 4096; ++it)
                    if (it->length()) matches.push_back({begin + static_cast<size_t>(it->position()), static_cast<size_t>(it->length())});
                begin = end + ((end < text.size() && (text[end] == L'\r' || text[end] == L'\n')) ? 1 : 0);
            }
        } else {
            auto haystack = text, needle = pattern;
            if (!match_case) {
                std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
                std::transform(needle.begin(), needle.end(), needle.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
            }
            for (size_t pos = 0; matches.size() < 4096 && (pos = haystack.find(needle, pos)) != std::wstring::npos; pos += needle.size())
                matches.push_back({pos, needle.size()});
        }
    } catch (const std::regex_error&) { error = L"Invalid search regex."; }
    return matches;
}
