#include "message_buffer.h"
#include <algorithm>
#include <cwchar>

MessageBuffer::MessageBuffer(size_t capacity) : capacity_(std::max<size_t>(1, capacity)) {}

void MessageBuffer::reset() { bytes_.clear(); total_ = skipped_ = 0; }

void MessageBuffer::append(std::string_view bytes, uint64_t skipped) {
    if (skipped) {
        // Do not silently concatenate old and new bytes across a lost-input gap.
        bytes_.clear(); total_ += skipped; skipped_ += skipped;
    }
    total_ += bytes.size();
    if (bytes.empty()) return;
    if (bytes.size() >= capacity_) {
        bytes_.assign(bytes.substr(bytes.size() - capacity_));
        return;
    }
    if (bytes_.size() + bytes.size() > capacity_) {
        const size_t drop = std::min(bytes_.size(), std::max(capacity_ / 4, bytes_.size() + bytes.size() - capacity_));
        bytes_.erase(0, drop);
    }
    bytes_.append(bytes.data(), bytes.size());
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
