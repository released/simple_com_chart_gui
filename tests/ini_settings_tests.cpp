#include "ini_settings.h"
#include "uart_files.h"
#include <windows.h>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
bool passed = true; unsigned checks = 0;
void check(bool ok, const char* label) { ++checks; std::cout << (ok ? "PASS: " : "FAIL: ") << label << '\n'; passed = ok && passed; }
}
int main() { try {
    wchar_t temp[32768]{};
    if (!GetEnvironmentVariableW(L"SCC_TEST_WORK", temp, 32768)) throw std::runtime_error("Registered test work is required");
    const auto root = std::filesystem::path(temp) / L"ini-fixtures";
    if (!std::filesystem::create_directory(root)) throw std::runtime_error("Fresh fixture directory required");
    const auto path = (root / L"settings.ini").wstring();
    const std::atomic<bool> cancel{false}; std::wstring error;
    const std::wstring unicode = L"\x6eab\x5ea6 %;#= C:\\logs\\\x6e2c\x8a66";
    CaptureSettings settings{unicode, L"drop", true, 513};
    const bool created=save_ini_patch(path, {{L"Unknown", L"Keep", unicode}}, error);
    if(!created)std::wcerr<<error<<L'\n';
    check(created, "new Unicode INI");
    const bool patched=save_ini_patch(path, settings.patch(), error);if(!patched)std::wcerr<<error<<L'\n';
    check(patched, "typed Capture atomic patch");
    const auto input = load_capture_settings(path); CaptureSettings restored;
    check(validate_capture_settings(input, restored, error) && restored.include == unicode && restored.max_channels == 513 && restored.regex, "Unicode and literal characters round-trip");
    wchar_t value[1024]{}; GetPrivateProfileStringW(L"Unknown", L"Keep", L"", value, 1024, path.c_str());
    check(value == unicode, "unknown section is preserved");
    check(save_ini_patch(path,{{L"FourCharts",L"Chart1",L"key:"+unicode},{L"FourCharts",L"Chart2",L"key:\"quoted\""},{L"Messages",L"FontSizePt",L"11"}},error),"chart/font blocks patch without rewriting Capture");
    GetPrivateProfileStringW(L"FourCharts",L"Chart2",L"",value,1024,path.c_str());
    check(std::wstring(value)==L"key:\"quoted\""&&load_capture_settings(path).include==unicode,"quoted chart prefix and active Capture survive unrelated patch");
    const auto original = uart_read_file(path, 1024 * 1024, cancel);
    for (size_t fail = 0; fail <= settings.patch().size(); ++fail) {
        auto changed = settings; changed.include = L"different";
        const bool ok = save_ini_patch(path, changed.patch(), error, [=](size_t step) { if (step == fail) throw std::runtime_error("Injected failure"); });
        check(!ok && uart_read_file(path, 1024 * 1024, cancel) == original, "Nth setting / pre-replace failure retains entire original");
    }
    const HANDLE locked = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    check(locked != INVALID_HANDLE_VALUE, "fixture denies target replacement");
    check(!save_ini_patch(path, {{L"Messages", L"FontSizePt", L"19"}}, error), "replacement denial is visible");
    if (locked != INVALID_HANDLE_VALUE) CloseHandle(locked);
    check(uart_read_file(path, 1024 * 1024, cancel) == original, "replacement denial retains bytes");
    const auto alternate = (root / L"external.ini").wstring();
    check(save_ini_patch(alternate, {{L"External", L"Keep", L"newer"}}, error), "external edit fixture");
    const bool conflict=save_ini_patch(path, settings.patch(), error, [&](size_t step) {
        if (step == settings.patch().size() && !MoveFileExW(alternate.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) throw std::runtime_error("External replacement fixture failed");
    }); if(!conflict)std::wcerr<<error<<L'\n';
    check(!conflict, "newer externally replaced INI is not overwritten");
    GetPrivateProfileStringW(L"External", L"Keep", L"", value, 1024, path.c_str());
    check(std::wstring(value) == L"newer", "external content survives detected conflict");
    const auto ansi = (root / L"ansi.ini").wstring();
    uart_write_file(ansi, "[Unknown]\r\nKeep=100%;#=C:\\logs\\x\r\n", false, cancel);
    const bool converted=save_ini_patch(ansi, {{L"Capture", L"Include", unicode}}, error);if(!converted)std::wcerr<<error<<L'\n';
    check(converted && load_capture_settings(ansi).include == unicode, "legacy ANSI converts compatibly before Unicode patch");
    GetPrivateProfileStringW(L"Unknown", L"Keep", L"", value, 1024, ansi.c_str());
    check(std::wstring(value) == L"100%;#=C:\\logs\\x", "legacy unknown literal text preserved");
    const auto chinese=root/L"\x6e2c\x8a66";std::filesystem::create_directory(chinese);
    check(save_ini_patch((chinese/L"\x8a2d\x5b9a.ini").wstring(),settings.patch(),error),"non-ASCII INI path");
    const auto broken=(root/L"broken.ini").wstring();const std::string corrupt="\xff\xfeX";
    uart_write_file(broken,corrupt,false,cancel);
    check(!save_ini_patch(broken,settings.patch(),error)&&uart_read_file(broken,1024,cancel)==corrupt,"truncated UTF-16 is preserved");
    const auto oversized=(root/L"oversized.ini").wstring();const std::string huge(1024*1024+1,'x');
    uart_write_file(oversized,huge,false,cancel);
    check(!save_ini_patch(oversized,settings.patch(),error)&&uart_read_file(oversized,2*1024*1024,cancel)==huge,"oversized INI is preserved");
    for (const auto& max : {L"", L"0", L"1025", L"-1", L"12x", L"999999999999999"}) {
        auto bad = input; bad.max_channels = max; check(!validate_capture_settings(bad, restored, error), "invalid channel limit rejected");
    }
    auto bad = input; bad.regex = L"2";
    check(!validate_capture_settings(bad, restored, error), "corrupt mode rejected instead of widened");
    check(validate_capture_settings(load_capture_settings((root/L"missing.ini").wstring()), restored, error) && restored.max_channels == 256 && !restored.regex, "missing keys use safe defaults");
    check(std::none_of(std::filesystem::directory_iterator(root), std::filesystem::directory_iterator{}, [](const auto& item) { return item.path().wstring().find(L".settings-stage-") != std::wstring::npos; }), "all owned staging files retired");
    std::cout << "settings_checks=" << checks << '\n'; return passed ? 0 : 1;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
