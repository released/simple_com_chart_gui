#include "capture_export.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <locale>
#include <stdexcept>

namespace {
using Bytes = std::vector<unsigned char>;
void u16(Bytes& b, unsigned value) { b.push_back(static_cast<unsigned char>(value)); b.push_back(static_cast<unsigned char>(value >> 8)); }
void u32(Bytes& b, uint32_t value) { u16(b, value); u16(b, value >> 16); }
void number(Bytes& b, double value) {
    const auto offset = b.size(); b.resize(offset + sizeof(value));
    std::memcpy(b.data() + offset, &value, sizeof(value));
}
size_t record(Bytes& b, unsigned type, const Bytes& payload) {
    if (payload.size() > 8224) throw std::runtime_error("XLS record is too long. Use CSV or log.");
    const auto offset = b.size(); u16(b, type); u16(b, static_cast<unsigned>(payload.size()));
    b.insert(b.end(), payload.begin(), payload.end()); return offset + 4;
}
void bof(Bytes& b, unsigned type) {
    Bytes p; u16(p, 0x600); u16(p, type); u16(p, 0x0DBB); u16(p, 1996); u32(p, 0x41); u32(p, 6);
    record(b, 0x809, p);
}
std::wstring wide(const std::string& s) {
    const auto n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n); return w;
}
std::string quoted(std::string s) {
    // Prevent spreadsheet formulas in user-controlled column names.
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first != std::string::npos && std::string("=+-@").find(s[first]) != std::string::npos) s.insert(0, "'");
    std::string out = "\"";
    for (char c : s) { out += c; if (c == '"') out += '"'; }
    return out + '"';
}
Bytes workbook(const CaptureSnapshot& data) {
    if (data.keys.size() > 254 || data.rows.size() > 65535)
        throw std::runtime_error("XLS supports 254 telemetry fields and 65535 data rows. Use CSV or log for this capture.");
    Bytes b; bof(b, 5);
    Bytes codepage; u16(codepage, 1200); record(b, 0x42, codepage);
    Bytes font; u16(font, 200); u16(font, 0); u16(font, 0x7fff); u16(font, 400);
    u16(font, 0); font.insert(font.end(), {0, 0, 0, 0, 5, 0, 'A', 'r', 'i', 'a', 'l'});
    record(b, 0x31, font);
    // Built-in Normal style XF plus a cell XF referencing it.
    Bytes style_xf(20, 0); style_xf[4] = 0xf5; style_xf[5] = 0xff;
    record(b, 0xe0, style_xf); record(b, 0xe0, Bytes(20, 0));
    record(b, 0x293, {0, 0x80, 0, 0});
    Bytes sheet; u32(sheet, 0); sheet.insert(sheet.end(), {0, 0, 7, 0, 'C', 'a', 'p', 't', 'u', 'r', 'e'});
    const auto sheet_offset = record(b, 0x85, sheet);
    std::vector<std::string> headers = {"Row", "Time (s)"};
    headers.insert(headers.end(), data.keys.begin(), data.keys.end());
    Bytes sst; u32(sst, static_cast<uint32_t>(headers.size())); u32(sst, static_cast<uint32_t>(headers.size()));
    unsigned sst_type = 0xfc;
    for (const auto& header : headers) {
        const auto w = wide(header);
        if (w.size() > 4000) throw std::runtime_error("XLS field name is too long. Use CSV or log.");
        if (sst.size() + 3 + w.size() * 2 > 8224) { record(b, sst_type, sst); sst.clear(); sst_type = 0x3c; }
        u16(sst, static_cast<unsigned>(w.size())); sst.push_back(1);
        for (wchar_t c : w) u16(sst, c);
    }
    record(b, sst_type, sst); record(b, 0xa, {});
    const auto position = static_cast<uint32_t>(b.size());
    for (int i = 0; i != 4; ++i) b[sheet_offset + i] = static_cast<unsigned char>(position >> (8 * i));
    bof(b, 0x10);
    Bytes dims; u32(dims, 0); u32(dims, static_cast<uint32_t>(data.rows.size() + 1));
    u16(dims, 0); u16(dims, static_cast<unsigned>(headers.size())); u16(dims, 0); record(b, 0x200, dims);
    for (unsigned col = 0; col < headers.size(); ++col) {
        Bytes p; u16(p, 0); u16(p, col); u16(p, 1); u32(p, col); record(b, 0xfd, p);
    }
    const auto cell = [&b](unsigned row, unsigned col, double value) {
        Bytes p; u16(p, row); u16(p, col); u16(p, 1); number(p, value); record(b, 0x203, p);
    };
    unsigned rownum = 1;
    for (const auto& row : data.rows) {
        cell(rownum, 0, static_cast<double>(row->sequence)); cell(rownum, 1, row->seconds);
        for (const auto& entry : row->cells) cell(rownum, static_cast<unsigned>(entry.first + 2), entry.second);
        ++rownum;
    }
    Bytes window(18, 0); window[0] = 0xb6; window[1] = 0x06; record(b, 0x23e, window);
    record(b, 0xa, {}); return b;
}
void write_xls(const CaptureSnapshot& data, const std::filesystem::path& path) {
    const auto bytes = workbook(data);
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IStorage* storage = nullptr; IStream* stream = nullptr;
    HRESULT hr = StgCreateDocfile(path.c_str(), STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &storage);
    if (SUCCEEDED(hr)) hr = storage->CreateStream(L"Workbook", STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
    ULONG written = 0;
    if (SUCCEEDED(hr)) hr = stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written);
    if (SUCCEEDED(hr) && written != bytes.size()) hr = STG_E_WRITEFAULT;
    if (SUCCEEDED(hr)) hr = stream->Commit(STGC_DEFAULT);
    if (stream) stream->Release();
    if (SUCCEEDED(hr)) hr = storage->Commit(STGC_DEFAULT);
    if (storage) storage->Release();
    if (SUCCEEDED(init)) CoUninitialize();
    if (FAILED(hr)) throw std::runtime_error("Cannot write XLS workbook (storage or disk error).");
}
}

