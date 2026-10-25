#pragma once
#include <atomic>
#include <filesystem>
#include <string>
#include <string_view>
std::string uart_read_file(const std::filesystem::path& path, size_t limit, const std::atomic<bool>& cancel);
void uart_write_file(const std::filesystem::path& path, std::string_view bytes, bool replace, const std::atomic<bool>& cancel);
std::string uart_hex(std::string_view value);
bool uart_unhex(std::string_view value, std::string& result);
std::wstring uart_wide(std::string_view utf8);
bool uart_number(const std::wstring& text, double low, double high, double& value);
