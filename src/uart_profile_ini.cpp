#include "uart_profiles.h"
#include "uart_files.h"
#include <windows.h>
#include <filesystem>
#include <stdexcept>
void write_profile_ini(const std::wstring& path,const UartProfile& p,const std::atomic<bool>& cancel) {
    std::wstring error; if (!validate_profile(p,error)) throw std::runtime_error("Invalid staged profile");
    std::string original="\xff\xfe";
    if (std::filesystem::exists(path)) original=uart_read_file(path,1024*1024,cancel);
    static std::atomic<unsigned> sequence{0};
    const auto staging=path+L".profile-stage-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++sequence);
    uart_write_file(staging,original,false,cancel);
    try {
        const auto put=[&](const wchar_t* section,const wchar_t* key,const std::wstring& value) {
            if (cancel || !WritePrivateProfileStringW(section,key,value.c_str(),staging.c_str())) throw std::runtime_error("Profile INI preparation cancelled or failed");
        };
        put(L"Capture",L"Include",p.include); put(L"Capture",L"Exclude",p.exclude); put(L"Capture",L"Regex",p.regex?L"1":L"0"); put(L"Capture",L"MaxChannels",std::to_wstring(p.max_channels));
        for (size_t i=0;i<4;++i) { const auto key=L"Chart"+std::to_wstring(i+1); put(L"FourCharts",key.c_str(),p.charts[i].empty()?L"":L"key:"+uart_wide(p.charts[i])); }
        put(L"UI",L"StartupTab",p.startup_tab==0?L"Messages":p.startup_tab==2?L"4Charts":L"Chart");
        put(L"Serial",L"Port",p.serial.port.device); put(L"Serial",L"DeviceInstanceId",p.serial.port.instance_id);
        put(L"Serial",L"Baud",std::to_wstring(p.serial.baud)); put(L"Serial",L"DataBits",std::to_wstring(p.serial.data_bits));
        put(L"Serial",L"Parity",p.serial.parity==1?L"ODD":p.serial.parity==2?L"EVEN":L"NONE"); put(L"Serial",L"StopBits",p.serial.stop_bits==1?L"1.5":p.serial.stop_bits==2?L"2":L"1");
        put(L"SerialControls",L"Configuration",L"v1:"+std::to_wstring(static_cast<int>(p.serial.controls.flow))+L":"+(p.serial.controls.dtr?L"1":L"0")+L":"+(p.serial.controls.rts?L"1":L"0"));
        put(L"Messages",L"FontSizePt",std::to_wstring(p.font_points));
        put(L"Messages",L"Encoding",std::to_wstring(static_cast<int>(p.display.encoding)));
        put(L"Messages",L"Clock",std::to_wstring(static_cast<int>(p.display.clock)));
        put(L"Messages",L"Newline",std::to_wstring(static_cast<int>(p.display.newline)));
        put(L"Messages",L"CodePage",std::to_wstring(p.display.code_page)); put(L"Messages",L"AnsiSgr",p.display.ansi_sgr?L"1":L"0");
        put(L"Messages",L"Wrap",p.wrap?L"1":L"0");
        put(L"Extraction",L"Configuration",uart_wide(encode_extraction(p.extraction)));
        put(L"ChannelDisplay",L"Configuration",uart_wide(uart_hex(p.channels.encode())));
        if(p.workspace)for(const auto& item:p.workspace_settings){const auto split=item.first.find('/');const auto section=uart_wide(item.first.substr(0,split)),key=uart_wide(item.first.substr(split+1));put(section.c_str(),key.c_str(),uart_wide(item.second));}
        WritePrivateProfileStringW(nullptr,nullptr,nullptr,staging.c_str());
        auto updated=uart_read_file(staging,1024*1024,cancel);
        uart_write_file(path,updated,true,cancel);
    } catch (...) { DeleteFileW(staging.c_str()); throw; }
    DeleteFileW(staging.c_str()); WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
}
