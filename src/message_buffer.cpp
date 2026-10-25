#include "message_buffer.h"
#include <algorithm>
#include <cwchar>

MessageBuffer::MessageBuffer(size_t capacity) : capacity_(std::max<size_t>(1, capacity)) {}

void MessageBuffer::reset() { bytes_.clear(); total_ = skipped_ = 0; receipts_.clear(); origin_ms_ = 0; }

void MessageBuffer::append(std::string_view bytes, uint64_t skipped, int64_t unix_ms) {
    if (!unix_ms) unix_ms = message_unix_ms();
    if (!origin_ms_) origin_ms_ = unix_ms;
    if (skipped) {
        // Do not silently concatenate old and new bytes across a lost-input gap.
        bytes_.clear(); receipts_.clear(); total_ += skipped; skipped_ += skipped;
    }
    if (!bytes.empty()) receipts_.push_back({total_, unix_ms});
    total_ += bytes.size();
    if (bytes.empty()) return;
    if (bytes.size() >= capacity_) {
        bytes_.assign(bytes.substr(bytes.size() - capacity_));
    } else {
      if (bytes_.size() + bytes.size() > capacity_) {
        const size_t drop = std::min(bytes_.size(), std::max(capacity_ / 4, bytes_.size() + bytes.size() - capacity_));
        bytes_.erase(0, drop);
      }
      bytes_.append(bytes.data(), bytes.size());
    }
    // Retain the timestamp covering the first remaining byte, plus newer boundaries.
    const auto it = std::upper_bound(receipts_.begin(), receipts_.end(), offset(),
        [](uint64_t at, const MessageReceipt& r) { return at < r.offset; });
    if (it != receipts_.begin()) receipts_.erase(receipts_.begin(), it - 1);
}

void MessageBuffer::append_history(const MessageBuffer& source) {
    if (&source == this) return;
    if (source.bytes().empty()) { if (source.offset()) append({}, source.offset()); return; }
    const auto& marks = source.receipts();
    size_t begin = 0; bool first = true;
    for (size_t i = 0; i < marks.size(); ++i) {
        const auto end = i + 1 < marks.size() ? static_cast<size_t>(marks[i + 1].offset - source.offset()) : source.bytes().size();
        if (end > begin && end <= source.bytes().size()) {
            append(std::string_view(source.bytes()).substr(begin, end - begin), first ? source.offset() : 0, marks[i].unix_ms);
            first = false; begin = end;
        }
    }
    if (marks.empty()) append(source.bytes(), source.offset());
}

std::wstring MessageBuffer::ascii(size_t start) const {
    static constexpr wchar_t digits[] = L"0123456789ABCDEF";
    std::wstring text;
    for (size_t i = start; i < bytes_.size(); ++i) {
        const auto byte = static_cast<unsigned char>(bytes_[i]);
        if (byte == '\r') text += L"\r\n";
        else if (byte == '\n') { if (!i || bytes_[i - 1] != '\r') text += L"\r\n"; }
        else if (byte == '\t' || (byte >= 32 && byte < 127)) text.push_back(static_cast<wchar_t>(byte));
        else { text += L"\\x"; text.push_back(digits[byte >> 4]); text.push_back(digits[byte & 15]); }
    }
    return text;
}

std::wstring MessageBuffer::hex(size_t start) const {
    static constexpr wchar_t digits[] = L"0123456789ABCDEF";
    std::wstring text;
    for (size_t i = start; i < bytes_.size(); i += 16) {
        wchar_t address[32] = {};
        std::swprintf(address, 32, L"%08llX  ", static_cast<unsigned long long>(offset() + i));
        text += address;
        for (size_t j = i; j < std::min(i + 16, bytes_.size()); ++j) {
            if (j != i) text.push_back(L' ');
            const auto byte = static_cast<unsigned char>(bytes_[j]);
            text.push_back(digits[byte >> 4]); text.push_back(digits[byte & 15]);
        }
        text += L"\r\n";
    }
    return text;
}
