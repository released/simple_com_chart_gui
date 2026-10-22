#include "message_buffer.h"
#include <stdexcept>

namespace {
void check(bool ok, const char* name) { if (!ok) throw std::runtime_error(name); }
}

void test_messages() {
    MessageBuffer buffer;
    buffer.append(std::string("A\0\xFF\r", 4));
    const auto first = buffer.ascii();
    buffer.append("\nB\n\t");
    check(first + buffer.ascii(4) == buffer.ascii(), "ASCII split CRLF append preserves rendering");
    check(buffer.ascii() == L"A\\x00\\xFF\r\nB\r\n\t", "ASCII nonprintables are explicit and NUL does not truncate");
    check(buffer.hex() == L"00000000  41 00 FF 0D 0A 42 0A 09\r\n", "HEX preserves all raw bytes and line endings");
    check(buffer.bytes().size() == 8 && buffer.offset() == 0, "Formatting does not modify raw capture");
    MessageBuffer small(16);
    small.append("0123456789ABCDEF"); small.append("x");
    check(small.bytes() == "456789ABCDEFx" && small.offset() == 4, "Bounded buffer evicts in batches");
    small.append(std::string(32, 'z'));
    check(small.bytes() == std::string(16, 'z') && small.offset() == 33, "Oversize packet keeps bounded newest bytes");
    small.append("a", 7);
    check(small.bytes() == "a" && small.offset() == 56 && small.skipped() == 7, "Input gap clears old history and advances exact offset");
    check(small.hex() == L"00000038  61\r\n", "HEX offset reflects eviction and missing input");
    small.append({}, 2); check(small.bytes().empty() && small.skipped() == 9, "Empty loss notification is safe");
    small.reset(); check(small.bytes().empty() && small.offset() == 0 && small.skipped() == 0, "Clear resets raw counts");
    buffer.reset();
    buffer.append(std::string(MessageBuffer::kCapacity * 2, '\0'));
    check(buffer.bytes().size() == MessageBuffer::kCapacity && buffer.offset() == MessageBuffer::kCapacity,
        "Default history is bounded to 256 KiB");
    MessageBuffer empty(0); empty.append("ab"); check(empty.bytes() == "b", "Zero capacity is safely clamped");
}
