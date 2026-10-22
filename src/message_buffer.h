#pragma once
#include <cstdint>
#include <string>
#include <string_view>

// Byte-preserving, UI-independent receive history. No text decoding or filtering.
class MessageBuffer {
public:
    static constexpr size_t kCapacity = 256 * 1024;
    explicit MessageBuffer(size_t capacity = kCapacity);
    void append(std::string_view bytes, uint64_t skipped = 0);
    void reset();
    const std::string& bytes() const { return bytes_; }
    uint64_t offset() const { return total_ - bytes_.size(); }
    uint64_t skipped() const { return skipped_; }
    std::wstring ascii(size_t start = 0) const;
    std::wstring hex(size_t start = 0) const;
private:
    size_t capacity_;
    uint64_t total_ = 0, skipped_ = 0;
    std::string bytes_;
};
