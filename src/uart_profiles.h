#pragma once
#include "serial_connection.h"
#include "message_format.h"
#include "field_extractor.h"
#include "display_preferences.h"
#include <array>
#include <atomic>
#include "workspace_settings.h"
struct UartProfile {
    SerialSettings serial;
    std::wstring include, exclude;
    bool regex = false;
    bool wrap = false;
    int max_channels = 256, font_points = 10, startup_tab = 1;
    std::array<std::string,4> charts;
    MessageDisplayOptions display;
    ExtractionConfig extraction;
    DisplayPreferences channels;
    bool workspace=false;
    WorkspaceSettings workspace_settings;
};
bool valid_profile_name(const std::wstring& name);
bool validate_profile(const UartProfile& profile, std::wstring& error);
std::string encode_profile(const UartProfile& profile);
UartProfile decode_profile(std::string_view bytes);
void write_profile_ini(const std::wstring& path, const UartProfile& profile, const std::atomic<bool>& cancel);
