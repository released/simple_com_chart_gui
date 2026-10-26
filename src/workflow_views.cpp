#include "workflow_views.h"
#include "uart_files.h"
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>
#include <stdexcept>
#include <locale>
namespace {
void key(const std::string& s,bool empty=false){if((s.empty()&&!empty)||s.size()>160||s.find('\0')!=s.npos)throw std::runtime_error("Invalid view key.");uart_wide(s);}
void validate(const ViewPreset& v){
    for(const auto* keys:{&v.checked,&v.columns}){if(keys->size()>1024)throw std::runtime_error("Too many view keys.");std::set<std::string> seen;for(const auto& k:*keys){key(k);if(!seen.insert(k).second)throw std::runtime_error("Duplicate view key.");}}
    for(const auto& k:v.charts)key(k,true);
    for(const auto& a:v.axes)if(!std::isfinite(a.window)||a.window<1||a.window>86400||!std::isfinite(a.low)||!std::isfinite(a.high)||a.low>=a.high||(a.fixed&&(std::abs(a.low)>1e100||std::abs(a.high)>1e100)))throw std::runtime_error("Invalid view axis: window 1..86400 s and fixed Y within +/-1e100.");
}
}
std::string encode_view(const ViewPreset& v){validate(v);std::ostringstream out;out.imbue(std::locale::classic());out<<"SimpleComChartView=1\n"<<std::setprecision(17);
    for(const auto& k:v.checked)out<<"C="<<uart_hex(k)<<'\n';for(const auto& k:v.columns)out<<"T="<<uart_hex(k)<<'\n';
    for(const auto& k:v.charts)out<<"P="<<uart_hex(k)<<'\n';for(const auto& a:v.axes)out<<"A="<<a.window<<' '<<a.fixed<<' '<<a.low<<' '<<a.high<<'\n';
    const auto s=out.str();if(s.size()>65536)throw std::runtime_error("View exceeds 64 KiB.");return s;
}
ViewPreset decode_view(const std::string& s){if(s.size()>65536||s.find('\0')!=s.npos)throw std::runtime_error("Invalid view file size.");std::istringstream in(s);in.imbue(std::locale::classic());std::string line;ViewPreset v;size_t plots=0,axes=0;bool tail=false;
    if(!std::getline(in,line)||line!="SimpleComChartView=1")throw std::runtime_error("Unsupported view version.");
    while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.size()<2||line[1]!='=')throw std::runtime_error("Invalid view row.");const auto body=line.substr(2);
        if(line[0]=='A'){if(axes>=5)throw std::runtime_error("Too many axes.");std::istringstream row(body);row.imbue(std::locale::classic());auto& a=v.axes[axes++];int fixed=-1;if(!(row>>a.window>>fixed>>a.low>>a.high)||fixed<0||fixed>1)throw std::runtime_error("Invalid view axis row.");row>>std::ws;if(!row.eof())throw std::runtime_error("Extra axis data.");a.fixed=fixed!=0;tail=true;}
        else {std::string k;if(!uart_unhex(body,k))throw std::runtime_error("Invalid view key encoding.");if(line[0]=='P'){if(tail||plots>=4)throw std::runtime_error("Invalid chart order.");v.charts[plots++]=k;}else if((line[0]=='C'||line[0]=='T')&&!plots&&!tail){auto& keys=line[0]=='C'?v.checked:v.columns;keys.push_back(k);if(keys.size()>1024)throw std::runtime_error("Too many view keys.");}else throw std::runtime_error("Unknown view row/order.");}}
    if(plots!=4||axes!=5)throw std::runtime_error("Incomplete view.");validate(v);return v;
}
