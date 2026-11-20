#include "message_lens.h"
#include <windows.h>
#include <psapi.h>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
uint64_t memory() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)))
        throw std::runtime_error("Cannot observe private bytes");
    return counters.PrivateUsage;
}
void measure(const char* name, const std::string& row, bool hide_nul) {
    std::string input;
    while (input.size() + row.size() <= MessageBuffer::kCapacity) input += row;
    MessageBuffer source;
    source.append(input, 0, 1000);
    MessageLens lens;
    std::string error;
    if (!lens.configure({}, error)) throw std::runtime_error(error);
    MessageDisplayOptions options;
    options.suppress_nul = hide_nul;
    const auto before = memory();
    const auto started = std::chrono::steady_clock::now();
    const auto& view = lens.update(source, options);
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    const auto after = memory();
    if (source.bytes() != input || view.rows.size() != input.size() / row.size())
        throw std::runtime_error("Dense-row projection changed raw bytes or logical row count");
    std::cout << "BENCH " << name << " bytes=" << input.size() << " rows=" << view.rows.size()
              << " elapsed_ms=" << elapsed << " private_before=" << before << " private_after=" << after
              << " private_growth=" << (after > before ? after - before : 0) << '\n';
}
}
int main() {
    try {
        measure("newline", "\n", false);
        measure("nul-newline", std::string("\0\n", 2), true);
        measure("ordinary", "sensor:1\n", true);
        std::cout << "PASS: bounded lens cost observations retain raw bytes and every logical row\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
