#pragma once
#include <windows.h>
#include <filesystem>

bool save_runtime_frame(HWND hwnd, const std::filesystem::path& path);
