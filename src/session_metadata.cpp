#include "session_metadata.h"
#include "uart_files.h"
#include <sstream>
#include <set>
#include <stdexcept>
UartProfile session_safe_profile(UartProfile p){
    p.serial.port={};if(p.workspace){p.workspace_settings["DualCOM.B/Port"]="";p.workspace_settings["DualCOM.B/InstanceId"]="";}return p;
}
std::string encode_capture_metadata(UartProfile p,const std::string& version,const std::string& source){
    if(version.size()!=9||(source!="A"&&source!="B"&&source!="Current"))throw std::runtime_error("Invalid capture origin/version.");
    for(size_t i=0;i<version.size();++i)if((i==1||i==3||i==5)?version[i]!='.':(version[i]<'0'||version[i]>'9'))throw std::runtime_error("Invalid capture version.");
    const auto profile=encode_profile(session_safe_profile(std::move(p)));
    return "SCC_CAPTURE_V1\r\nVersion="+version+"\r\nSource="+source+"\r\nProfile="+uart_hex(profile)+"\r\n";
}
CaptureMetadata decode_capture_metadata(std::string_view input){
    if(input.size()>1100*1024)throw std::runtime_error("Capture metadata exceeds its bound.");
    std::istringstream in{std::string(input)};std::string line;if(!std::getline(in,line)||line!="SCC_CAPTURE_V1\r")throw std::runtime_error("Unsupported capture metadata.");
    CaptureMetadata out;std::set<std::string> seen;
    while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();const auto eq=line.find('=');if(eq==line.npos||!seen.insert(line.substr(0,eq)).second)throw std::runtime_error("Malformed/duplicate capture metadata.");const auto key=line.substr(0,eq),v=line.substr(eq+1);
        if(key=="Version")out.version=v;else if(key=="Source")out.source=v;else if(key=="Profile"){std::string p;if(!uart_unhex(v,p))throw std::runtime_error("Invalid capture profile HEX.");out.profile=decode_profile(p);}else if(key=="Changed"&&v=="1")out.changed=true;else throw std::runtime_error("Unknown capture metadata key.");
    }
    if(seen.size()!=(out.changed?4:3)||!seen.count("Profile")||!seen.count("Version")||!seen.count("Source"))throw std::runtime_error("Incomplete capture metadata.");
    encode_capture_metadata(out.profile,out.version,out.source);
    if(!out.profile.serial.port.device.empty()||!out.profile.serial.port.instance_id.empty()||(out.profile.workspace&&(!out.profile.workspace_settings.at("DualCOM.B/Port").empty()||!out.profile.workspace_settings.at("DualCOM.B/InstanceId").empty())))throw std::runtime_error("Capture metadata contains device identifiers; explicit sanitized settings are required.");return out;
}
