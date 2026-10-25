#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

enum class MessageEncoding { Ascii, Hex, Utf8, Ansi };
enum class MessageClock { None, Local, Elapsed, Delta };
enum class MessageNewline { Auto, LF, CR, CRLF };
struct MessageDisplayOptions {
    MessageEncoding encoding = MessageEncoding::Ascii;
    MessageClock clock = MessageClock::None;
    MessageNewline newline = MessageNewline::Auto;
    unsigned code_page = 0;
    bool ansi_sgr = false;
};
struct MessageReceipt { uint64_t offset = 0; int64_t unix_ms = 0; };
struct MessageColor { size_t start = 0, count = 0; uint32_t color = 0x0000ff00; };
struct FormattedMessage { std::wstring text; std::vector<MessageColor> colors; };
int64_t message_unix_ms();
std::string message_utf8(std::wstring_view text);
FormattedMessage format_message(std::string_view bytes, uint64_t offset,
    const std::vector<MessageReceipt>& receipts, const MessageDisplayOptions& options,
    int64_t origin_ms = 0);
struct MessageMatch { size_t start = 0, length = 0; };
std::vector<MessageMatch> find_message_matches(const std::wstring& text, const std::wstring& pattern,
    bool regex, bool match_case, std::wstring& error);
