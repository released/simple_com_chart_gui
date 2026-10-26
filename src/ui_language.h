#pragma once
#include <windows.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
namespace ui {
using Texts=std::unordered_map<std::string,std::wstring>;
struct Catalog {std::string code;Texts texts;};
std::string text_id(std::wstring_view);
Catalog parse_catalog(std::string_view,const std::string& expected_code);
std::wstring translate(const Catalog&,std::wstring_view);
void initialize_language(const std::filesystem::path& executable);
const std::string& active_language();
const std::wstring& language_notice();
std::string saved_language(const std::filesystem::path& ini);
bool save_language(const std::filesystem::path& ini,const std::string& code);
std::wstring tr(std::wstring_view);
std::wstring display_caption(std::wstring_view);
const wchar_t* caption(const wchar_t*);
}