CaptureFormat capture_format(const std::filesystem::path& path) {
    auto ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    if (ext == L".xls") return CaptureFormat::Xls;
    if (ext == L".csv" || ext == L".cvs") return CaptureFormat::Csv;
    if (ext == L".txt" || ext == L".log") return CaptureFormat::Text;
    throw std::runtime_error("Supported extensions: .log, .txt, .xls, .csv, .cvs");
}

namespace {
template <typename Writer>
void write_atomically(const std::filesystem::path& path, Writer write) {
    const auto parent = std::filesystem::absolute(path).parent_path();
    WIN32_FILE_ATTRIBUTE_DATA before{};
    const bool existed = GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &before) != FALSE;
    wchar_t temporary[MAX_PATH] = {};
    if (!GetTempFileNameW(parent.c_str(), L"scc", 0, temporary))
        throw std::runtime_error("Cannot create export file. Check folder permissions and free space.");
    try {
        write(temporary);
        WIN32_FILE_ATTRIBUTE_DATA after{};
        const bool exists_now = GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &after) != FALSE;
        if (existed != exists_now || (existed && (before.nFileSizeHigh != after.nFileSizeHigh ||
            before.nFileSizeLow != after.nFileSizeLow || CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime))))
            throw std::runtime_error("Destination changed during export. Save again to confirm replacement.");
        if (!MoveFileExW(temporary, path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace destination. It may be open in another application.");
    } catch (...) { DeleteFileW(temporary); throw; }
}
}

void export_capture(const CaptureSnapshot& data, const std::filesystem::path& path, CaptureFormat format) {
    write_atomically(path, [&](const std::filesystem::path& temporary) {
        if (format == CaptureFormat::Xls) { write_xls(data, temporary); return; }
        std::ofstream f(temporary, std::ios::binary | std::ios::trunc);
        f.exceptions(std::ios::failbit | std::ios::badbit); f.imbue(std::locale::classic()); f << std::setprecision(17);
        if (format == CaptureFormat::Text) {
            for (const auto& row : data.rows) f << row->raw << "\r\n";
        } else {
            f << "\xef\xbb\xbf\"Row\",\"Time (s)\"";
            for (const auto& key : data.keys) f << ',' << quoted(key);
            f << "\r\n";
            for (const auto& row : data.rows) {
                f << row->sequence << ',' << row->seconds;
                size_t index = 0;
                for (size_t col = 0; col < data.keys.size(); ++col) {
                    f << ',';
                    if (index < row->cells.size() && row->cells[index].first == col) f << row->cells[index++].second;
                }
                f << "\r\n";
            }
        }
        f.close();
    });
}

void export_message_text(const std::string& text, const std::filesystem::path& path) {
    if (capture_format(path) != CaptureFormat::Text)
        throw std::runtime_error("Messages supports .log and .txt only.");
    write_atomically(path, [&](const std::filesystem::path& temporary) {
        std::ofstream f(temporary, std::ios::binary | std::ios::trunc);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        f.write(text.data(), static_cast<std::streamsize>(text.size())); f.close();
    });
}
