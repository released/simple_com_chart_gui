#include "workspace_settings.h"
#include <windows.h>
#include "settings_fields.h"
#include "binary_options.h"
#include "derived_channels.h"
#include "text_telemetry.h"
#include "traffic_framer.h"
#include "serial_connection.h"
#include "lab_metrics.h"
#include "recording_limits.h"
#include <algorithm>
#include <cmath>
#include <sstream>
const std::vector<std::string>& workspace_keys(){static const std::vector<std::string> keys={
    "DualCOM/SplitPercent","DualCOM.B/Port","DualCOM.B/InstanceId","DualCOM.B/Baud","DualCOM.B/Data","DualCOM.B/Parity","DualCOM.B/Stop",
    "Workbench.Binary.A/Configuration","Workbench.Binary.B/Configuration","Workbench.Text.B/Configuration","Workbench.TX.A/Configuration","Workbench.TX.B/Configuration",
    "Workbench.Framing.A/Configuration","Workbench.Framing.B/Configuration","Workbench.View.A/Configuration","Workbench.View.B/Configuration","Replay.Binary/Configuration","Derived/Configuration",
    "Instruments/Key0","Instruments/Key1","Instruments/Key2","Instruments/Key3","Instruments/Key4","Instruments/Ranges","SignalView/XY","SignalView/Compare",
    "Lab.Sequence.A/Configuration","Lab.Sequence.B/Configuration","Lab.Digital/Configuration",
    "InputFraming/A","InputFraming/B","InputFraming/Replay","Recording/RotateMiB","Recording/RotateDaily","Recording/Limits"};return keys;}
