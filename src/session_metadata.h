#pragma once
#include "uart_profiles.h"
struct CaptureMetadata {UartProfile profile;std::string version,source;bool changed=false;};
UartProfile session_safe_profile(UartProfile);
std::string encode_capture_metadata(UartProfile,const std::string& version,const std::string& source);
CaptureMetadata decode_capture_metadata(std::string_view);
