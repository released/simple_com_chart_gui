#include "message_buffer.h"
#include "uart_recorder.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char* name) { if (!ok) throw std::runtime_error(name); std::cout << "PASS: " << name << '\n'; }
std::string read(const std::filesystem::path& path) { std::ifstream file(path, std::ios::binary); return {std::istreambuf_iterator<char>(file), {}}; }
}
int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected fixture directory");
        auto fixture = std::filesystem::path(argv[1]) / std::to_wstring(GetCurrentProcessId());
        std::filesystem::create_directories(fixture);
        MessageBuffer history;
        history.append("A\r", 0, 1700000000000); history.append("\nB\n", 0, 1700000001250);
        MessageDisplayOptions options;
        check(format_message(history.bytes(), history.offset(), history.receipts(), options).text == history.ascii(), "legacy ASCII including split CRLF");
        options.clock = MessageClock::Elapsed;
        auto formatted = format_message(history.bytes(), history.offset(), history.receipts(), options, history.origin_ms());
        check(formatted.text == L"[0.000s] A\r\n[1.250s] B\r\n", "elapsed timestamps use first-byte receipt");
        options.clock = MessageClock::Delta;
        check(format_message(history.bytes(), 0, history.receipts(), options).text.find(L"[dt +1.250s] B") != std::wstring::npos, "per-line receipt delta");
        MessageBuffer transferred; transferred.append_history(history);
        check(transferred.bytes() == history.bytes() && transferred.receipts().size() == 2 && transferred.receipts()[1].unix_ms == 1700000001250, "pending delivery preserves receipt boundaries");
        options = {}; options.encoding = MessageEncoding::Utf8;
        MessageBuffer unicode; unicode.append("\xE4\xB8", 0, 1700000000000);
        check(format_message(unicode.bytes(), 0, unicode.receipts(), options).text.empty(), "partial UTF-8 sequence waits for next chunk");
        unicode.append("\xAD\xE6\x96\x87");
        check(format_message(unicode.bytes(), 0, unicode.receipts(), options).text == L"中文", "UTF-8 split chunks decode exact Unicode");
        check(format_message("\xFFx", 0, {}, options).text == L"\\xFFx", "invalid UTF-8 is visible, not silently discarded");
        options.encoding = MessageEncoding::Ansi; options.code_page = 950;
        check(format_message("\xA4\xA4", 0, {}, options).text == L"中", "explicit ANSI Big5 decoding");
        options = {}; options.ansi_sgr = true;
        const auto color = format_message("\x1b[31mERROR\x1b[0m ok", 0, {}, options);
        check(color.text == L"ERROR ok" && color.colors.size() >= 2 && color.colors.front().color == RGB(220,70,70), "optional ANSI SGR spans");
        options.ansi_sgr = false;
        check(format_message("\x1b[31mX", 0, {}, options).text == L"\\x1B[31mX", "ANSI off preserves visible escape bytes");
        options.newline = MessageNewline::CRLF;
        check(format_message("a\rb\nc\r\n", 0, {}, options).text == L"a\\x0Db\\x0Ac\r\n", "explicit CRLF delimiter");
        options = {}; options.encoding = MessageEncoding::Hex;
        check(format_message(history.bytes(), history.offset(), history.receipts(), options).text == history.hex(), "legacy HEX offsets and bytes");
        std::wstring error;
        check(find_message_matches(L"ERR err warn", L"err", false, false, error).size() == 2 && error.empty(), "literal case-insensitive search");
        check(find_message_matches(L"ERR err warn", L"err|warn", true, true, error).size() == 2, "regex alternatives and case");
        check(find_message_matches(L"x", L"[", true, true, error).empty() && !error.empty(), "invalid regex reports error");
        check(find_message_matches(std::wstring(10000, L'a'), L"(a+)+$", true, true, error).empty() && !error.empty(), "unsafe nested regex rejected before execution");
        check(find_message_matches(std::wstring(10000, L'a'), L"(a+|b)(a+|b)X", true, true, error).empty() && !error.empty(), "ambiguous grouped quantifiers rejected before execution");
        UartRecorder recorder;
        RecordingOptions recording; recording.base = fixture / L"session"; recording.rotate_bytes = 1024;
        check(recorder.start(recording), "recording starts asynchronously");
        check(!recorder.start(recording), "duplicate start rejected");
        std::string raw("A\0\xFF\r\n", 5); recorder.append(raw, 1700000000000); recorder.append("tail", 1700000001000);
        recorder.event("disconnect", 1700000001500); recorder.stop();
        check(recorder.wait_idle(5000), "stop drains queue and flushes files");
        check(read(fixture / L"session_1.bin") == raw + "tail", "binary recording preserves exact bytes, excludes synthetic markers");
        check(read(fixture / L"session_1.idx").find("0,5,1700000000000,RX") != std::string::npos && read(fixture / L"session_1.idx").find("disconnect") != std::string::npos, "index contains receipt times and separate events");
        check(read(fixture / L"session_1.log").find("A\\x00\\xFF\r\n") != std::string::npos, "readable recording escapes binary and normalizes CRLF");
        check(recorder.start(recording), "restart request accepted"); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().detail.find(L"ERROR") != std::wstring::npos && read(fixture / L"session_1.bin") == raw + "tail", "existing recording never overwritten");
        recording.base = fixture / L"rotation"; check(recorder.start(recording), "rotation fixture starts");
        recorder.append(std::string(800, 'a'), 1700000000000); recorder.append(std::string(800, 'b'), 1700000000001); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().segments == 2 && read(fixture / L"rotation_1.bin") == std::string(800, 'a') && read(fixture / L"rotation_2.bin") == std::string(800, 'b'), "size rotation and raw concatenation");
        recording.base = fixture / L"daily"; check(recorder.start(recording), "daily fixture starts");
        recorder.append("a", 1700000000000); recorder.append("b", 1700086400000); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().segments == 2, "PC local date rotation");
        recording.base = fixture / L"missing" / L"中文"; check(recorder.start(recording), "bad path request is asynchronous");
        recorder.stop(); check(recorder.wait_idle(5000) && recorder.status().detail.find(L"ERROR") != std::wstring::npos && recorder.status().detail.find(L"中文") != std::wstring::npos, "filesystem failure includes Unicode path");
        recording.base = fixture / L"overflow"; check(recorder.start(recording), "overflow fixture starts");
        recorder.append(std::string(8 * 1024 * 1024 + 1, 'x'), 1700000000000);
        check(recorder.wait_idle(5000) && recorder.status().lost == 8 * 1024 * 1024 + 1 && read(fixture / L"overflow_1.idx").find("INPUT GAP") != std::string::npos, "bounded recorder stops visibly on overflow");
        recording.base = fixture / L"duplex"; check(recorder.start(recording), "RX TX recording fixture starts");
        recorder.append("before",1700000000000); recorder.append(std::string("A\0B",3),1700000000001,true); recorder.append("after\n",1700000000002); recorder.stop();
        check(recorder.wait_idle(5000) && read(fixture / L"duplex_1.bin") == "beforeafter\n" && recorder.status().bytes == 12 && recorder.status().tx_bytes == 3, "raw recording excludes TX and keeps independent byte totals");
        const auto duplex_text = read(fixture / L"duplex_1.log");
        check(duplex_text.find("[TX]") != std::string::npos && duplex_text.find("A\\x00B") != std::string::npos && duplex_text.find("[RX] after") != std::string::npos, "readable log separates interleaved TX RX directions");
        std::cout << "PASS: UART receive core\n"; return 0;
    } catch (const std::exception& ex) { std::cerr << "FAIL: " << ex.what() << '\n'; return 1; }
}
