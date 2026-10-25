#pragma once
#include "uart_files.h"
#include "message_format.h"
#include <stdexcept>
inline std::wstring pack_settings(const std::vector<std::wstring>& fields){
    std::string bytes;for(const auto& field:fields){if(field.find(L'\0')!=field.npos)throw std::runtime_error("NUL in settings.");bytes+=message_utf8(field);bytes+='\0';}
    if(bytes.size()>14000)throw std::runtime_error("Settings exceed 14 KiB.");return L"v1:"+uart_wide(uart_hex(bytes));
}
inline std::vector<std::wstring> unpack_settings(const std::wstring& value,size_t count){
    std::string bytes;if(value.rfind(L"v1:",0)!=0||!uart_unhex(message_utf8(value.substr(3)),bytes)||bytes.size()>14000)throw std::runtime_error("Invalid settings encoding.");
    std::vector<std::wstring> fields;size_t at=0;while(at<bytes.size()){const auto end=bytes.find('\0',at);if(end==bytes.npos)throw std::runtime_error("Incomplete settings.");fields.push_back(uart_wide(std::string_view(bytes).substr(at,end-at)));at=end+1;}
    if(fields.size()!=count)throw std::runtime_error("Invalid settings field count.");return fields;
}
