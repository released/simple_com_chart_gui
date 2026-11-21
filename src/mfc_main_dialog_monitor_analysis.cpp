#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "discovery_list.h"
#include "json_tree_dialog.h"
#include <sstream>
#include <iomanip>
namespace {
std::wstring number(double value){std::wostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(10)<<value;return s.str();}
MonitorAnalysisResult distribution_result(const Distribution& data,std::wstring summary){
    MonitorAnalysisResult result;result.title=L"Value / receipt distributions";result.columns={L"Bucket",L"Lower (inclusive)",L"Upper",L"Samples",L"Histogram"};
    result.summary=std::move(summary)+L"\r\n"+ui::tr(L"Finite samples / p50 / p95 / p99:")+L" "+std::to_wstring(data.count)+L" / "+(data.count?number(data.p50)+L" / "+number(data.p95)+L" / "+number(data.p99):ui::tr(L"n/a"));
    if(!data.count)return result;
    const auto maximum=*std::max_element(data.bins.begin(),data.bins.end());
    const auto edge=[&](size_t i){const double t=static_cast<double>(i)/32;return (1-t)*data.low+t*data.high;};
    for(size_t i=0;i<32;++i)result.rows.push_back({std::to_wstring(i),number(edge(i)),number(edge(i+1)),std::to_wstring(data.bins[i]),std::wstring(maximum?40*data.bins[i]/maximum:0,L'|')});
    return result;
}
}
void CMainDialog::monitor_timeline_view(){
    if(monitor_analysis_.busy()){show_status_message(ui::tr(L"Wait for snapshot analysis."),5000);return;}
    const auto a=primary_traffic_?primary_traffic_->snapshot():TrafficBatch{};
    const auto b=workbench_?workbench_->secondary_.traffic.snapshot():TrafficBatch{};
    if(!monitor_analysis_.start([a,b](const std::atomic<bool>& cancel){
        MonitorAnalysisResult result;result.title=L"A/B traffic timeline snapshot";result.first_column_sample=L"[2026-11-21T23:59:59.999Z] ";result.columns={L"PC receipt time",L"Port",L"Direction",L"Generation",L"Sequence",L"Bytes",L"HEX preview"};
        result.summary=ui::tr(L"Immutable retained A/B snapshot. Same-millisecond ties sort A then B; PC receipt order is not wire order. HEX preview: first32 bytes only. No forwarding or raw recording changes.")+L"\r\n"+ui::tr(L"Expired bytes A / B:")+L" "+std::to_wstring(a.expired)+L" / "+std::to_wstring(b.expired);
        for(const auto& row:monitor_timeline(a,b,cancel)){const auto& r=row.record;result.rows.push_back({r.receipt_ms>0?format_message_timestamp(r.receipt_ms,0,0,MessageClock::UtcIso):ui::tr(L"Unavailable"),row.source?L"B":L"A",r.tx?L"TX":L"RX",std::to_wstring(r.generation),std::to_wstring(r.sequence),std::to_wstring(r.bytes.size()),tx_preview(r.bytes,32)});}return result;
    },discovery_thread_launcher_))show_status_message(ui::tr(L"Cannot start background operation; try again."),8000);
    else show_status_message(ui::tr(L"Analyzing immutable snapshot in background..."),5000);
}
void CMainDialog::monitor_distribution_view(){
    if(monitor_analysis_.busy()){show_status_message(ui::tr(L"Wait for snapshot analysis."),5000);return;}
    MonitorModal modal(discovery_modal_);std::vector<std::wstring> keys;for(const auto& key:capture_.keys())keys.push_back(uart_wide(key));
    const auto first=capture_.rows().empty()?0:capture_.rows().front()->seconds,last=capture_.rows().empty()?0:capture_.rows().back()->seconds;
    UartToolForm form(L"Value / receipt distributions",L"Numeric values use raw units and inclusive capture-relative seconds, up to10000 retained samples. Receipt intervals use retained RX read chunks in milliseconds, not packets or device sampling jitter; TX/generation/sequence gaps are not bridged. No interpolation. 32 buckets; nearest-rank quantiles; final upper bound included.",
        {{L"Data",L"Numeric values",{L"Numeric values",L"RX receipt intervals"}},{L"Source",L"A",{L"A",L"B"}},{L"Channel",keys.empty()?L"":keys[0],keys},{L"From capture seconds",number(first)},{L"To capture seconds",number(last)}},{},[](const auto& f,std::wstring& error){double a=0,b=0;if(f[0]==L"Numeric values"&&(f[2].empty()||!uart_number(f[3],0,1e12,a)||!uart_number(f[4],a,1e12,b))){error=ui::tr(L"Select a channel and valid inclusive capture range.");return false;}return true;},this);
    form.accept_caption(L"Analyze snapshot");if(form.DoModal()!=IDOK||closing_serial_)return;const auto f=form.values();const unsigned source=f[1]==L"B";const bool receipt=f[0]==L"RX receipt intervals";const auto snapshot=receipt?CaptureSnapshot{}:capture_.snapshot();
    const auto traffic=receipt?(source?(workbench_?workbench_->secondary_.traffic.snapshot():TrafficBatch{}):(primary_traffic_?primary_traffic_->snapshot():TrafficBatch{})):TrafficBatch{};
    double a=0,b=0;if(!receipt){uart_number(f[3],0,1e12,a);uart_number(f[4],a,1e12,b);}
    if(!monitor_analysis_.start([snapshot,traffic,source,receipt,f,a,b](const std::atomic<bool>& cancel){
        const auto values=receipt?monitor_intervals(traffic,cancel):monitor_values(snapshot,message_utf8(f[2]),source,a,b,cancel);
        auto summary=(source?L"B":L"A")+std::wstring(L" | ")+(receipt?ui::tr(L"RX read-chunk receipt intervals (ms)"):f[2]+L" | "+number(a)+L".."+number(b)+L" s | "+ui::tr(L"Raw values; generations may be combined"));
        summary+=L"\r\n"+ui::tr(L"Retained snapshot only; no interpolation. Last bucket includes its upper bound.");
        return distribution_result(monitor_distribution(values,cancel),std::move(summary));
    },discovery_thread_launcher_))show_status_message(ui::tr(L"Cannot start background operation; try again."),8000);
    else show_status_message(ui::tr(L"Analyzing immutable snapshot in background..."),5000);
}
void CMainDialog::poll_monitor_analysis(){
    if(discovery_modal_||!::IsWindowEnabled(m_hWnd))return;
    MonitorAnalysisResult result;std::wstring error;
    if(!monitor_analysis_.take(result,error)||discovery_close_pending_||closing_serial_)return;
    if(!error.empty()){show_status_message(ui::tr(error),8000);return;}
    MonitorModal modal(discovery_modal_);DiscoveryListDialog list(this,result.title,std::move(result.columns),std::move(result.rows),std::move(result.summary),L"Close");list.first_column_sample(std::move(result.first_column_sample));list.DoModal();
}
void CMainDialog::monitor_json_paths(){
    MonitorModal modal(discovery_modal_);const auto row=data_page_.selected_row();
    std::wstring suggestion;
    // A retained row is optional input, independent of the smaller case-tool limit.
    if(row&&row->parser_prefix!=UINT16_MAX&&row->parser_prefix<=row->raw.size()){
        const auto input=std::string_view(row->raw).substr(row->parser_prefix);
        if(input.size()<=65536&&input.find('\0')==input.npos){
            try{suggestion=uart_wide(input);}catch(const std::exception&){}
        }
    }
    UartToolForm sample(L"JSON numeric paths",L"Offline sample only: up to64 KiB UTF-8, depth16 and4096 nodes. Duplicate object keys are rejected. The selected retained row is a suggestion; no live parser changes.",{{L"JSON sample",suggestion,{},10,65536}},{},[](const auto& f,std::wstring& error){try{json_numeric_tree(message_utf8(f[0]));return true;}catch(const std::exception& e){error=ui::tr(uart_wide(e.what()));return false;}},this);
    sample.accept_caption(L"Inspect paths");if(sample.DoModal()!=IDOK||closing_serial_)return;
    JsonTreeDialog tree(this,json_numeric_tree(message_utf8(sample.values()[0])));if(tree.DoModal()!=IDOK||closing_serial_)return;
    show_advanced_tools(AdvancedTools::Extraction);advanced_tools_.set_field(AdvancedTools::Extraction,0,L"JSON");advanced_tools_.set_field(AdvancedTools::Extraction,1,uart_wide(tree.selection()));advanced_tools_.set_field(AdvancedTools::Extraction,7,sample.values()[0]);
    advanced_tools_.notice(AdvancedTools::Extraction,ui::tr(L"JSON paths staged only. Review aliases, time settings and Preview; Apply explicitly. Live parser and INI unchanged."));
}
