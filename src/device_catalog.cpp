#include "device_catalog.h"
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <algorithm>
#include <cwctype>
#include <stdexcept>
namespace {
std::wstring property(HDEVINFO devices,SP_DEVINFO_DATA& item,DWORD id) {
    wchar_t b[2048]{};DWORD kind=0;
    if(!SetupDiGetDeviceRegistryPropertyW(devices,&item,id,&kind,reinterpret_cast<PBYTE>(b),sizeof(b),nullptr))return {};
    b[2047]=0;return std::wstring(b).substr(0,1024);
}
std::wstring registry(HKEY key,const wchar_t* name) {
    if(key==INVALID_HANDLE_VALUE||!key)return {};
    wchar_t b[1024]{};DWORD bytes=sizeof(b),kind=0;
    if(RegQueryValueExW(key,name,nullptr,&kind,reinterpret_cast<BYTE*>(b),&bytes)!=ERROR_SUCCESS||kind!=REG_SZ)return {};
    b[1023]=0;return b;
}
std::wstring folded(std::wstring s){for(auto& c:s)c=static_cast<wchar_t>(std::towlower(c));return s;}
}
std::vector<DeviceDetails> serial_device_catalog(const std::atomic<bool>& cancel) {
    const auto devices=SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS,nullptr,nullptr,DIGCF_PRESENT);
    if(devices==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot enumerate present serial devices.");
    struct Owner{HDEVINFO d;~Owner(){SetupDiDestroyDeviceInfoList(d);}} owner{devices};
    std::vector<DeviceDetails> out;SP_DEVINFO_DATA item{};item.cbSize=sizeof(item);
    for(DWORD n=0;;++n) {
        if(cancel)throw std::runtime_error("Device scan cancelled.");
        if(!SetupDiEnumDeviceInfo(devices,n,&item)){if(GetLastError()!=ERROR_NO_MORE_ITEMS)throw std::runtime_error("Incomplete serial-device enumeration.");break;}
        if(n>=512)throw std::runtime_error("Device scan exceeds 512 entries.");
        DeviceDetails d;const auto key=SetupDiOpenDevRegKey(devices,&item,DICS_FLAG_GLOBAL,0,DIREG_DEV,KEY_READ);
        d.port=registry(key,L"PortName");d.serial=registry(key,L"SerialNumber");if(key!=INVALID_HANDLE_VALUE)RegCloseKey(key);
        if(d.port.size()<4||d.port.substr(0,3)!=L"COM"||!std::all_of(d.port.begin()+3,d.port.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';}))continue;
        wchar_t id[2048]{};if(SetupDiGetDeviceInstanceIdW(devices,&item,id,2048,nullptr))d.instance=id;
        d.name=property(devices,item,SPDRP_FRIENDLYNAME);d.hardware=property(devices,item,SPDRP_HARDWAREID);
        d.manufacturer=property(devices,item,SPDRP_MFG);d.location=property(devices,item,SPDRP_LOCATION_INFORMATION);
        const auto driver=SetupDiOpenDevRegKey(devices,&item,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_READ);
        d.driver=registry(driver,L"ProviderName");d.driver_version=registry(driver,L"DriverVersion");if(driver!=INVALID_HANDLE_VALUE)RegCloseKey(driver);
        out.push_back(std::move(d));
    }
    std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.port<b.port;});return out;
}
std::wstring device_catalog_report(const std::vector<DeviceDetails>& devices,const std::wstring& search) {
    std::wstring out=L"Present COM devices (read-only; no port was opened).\r\nIdentifiers may be private. Copy only when intended.\r\n\r\n";size_t shown=0;const auto wanted=folded(search);
    for(const auto& d:devices){if(folded(d.port+L" "+d.name+L" "+d.instance+L" "+d.hardware+L" "+d.manufacturer).find(wanted)==std::wstring::npos)continue;
        const std::pair<const wchar_t*,std::wstring> fields[]={{L"COM",d.port},{L"Name",d.name},{L"Hardware IDs / VID/PID",d.hardware},{L"Instance ID",d.instance},{L"Driver-published serial",d.serial},{L"Manufacturer",d.manufacturer},{L"Location",d.location},{L"Driver provider",d.driver},{L"Driver version",d.driver_version}};
        std::wstring entry;for(const auto& f:fields)entry+=std::wstring(f.first)+L": "+(f.second.empty()?L"n/a":f.second)+L"\r\n";
        if(out.size()+entry.size()>30000){out+=L"Output bounded; narrow the search.\r\n";break;}out+=entry+L"\r\n";++shown;
    }
    out+=L"Shown: "+std::to_wstring(shown)+L". Serial is n/a unless explicitly published; an instance suffix is not guessed to be a USB serial number.";return out;
}
