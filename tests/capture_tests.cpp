#include "capture_store.h"
#include "capture_export.h"
#include <windows.h>
#include <objbase.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string read(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}};
}
}

void test_capture(const std::filesystem::path& project) {
    CaptureStore bounded(3, 4096);
    bounded.reset(2);
    bounded.append("a:1", {{"a", 1}}, 100);
    bounded.append("b:-2.5", {{"b", -2.5}}, 101);
    auto immutable = bounded.snapshot();
    bounded.append("ignored:3", {{"ignored", 3}}, 102);
    bounded.append("plain prose", {}, 103);
    require(bounded.rows().size() == 3 && bounded.dropped() == 1, "Bounded row history");
    require(immutable.rows.size() == 2 && immutable.rows[0]->raw == "a:1", "Immutable export snapshot");
    require(bounded.keys().size() == 2 && bounded.rows().back()->sequence == 4, "Channel limit / sequence");
    double value = 0;
    require(CaptureStore::value(*bounded.rows().front(), 1, value) && value == -2.5, "Sparse signed values");
    require(!CaptureStore::value(*bounded.rows().back(), 0, value), "Missing values remain blank");
    CaptureStore bytes(10, 128); bytes.append(std::string(1024, 'x'), {}, 1);
    require(bytes.rows().empty() && bytes.dropped() == 1, "Byte cap / oversized line");
    bounded.reset(); require(bounded.rows().empty() && bounded.keys().empty() && bounded.dropped() == 0, "Reset capture");

    CaptureStore capture;
    capture.append("a:1,small:-0.0025,large:2074", {{"a", 1}, {"small", -.0025}, {"large", 2074}}, 10);
    capture.append("a:2", {{"a", 2}}, 10.25);
    capture.append("raw text without numeric data", {}, 11);
    capture.append("unicode", {{u8"溫度", 26.5}, {"=FORMULA", 4}, {"quote\"field", 5}}, 12);
    const auto folder = project / L"build/verify/export-tests" /
        (std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    require(std::filesystem::create_directories(folder), "Create unique export evidence folder");
    const auto data = capture.snapshot();
    for (const auto* suffix : {L".log", L".txt", L".csv", L".cvs", L".xls"}) {
        const auto file = folder / (std::wstring(L"capture") + suffix);
        export_capture(data, file, capture_format(file));
        require(std::filesystem::file_size(file) > 0, "Export created nonempty file");
    }
    const auto text = read(folder / L"capture.log");
    require(text == read(folder / L"capture.txt") && text.find("raw text without numeric data\r\n") != std::string::npos, "Plain raw log content");
    const auto csv = read(folder / L"capture.csv");
    require(csv == read(folder / L"capture.cvs") && csv.find("\"'=FORMULA\"") != std::string::npos &&
        csv.find("\"quote\"\"field\"") != std::string::npos && csv.find("-0.002500") != std::string::npos, "CSV precision, quoting, formula protection, alias");
    IStorage* storage = nullptr;
    HRESULT hr = StgOpenStorage((folder / L"capture.xls").c_str(), nullptr, STGM_READ | STGM_SHARE_DENY_WRITE, nullptr, 0, &storage);
    require(SUCCEEDED(hr), "XLS is a compound binary file, not renamed text");
    IStream* stream = nullptr;
    hr = storage->OpenStream(L"Workbook", nullptr, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream);
    unsigned char signature[8] = {}; ULONG read_count = 0;
    if (SUCCEEDED(hr)) hr = stream->Read(signature, sizeof(signature), &read_count);
    if (stream) stream->Release(); storage->Release();
    require(SUCCEEDED(hr) && read_count == 8 && signature[0] == 9 && signature[1] == 8 && signature[4] == 0 && signature[5] == 6, "BIFF8 workbook BOF");
    CaptureStore too_wide; too_wide.reset(1024);
    std::unordered_map<std::string, double> fields;
    for (int i = 0; i < 255; ++i) fields["field" + std::to_string(i)] = i;
    too_wide.append("wide", fields, 1);
    const auto target = folder / L"capture.xls";
    const auto original = read(target);
    bool rejected = false;
    try { export_capture(too_wide.snapshot(), target, CaptureFormat::Xls); } catch (...) { rejected = true; }
    require(rejected && read(target) == original, "XLS column overflow preserves existing destination");
    CaptureStore wide; wide.reset(254);
    fields.clear();
    for (int i = 0; i < 254; ++i) fields["K" + std::to_string(i) + std::string(120, 'x')] = i + .125;
    wide.append("wide", fields, 1);
    export_capture(wide.snapshot(), folder / L"wide.xls", CaptureFormat::Xls);
    const auto locked = folder / L"capture.log";
    HANDLE handle = CreateFileW(locked.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    require(handle != INVALID_HANDLE_VALUE, "Lock test-owned export file");
    bool lock_rejected = false;
    try { export_capture(data, locked, CaptureFormat::Text); } catch (...) { lock_rejected = true; }
    CloseHandle(handle);
    require(lock_rejected && read(locked) == text, "Locked destination is preserved");
    export_capture(data, folder / L"capture.log", CaptureFormat::Text);
    require(read(folder / L"capture.log") == text, "Confirmed replacement succeeds");
    bool extension_rejected = false;
    try { capture_format(L"bad.xlsx"); } catch (...) { extension_rejected = true; }
    require(extension_rejected, "Reject unsupported extension");
    std::cout << "PASS: capture bounds, sparse rows, snapshot, five export extensions, binary XLS, safe failure\nEvidence: " << folder.string() << '\n';
}