WorkspaceSettings read_workspace_settings(const std::wstring& ini){WorkspaceSettings settings;std::vector<wchar_t> value(32768);
    for(const auto& key:workspace_keys()){const auto at=key.find('/');const auto section=uart_wide(key.substr(0,at)),name=uart_wide(key.substr(at+1));const auto n=GetPrivateProfileStringW(section.c_str(),name.c_str(),L"",value.data(),static_cast<DWORD>(value.size()),ini.c_str());if(n>=value.size()-1)throw std::runtime_error("Workspace preference truncated.");settings[key]=message_utf8(value.data());}return settings;
}
bool validate_workspace_settings(const WorkspaceSettings& settings,std::wstring& error){try{
    if(settings.size()!=workspace_keys().size())throw std::runtime_error("Workspace key count mismatch.");
    const auto get=[&](const std::string& key)->std::string{const auto it=settings.find(key);if(it==settings.end())throw std::runtime_error("Missing workspace key.");if(it->second.size()>32760||it->second.find_first_of("\r\n\0",0,3)!=std::string::npos)throw std::runtime_error("Invalid workspace value.");uart_wide(it->second);return it->second;};
    const auto number=[&](const std::string& key,double fallback,double low,double high){const auto s=get(key);if(s.empty())return fallback;double value=0;if(!uart_number(uart_wide(s),low,high,value)||value!=std::floor(value))throw std::runtime_error("Invalid workspace integer.");return value;};
    number("DualCOM/SplitPercent",50,25,75);SerialSettings serial;serial.port.device=uart_wide(get("DualCOM.B/Port"));if(serial.port.device.empty())serial.port.device=L"COM1";serial.port.instance_id=uart_wide(get("DualCOM.B/InstanceId"));if(serial.port.instance_id.size()>512)throw std::runtime_error("B identity too long.");
    serial.baud=static_cast<int>(number("DualCOM.B/Baud",115200,1,4000000));serial.data_bits=static_cast<int>(number("DualCOM.B/Data",8,5,8));serial.parity=static_cast<int>(number("DualCOM.B/Parity",0,0,2));serial.stop_bits=static_cast<int>(number("DualCOM.B/Stop",0,0,2));if(!valid_serial_settings(serial))throw std::runtime_error("Invalid B serial settings.");
    number("Recording/RotateMiB",32,1,1024);number("Recording/RotateDaily",1,0,1);
    for(const auto& key:workspace_keys()){const auto value=get(key);if(value.empty())continue;
        if(key.find("InputFraming/")==0)decode_text_framing(value);
        else if(key=="Recording/Limits")decode_recording_limits(value);
        else if(key.find("Workbench.Binary.")==0){auto bytes=uart_wide(value);if(bytes.rfind(L"v1:1:",0)!=0&&bytes.rfind(L"v1:0:",0)!=0)throw std::runtime_error("Invalid binary workspace schema.");std::string decoded;if(!uart_unhex(message_utf8(bytes.substr(5)),decoded))throw std::runtime_error("Invalid binary settings HEX.");auto fields=unpack_settings(L"v1:"+uart_wide(uart_hex(decoded)),11);fields.push_back(L"");BinaryFrameConfig config;std::wstring detail;if(!parse_binary_options(fields,config,detail))throw std::runtime_error("Invalid binary workspace values.");if((bytes[3]==L'1')!=(fields[0]==L"Enabled"))throw std::runtime_error("Binary enabled state mismatch.");}
        else if(key=="Replay.Binary/Configuration"){auto fields=unpack_settings(uart_wide(value),12);if(fields[0]!=L"Text"&&fields[0]!=L"Binary")throw std::runtime_error("Invalid replay extraction.");fields[0]=L"Enabled";BinaryFrameConfig config;std::wstring detail;if(!parse_binary_options(fields,config,detail))throw std::runtime_error("Invalid replay schema.");}
        else if(key=="Workbench.Text.B/Configuration"){const auto f=unpack_text_settings(uart_wide(value));TextTelemetryConfig c;const std::vector<std::wstring> modes{L"Key/value",L"CSV",L"Tab",L"JSON",L"Regex"};const auto found=std::find(modes.begin(),modes.end(),f[4]);if(found==modes.end()||(f[0]!=L"Enabled"&&f[0]!=L"Disabled")||(f[3]!=L"Text"&&f[3]!=L"Regex"))throw std::runtime_error("Invalid B text mode.");c.enabled=f[0]==L"Enabled";c.regex=f[3]==L"Regex";c.include=message_utf8(f[1]);c.exclude=message_utf8(f[2]);c.extraction.mode=static_cast<ExtractionConfig::Mode>(found-modes.begin());c.extraction.columns=message_utf8(f[5]);c.extraction.pattern=message_utf8(f[6]);double a=0,b=0;if(!uart_number(f[7],1,8,a)||!uart_number(f[8],1,8,b)||a!=std::floor(a)||b!=std::floor(b))throw std::runtime_error("Invalid B capture groups.");c.extraction.name_group=static_cast<int>(a);c.extraction.value_group=static_cast<int>(b);TextTelemetry parser;std::string detail;if(!parser.configure(c,detail))throw std::runtime_error(detail);}
        else if(key.find("Workbench.TX.")==0){std::istringstream input(value);std::string version,a,b;std::getline(input,version,':');std::getline(input,a,':');std::getline(input,b);double x=0,y=0;if(version!="v1"||!uart_number(uart_wide(a),0,1000,x)||!uart_number(uart_wide(b),0,60000,y)||x!=std::floor(x)||y!=std::floor(y))throw std::runtime_error("Invalid TX pacing.");}
        else if(key.find("Workbench.Framing.")==0){const auto f=unpack_settings(uart_wide(value),4);double m=0,b=0,t=0;if(!uart_number(f[0],0,5,m)||m!=std::floor(m)||!uart_number(f[1],1,65536,b)||b!=std::floor(b)||!uart_number(f[2],1,60000,t)||t!=std::floor(t))throw std::runtime_error("Invalid framing values.");TrafficFraming c;c.mode=static_cast<TrafficFraming::Mode>(static_cast<int>(m));c.bytes=static_cast<size_t>(b);c.idle_ms=static_cast<unsigned>(t);if(!uart_unhex(message_utf8(f[3]),c.header))throw std::runtime_error("Invalid framing HEX.");TrafficFramer check;std::string detail;if(!check.configure(c,detail))throw std::runtime_error(detail);}
        else if(key=="Derived/Configuration"){std::string definitions,detail;if(!uart_unhex(value,definitions))throw std::runtime_error("Invalid derived settings.");uart_wide(definitions);DerivedChannels check;if(!check.configure(definitions,detail))throw std::runtime_error(detail);}
        else if(key.find("Instruments/Key")==0){std::string name;if(!uart_unhex(value,name)||name.size()>160)throw std::runtime_error("Invalid instrument key.");uart_wide(name);}
        else if(key=="Instruments/Ranges"){std::istringstream input(value);std::vector<double> v;std::string s;while(std::getline(input,s,',')){double n=0;if(!uart_number(uart_wide(s),-1e100,1e100,n))throw std::runtime_error("Invalid instrument range.");v.push_back(n);}if(v.size()!=7||v[0]>=v[1]||v[2]>=v[3]||v[5]<0||v[5]>1e9||v[6]<32||v[6]>4096||v[6]!=std::floor(v[6])||(static_cast<unsigned>(v[6])&(static_cast<unsigned>(v[6])-1)))throw std::runtime_error("Invalid instrument limits.");}
        else if(key=="SignalView/XY"){const auto f=unpack_settings(uart_wide(value),2);for(const auto& name:f)if(message_utf8(name).size()>160)throw std::runtime_error("Invalid XY key.");}
        else if(key=="SignalView/Compare"){const auto f=unpack_settings(uart_wide(value),3);double a=0,b=0,c=0;if(!uart_number(f[0],-1e9,1e9,a)||!uart_number(f[1],0,3600,b)||!uart_number(f[2],0,1e100,c))throw std::runtime_error("Invalid comparison preferences.");}
        else if(key.find("Workbench.View.")==0){const auto f=unpack_settings(uart_wide(value),2);double mode=0;if(!uart_number(f[0],0,3,mode)||mode!=std::floor(mode)||(f[1]!=L"0"&&f[1]!=L"1"))throw std::runtime_error("Invalid A/B display preferences.");}
        else if(key.find("Lab.Sequence.")==0){const auto f=unpack_settings(uart_wide(value),3);double modulus=0,step=0;if(message_utf8(f[0]).size()>160||f[0].find_first_of(L"\r\n")!=f[0].npos||!uart_number(f[1],2,4294967296.0,modulus)||modulus!=std::floor(modulus)||!uart_number(f[2],1,modulus-1,step)||step!=std::floor(step))throw std::runtime_error("Invalid sequence workspace preferences.");}
        else if(key=="Lab.Digital/Configuration"){const auto f=unpack_settings(uart_wide(value),3);std::vector<DigitalLane> lanes;std::string detail;double window=0,hold=0;if(!parse_digital_lanes(message_utf8(f[0]),lanes,detail)||!uart_number(f[1],.1,86400,window)||!uart_number(f[2],.001,86400,hold))throw std::runtime_error("Invalid digital history workspace preferences.");}
    }return true;
}catch(const std::exception& e){error=uart_wide(e.what());return false;}}
