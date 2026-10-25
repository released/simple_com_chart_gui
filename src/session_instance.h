#pragma once
#include <windows.h>
#include <string>

// Cooperative per-session exclusion. The identity never contains a path/version.
class SessionInstance {
public:
    enum class Result { Owner, Activated, Failed };
    ~SessionInstance();
    Result acquire(const std::wstring& identity, std::wstring& error);
    void ready(HWND main);
private:
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    HANDLE mutex_ = nullptr, event_ = nullptr;
    HWND window_ = nullptr, main_ = nullptr;
    UINT activate_ = 0;
    bool owns_ = false;
};
void publish_main_instance(HWND main);
