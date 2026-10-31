#pragma once
#include <windows.h>
#include <string>
#include <string_view>

inline std::wstring error_message(std::string_view bytes) {
    if(bytes.empty()) return {};
    bytes=bytes.substr(0,32768);
    const auto count=static_cast<int>(bytes.size());
    const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),count,nullptr,0);
    if(size) {
        std::wstring result(size,L'\0');
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),count,result.data(),size);
        return result;
    }
    constexpr wchar_t hex[]=L"0123456789ABCDEF";
    std::wstring result=L"[invalid UTF-8] ";
    for(unsigned char ch:bytes.substr(0,512)) {
        if(ch>=32&&ch<127) result+=static_cast<wchar_t>(ch);
        else {result+=L"\\x";result+=hex[ch>>4];result+=hex[ch&15];}
    }
    if(bytes.size()>512)result+=L" ...";
    return result;
}
