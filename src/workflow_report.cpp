#include "workflow_report.h"
#include "chart_analysis.h"
#include "plot_envelope.h"
#include "uart_files.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <cmath>
#include <stdexcept>
#include <locale>
namespace {
std::string html(std::string_view s){uart_wide(s);std::string out;for(unsigned char c:s){switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;case '\'':out+="&#39;";break;default:if(c>=32||c=='\n'||c=='\t')out+=c;else out+=' ';}}return out;}
std::string csv(const std::string& s){std::string out="\"";for(char c:s){out+=c;if(c=='"')out+='"';}return out+'"';}
}
std::string capture_case_csv(const std::string& input,const std::string& expected,const std::string& seconds){if(input.size()>4096||input.find('\0')!=input.npos||expected.size()>16384||seconds.size()>64)throw std::runtime_error("Case input/expected/time outside bounds.");uart_wide(input);uart_wide(expected);return csv(input)+','+csv(expected)+','+csv(seconds)+'\n';}
std::string capture_case_input(const CaptureRow& row){if(row.parser_prefix==UINT16_MAX||row.parser_prefix>row.raw.size())throw std::runtime_error("Select a retained text row; binary descriptions are not parser input.");const auto input=row.raw.substr(row.parser_prefix);if(input.size()>4096||input.find('\0')!=input.npos)throw std::runtime_error("Selected text input exceeds case bounds.");uart_wide(input);return input;}
std::string workflow_html(const CaptureSnapshot& data,const std::vector<std::string>& fields,const std::string& title,double first,double last,const std::atomic<bool>& cancel){
    if(fields.empty()||fields.size()>8||title.size()>512||!std::isfinite(first)||!std::isfinite(last)||first<0||first>last||data.rows.size()>10000||data.keys.size()>1024)throw std::runtime_error("Report: 1..8 unique fields, finite inclusive capture-time range, retained bounds only.");std::set<std::string> unique;
    std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(12)<<"<!doctype html><html lang=\"en\"><meta charset=\"utf-8\"><meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; style-src 'unsafe-inline'\"><title>"<<html(title)<<"</title><style>body{font:15px system-ui;max-width:1100px;margin:24px auto;color:#222}table{border-collapse:collapse}td,th{border:1px solid #aaa;padding:6px;text-align:left}svg{width:100%;background:#fafafa}path{fill:none;stroke:#333;stroke-width:1}text{font:13px sans-serif}</style><h1>"<<html(title)<<"</h1><p>Retained capture snapshot; inclusive capture-relative seconds "<<first<<" .. "<<last<<". Expired rows: "<<data.dropped<<". Source values, not display scaling. Host/synthetic clocks are not wire timing. Missing rows or generation changes break drawn segments. Plot rendering preserves first/min/max/last in each display bucket.</p>";
    for(const auto& key:fields){if(cancel)throw std::runtime_error("Cancelled.");if(!unique.insert(key).second)throw std::runtime_error("Duplicate report field.");const auto found=std::find(data.keys.begin(),data.keys.end(),key);if(found==data.keys.end())throw std::runtime_error("Report field not retained: "+key);const auto column=static_cast<size_t>(found-data.keys.begin());const auto stats=analyze_capture(data,column,first,last);out<<"<h2>"<<html(key)<<"</h2><table><tr><th>Samples</th><th>Min</th><th>Max</th><th>Mean</th><th>RMS</th><th>SD</th></tr><tr><td>"<<stats.count<<"</td>";for(double n:{stats.minimum,stats.maximum,stats.mean,stats.rms,stats.stddev}){out<<"<td>";if(stats.count&&std::isfinite(n))out<<n;else out<<"n/a";out<<"</td>";}out<<"</tr></table>";
        if(!stats.count)continue;const double scale=std::max({std::abs(stats.minimum),std::abs(stats.maximum),1e-100});double lo=stats.minimum/scale,hi=stats.maximum/scale;if(lo==hi){lo-=0.05;hi+=0.05;}const double span=last>first?last-first:1;
        out<<"<svg viewBox=\"0 0 1100 300\" role=\"img\" aria-label=\""<<html(key)<<"\"><text x=\"8\" y=\"18\">"<<stats.maximum<<"</text><text x=\"8\" y=\"265\">"<<stats.minimum<<"</text><text x=\"80\" y=\"290\">"<<first<<" s</text><text x=\"970\" y=\"290\">"<<last<<" s</text>";
        std::vector<ChannelSample> segment;uint64_t generation=UINT64_MAX;size_t rendered=0;
        const auto flush=[&]{if(segment.empty())return;const auto points=plot_envelope(segment,first,last>first?last:first+1,256);rendered+=points.size();if(rendered>40000)throw std::runtime_error("Report plot exceeds point budget.");out<<"<path d=\"";bool start=true;for(const auto& n:points){const auto x=80+1000*(n.t-first)/span,y=265-240*(n.v/scale-lo)/(hi-lo);out<<(start?'M':'L')<<x<<' '<<y<<' ';start=false;}out<<"\"/>";if(points.size()==1){const auto& point=points.front();out<<"<circle cx=\""<<80+1000*(point.t-first)/span<<"\" cy=\""<<265-240*(point.v/scale-lo)/(hi-lo)<<"\" r=\"2\" fill=\"#333\"/>";}segment.clear();};
        for(const auto& row:data.rows){if(cancel)throw std::runtime_error("Cancelled.");double n=0;if(!std::isfinite(row->seconds)||row->seconds<first||row->seconds>last)continue;if(!CaptureStore::value(*row,column,n)||!std::isfinite(n)){flush();generation=UINT64_MAX;continue;}if(generation!=row->generation){flush();generation=row->generation;}segment.push_back({row->seconds,n});}flush();out<<"</svg>";
    }out<<"<p>Generated by Simple COM Chart. This static report contains selected retained numeric data, no complete raw recording or executable script.</p></html>";auto result=out.str();if(result.size()>16*1024*1024)throw std::runtime_error("Report exceeds 16 MiB.");return result;
}
