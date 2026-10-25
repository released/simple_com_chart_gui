#include "uart_profiles.h"
#include "uart_files.h"
#include "line_filter.h"
#include "settings_fields.h"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <map>
#include <sstream>
#include <stdexcept>

bool valid_profile_name(const std::wstring& name) {
    if (name.empty() || name.size() > 48) return false;
    for (const auto c : name) if (!((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'-')) return false;
    auto upper = name; std::transform(upper.begin(), upper.end(), upper.begin(), towupper);
    if (upper == L"CON" || upper == L"PRN" || upper == L"AUX" || upper == L"NUL") return false;
    if (upper.size() == 4 && (upper.substr(0,3) == L"COM" || upper.substr(0,3) == L"LPT") && upper[3] >= L'0' && upper[3] <= L'9') return false;
    return true;
}
bool validate_profile(const UartProfile& p, std::wstring& error) {
    const auto safe_ini_text=[](const std::wstring& value) {
        return std::none_of(value.begin(),value.end(),[](wchar_t c){return c==0 || c==L'\r' || c==L'\n';}) &&
            (value.empty() || WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr)>0);
    };
    auto serial = p.serial; if (serial.port.device.empty()) serial.port.device = L"COM1";
    if (!valid_serial_settings(serial) || !p.serial.port.open_path.empty() || p.serial.port.instance_id.size() > 512 ||
        !safe_ini_text(p.include) || !safe_ini_text(p.exclude) || !safe_ini_text(p.serial.port.instance_id) ||
        p.max_channels < 1 || p.max_channels > 1024 || p.font_points < 8 || p.font_points > 32 || p.startup_tab < 0 || p.startup_tab > 2 ||
        p.include.size() > 256 || p.exclude.size() > 256 || p.include.find(L'\0') != std::wstring::npos || p.exclude.find(L'\0') != std::wstring::npos ||
        static_cast<int>(p.display.encoding) < 0 || static_cast<int>(p.display.encoding) > 3 || static_cast<int>(p.display.clock) < 0 || static_cast<int>(p.display.clock) > 3 ||
        static_cast<int>(p.display.newline) < 0 || static_cast<int>(p.display.newline) > 3 ||
        (p.display.code_page != 0 && p.display.code_page != 950 && p.display.code_page != 936 && p.display.code_page != 1252)) {
        error = L"Invalid profile settings/range; no settings applied"; return false;
    }
    for (const auto& key : p.charts) if (key.size() > 160 || std::any_of(key.begin(), key.end(), [](unsigned char c) { return c < 32; })) { error = L"Invalid profile chart key"; return false; }
    try { for (const auto& key : p.charts) uart_wide(key); }
    catch (...) { error = L"Invalid profile UTF-8"; return false; }
    LineFilter filter; std::string problem;
    FieldExtractor extractor;if(!extractor.configure(p.extraction,problem)) {error=uart_wide(problem);return false;}
    try {p.channels.encode();}catch(...) {error=L"Invalid/oversized channel display settings.";return false;}
    if(p.workspace){if(!validate_workspace_settings(p.workspace_settings,error))return false;
        bool receipt=false;for(const auto& item:p.workspace_settings)if(item.first.find("Workbench.Binary.")==0&&item.second.rfind("v1:1:",0)==0)receipt=true;
        const auto text=p.workspace_settings.find("Workbench.Text.B/Configuration");if(text!=p.workspace_settings.end()&&!text->second.empty()&&unpack_settings(uart_wide(text->second),9)[0]==L"Enabled")receipt=true;
        if(receipt&&(!p.extraction.time_field.empty()||(p.extraction.mode==ExtractionConfig::Regex&&p.extraction.time_group))){error=L"Workspace A/B numeric sources require PC receipt time; clear device time fields/groups.";return false;}
    }else if(!p.workspace_settings.empty()){error=L"Unexpected workspace settings in legacy profile.";return false;}
    if (!filter.configure(message_utf8(p.include), message_utf8(p.exclude), p.regex, problem)) { error.assign(problem.begin(),problem.end()); return false; }
    error.clear(); return true;
}
std::string encode_profile(const UartProfile& p) {
    std::wstring error; if (!validate_profile(p,error)) throw std::runtime_error("Invalid profile");
    std::map<std::string,std::string> values;
    const auto text = [&](const char* key, const std::wstring& value) { values[key] = uart_hex(message_utf8(value)); };
    const auto number = [&](const char* key, int value) { values[key] = std::to_string(value); };
    text("Port",p.serial.port.device); text("Instance",p.serial.port.instance_id); text("Include",p.include); text("Exclude",p.exclude);
    number("Baud",p.serial.baud); number("Bits",p.serial.data_bits); number("Parity",p.serial.parity); number("Stop",p.serial.stop_bits);
    number("Flow",static_cast<int>(p.serial.controls.flow)); number("DTR",p.serial.controls.dtr); number("RTS",p.serial.controls.rts);
    number("Regex",p.regex); number("Channels",p.max_channels); number("Font",p.font_points); number("Startup",p.startup_tab);
    number("Encoding",static_cast<int>(p.display.encoding)); number("Clock",static_cast<int>(p.display.clock)); number("Newline",static_cast<int>(p.display.newline));
    number("CodePage",static_cast<int>(p.display.code_page)); number("SGR",p.display.ansi_sgr);
    number("Wrap",p.wrap);
    values["Extraction"]=uart_hex(encode_extraction(p.extraction));
    values["ChannelDisplay"]=uart_hex(p.channels.encode());
    for (size_t i = 0; i < 4; ++i) values["Chart" + std::to_string(i)] = uart_hex(p.charts[i]);
    if(p.workspace)for(const auto& item:p.workspace_settings)values["Workspace."+item.first]=uart_hex(item.second);
    std::string result = p.workspace?"SimpleComChartProfile=2\n":"SimpleComChartProfile=1\n"; for (const auto& value : values) result += value.first + "=" + value.second + "\n";if(result.size()>(p.workspace?512*1024:65536)) throw std::runtime_error("Profile exceeds its schema size limit.");return result;
}
UartProfile decode_profile(std::string_view bytes) {
    if (bytes.size() > 512*1024 || bytes.find('\0') != std::string::npos) throw std::runtime_error("Invalid/oversized profile");
    std::istringstream stream{std::string(bytes)}; std::string line;
    if (!std::getline(stream,line) || (line != "SimpleComChartProfile=1"&&line!="SimpleComChartProfile=2")) throw std::runtime_error("Unsupported profile schema");
    const bool workspace=line=="SimpleComChartProfile=2";if(!workspace&&bytes.size()>65536)throw std::runtime_error("Legacy profile exceeds 64 KiB.");
    std::map<std::string,std::string> values;
    while (std::getline(stream,line)) {
        const auto equals = line.find('=');
        if (equals == std::string::npos || !values.emplace(line.substr(0,equals),line.substr(equals+1)).second) throw std::runtime_error("Malformed/duplicate profile key");
    }
    UartProfile p;
    p.workspace=workspace;
    const auto get = [&](const std::string& key) { const auto it = values.find(key); if (it == values.end()) throw std::runtime_error("Missing profile key"); const auto value = it->second; values.erase(it); return value; };
    const auto number = [&](const char* key) { const auto value = get(key); int result = -1; const auto parsed = std::from_chars(value.data(),value.data()+value.size(),result);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size()) throw std::runtime_error("Invalid profile number"); return result; };
    const auto boolean = [&](const char* key) { const auto value = number(key); if (value != 0 && value != 1) throw std::runtime_error("Invalid profile boolean"); return value != 0; };
    const auto text = [&](const char* key) { std::string result; if (!uart_unhex(get(key),result) || result.find('\0') != std::string::npos) throw std::runtime_error("Invalid profile text"); return uart_wide(result); };
    p.serial.port.device = text("Port"); p.serial.port.instance_id = text("Instance"); p.include = text("Include"); p.exclude = text("Exclude");
    p.serial.baud = number("Baud"); p.serial.data_bits = number("Bits"); p.serial.parity = number("Parity"); p.serial.stop_bits = number("Stop");
    p.serial.controls.flow = static_cast<SerialFlow>(number("Flow")); p.serial.controls.dtr = boolean("DTR"); p.serial.controls.rts = boolean("RTS");
    p.regex = boolean("Regex"); p.max_channels = number("Channels"); p.font_points = number("Font"); p.startup_tab = number("Startup");
    p.display.encoding = static_cast<MessageEncoding>(number("Encoding")); p.display.clock = static_cast<MessageClock>(number("Clock")); p.display.newline = static_cast<MessageNewline>(number("Newline"));
    const auto code_page = number("CodePage"); if (code_page < 0) throw std::runtime_error("Invalid code page"); p.display.code_page = static_cast<unsigned>(code_page); p.display.ansi_sgr = boolean("SGR");
    p.wrap=boolean("Wrap");
    if(values.count("Extraction")) {std::string encoded;if(!uart_unhex(get("Extraction"),encoded)) throw std::runtime_error("Invalid parser profile");p.extraction=decode_extraction(encoded);}
    if(values.count("ChannelDisplay")) {std::string encoded;if(!uart_unhex(get("ChannelDisplay"),encoded)) throw std::runtime_error("Invalid channel display profile");p.channels=DisplayPreferences::decode(encoded);}
    for (size_t i=0; i<4; ++i) if (!uart_unhex(get("Chart"+std::to_string(i)),p.charts[i])) throw std::runtime_error("Invalid chart name encoding");
    if(workspace)for(const auto& key:workspace_keys()){std::string value;if(!uart_unhex(get("Workspace."+key),value))throw std::runtime_error("Invalid workspace encoding.");p.workspace_settings[key]=std::move(value);}
    std::wstring error; if (!values.empty() || !validate_profile(p,error)) throw std::runtime_error("Unknown key or invalid profile settings"); return p;
}
