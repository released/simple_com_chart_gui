#pragma once
#include <map>
#include <string>
#include <vector>
using WorkspaceSettings=std::map<std::string,std::string>;
const std::vector<std::string>& workspace_keys();
WorkspaceSettings read_workspace_settings(const std::wstring& ini);
bool validate_workspace_settings(const WorkspaceSettings&,std::wstring& error);
