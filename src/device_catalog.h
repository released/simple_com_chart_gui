#pragma once
#include <atomic>
#include <string>
#include <vector>
struct DeviceDetails {
    std::wstring port,name,instance,hardware,manufacturer,location,driver,driver_version,serial;
};
std::vector<DeviceDetails> serial_device_catalog(const std::atomic<bool>&);
std::wstring device_catalog_report(const std::vector<DeviceDetails>&,const std::wstring& search);
