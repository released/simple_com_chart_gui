#pragma once
#include <windows.h>
#include <atomic>

// Worker-only overlapped operation. The owner keeps the HANDLE alive until completion.
bool serial_transfer(HANDLE handle, bool write, void* data, DWORD size, DWORD& transferred,
    const std::atomic<bool>& cancelled, DWORD& error);
