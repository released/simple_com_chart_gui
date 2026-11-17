#pragma once
#include <string>

// The Win32 profile reader removes one outer quote pair. Protect the literal
// value, including its own quotes/whitespace, without changing legacy readers.
inline std::wstring ini_literal(const std::wstring& value) {
    return L"\"" + value + L"\"";
}
