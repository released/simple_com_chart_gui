#pragma once
#include <windows.h>
#include <string>
#include <cwchar>

inline std::wstring capture_filename(const SYSTEMTIME& time) {
    wchar_t name[40] = {};
    swprintf_s(name, L"%04u%02u%02u_%02u%02u%02u.log", time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond);
    return name;
}
