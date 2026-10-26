#pragma once
#include <afxdlgs.h>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include "capture_filename.h"
#include "ui_language.h"
inline std::optional<std::filesystem::path> workflow_pick(CWnd* owner,const wchar_t* extension,const wchar_t* filter,bool open=false){
    SYSTEMTIME now{};GetLocalTime(&now);auto name=std::filesystem::path(capture_filename(now));name.replace_extension(std::wstring(L".")+extension);
    CFileDialog pick(open,extension,open?nullptr:name.c_str(),OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(open?OFN_FILEMUSTEXIST:OFN_OVERWRITEPROMPT),ui::caption(filter),owner);
    if(pick.DoModal()!=IDOK)return {};const std::filesystem::path path(pick.GetPathName().GetString());
    if(!open&&_wcsicmp(path.extension().c_str(),(std::wstring(L".")+extension).c_str())!=0)throw std::runtime_error("Use the selected workflow file extension.");return path;
}
