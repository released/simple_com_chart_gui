#include "recording_settings.h"
#include "message_format.h"
#include <windows.h>
#include <algorithm>
#include <stdexcept>
RecordingOptions saved_recording_options(const std::wstring& ini,std::filesystem::path base) {
    RecordingOptions o;o.base=std::move(base);
    o.rotate_bytes=static_cast<uint64_t>(std::clamp<unsigned>(GetPrivateProfileIntW(L"Recording",L"RotateMiB",32,ini.c_str()),1,1024))*1024*1024;
    o.rotate_daily=GetPrivateProfileIntW(L"Recording",L"RotateDaily",1,ini.c_str())!=0;
    wchar_t buffer[256]{};
    if(GetPrivateProfileStringW(L"Recording",L"Limits",L"",buffer,256,ini.c_str())>=255)throw std::runtime_error("Recording limits truncated; review Recording options.");
    o.limits=decode_recording_limits(message_utf8(buffer));return o;
}
