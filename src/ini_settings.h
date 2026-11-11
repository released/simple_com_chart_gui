#pragma once
#include <functional>
#include <string>
#include <vector>

struct IniSetting {
    std::wstring section, key, value;
};
// Patches only named keys. The bounded original, including unknown sections, is staged intact.
bool save_ini_patch(const std::wstring& path, const std::vector<IniSetting>& values,
                    std::wstring& error,
                    const std::function<void(size_t)>& before_step = {});

struct CaptureSettingsInput {
    std::wstring include, exclude, regex = L"0", max_channels = L"256";
};
struct CaptureSettings {
    std::wstring include, exclude;
    bool regex = false;
    int max_channels = 256;
    std::vector<IniSetting> patch() const;
};
CaptureSettingsInput load_capture_settings(const std::wstring& path);
bool validate_capture_settings(const CaptureSettingsInput&, CaptureSettings&, std::wstring& error);
