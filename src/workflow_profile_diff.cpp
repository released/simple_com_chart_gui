#include "workflow_profile_diff.h"
#include "uart_files.h"
#include <map>
#include <sstream>
namespace {
std::map<std::string,std::string> entries(const UartProfile& p){std::map<std::string,std::string> result;std::istringstream in(encode_profile(p));std::string line;std::getline(in,line);while(std::getline(in,line)){const auto at=line.find('=');result.emplace(line.substr(0,at),line.substr(at+1));}return result;}
std::string display(const std::string& key,const std::string& value){if(key=="Port"||key=="Instance"||key=="Include"||key=="Exclude"||key=="Extraction"||key=="ChannelDisplay"||key.rfind("Chart",0)==0||key.rfind("Workspace.",0)==0){std::string out;if(uart_unhex(value,out))return out;}return value;}
}
std::vector<ProfileDifference> profile_differences(const UartProfile& current,const UartProfile& staged){auto a=entries(current),b=entries(staged);std::map<std::string,bool> keys;for(const auto& x:a)keys[x.first]=true;for(const auto& x:b)keys[x.first]=true;std::vector<ProfileDifference> out;for(const auto& x:keys){if(x.first.rfind("Workspace.",0)==0&&!b.count(x.first))continue;const auto l=a.find(x.first),r=b.find(x.first);if(l==a.end()||r==b.end()||l->second!=r->second)out.push_back({x.first,l==a.end()?"(default)":display(x.first,l->second),r==b.end()?"(default)":display(x.first,r->second)});}return out;}
