#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_files.h"
#include "uart_tool_form.h"
#include "discovery_list.h"
#include <sstream>
#include <iomanip>

namespace {
std::wstring decimal(double value){std::wostringstream out;out<<std::setprecision(10)<<value;return out.str();}
}
void CMainDialog::begin_protocol_file(const std::filesystem::path& path,unsigned kind,size_t port) {
    if(kind>3||port>1||protocol_file_.busy()||exporting_||profile_job_.busy()||closing_serial_)return;
    protocol_file_epoch_=protocol_epoch_;
    const bool started=protocol_file_.start([path,kind,port](const std::atomic<bool>& cancel){
        ProtocolFileResult result;result.kind=kind;result.port=port;
        const auto bytes=uart_read_file(path,kind==1?2*1024*1024:65536,cancel);
        if(cancel)throw std::runtime_error("Cancelled; prior data retained");
        if(kind==0)result.points=decode_modbus_points(bytes);
        else if(kind==1)result.frames=std::make_shared<const std::vector<LinFrame>>(decode_lin_trace(bytes));
        else if(kind==2)result.signals=decode_lin_signals(bytes);
        else{result.database=parse_lin_ldf(bytes);result.source=bytes;}
        if(cancel)throw std::runtime_error("Cancelled; prior data retained");return result;
    });
    auto& page=kind==0?modbus_page_:lin_page_;
    page.status(ui::tr(started?L"Loading protocol file in background; current data retained until validation completes.":L"Cannot start protocol file worker; retry is available."));
    page.state(protocol_file_.busy());
}
bool CMainDialog::begin_protocol_export(const std::filesystem::path& path,bool lin,bool trace) {
    if(exporting_||protocol_file_.busy())return false;
    std::function<std::string()> snapshot;
    if(lin&&trace){const auto frames=lin_frames_;if(!frames||frames->empty())return false;snapshot=[frames]{return encode_lin_trace(*frames);};}
    else if(lin){if(!lin_ldf_source_.empty()){auto source=lin_ldf_source_;snapshot=[source=std::move(source)]{return source;};}
        else{auto signals=lin_signals_;snapshot=[signals=std::move(signals)]{return encode_lin_signals(signals);};}}
    else{auto points=modbus_points_[modbus_page_.target()];if(points.empty())return false;snapshot=[points=std::move(points)]{return encode_modbus_points(points);};}
    auto committed=std::make_shared<uint64_t>(0);
    return begin_export_job([path,snapshot=std::move(snapshot),committed]{const auto bytes=snapshot();const std::atomic<bool> cancel{false};uart_write_file(path,bytes,true,cancel);*committed=bytes.size();},ui::tr(L"Protocol snapshot saved."),committed);
}
void CMainDialog::save_protocol_file(bool lin,bool trace) {
    if(exporting_||protocol_file_.busy())return;MonitorModal modal(discovery_modal_);
    const wchar_t* extension=lin?(trace?L"csv":lin_ldf_source_.empty()?L"scclin":L"ldf"):L"sccmodbus";
    const wchar_t* filter=lin?(trace?L"LIN trace (*.csv)|*.csv||":lin_ldf_source_.empty()?L"LIN signals (*.scclin)|*.scclin||":L"LIN definition (*.ldf)|*.ldf||"):
        L"Modbus definitions (*.sccmodbus)|*.sccmodbus||";
    CFileDialog dialog(FALSE,extension,nullptr,OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR,ui::caption(filter),this);
    if(dialog.DoModal()!=IDOK)return;
    if(!begin_protocol_export(std::filesystem::path(dialog.GetPathName().GetString()),lin,trace))
        (lin?lin_page_:modbus_page_).status(ui::tr(L"No protocol data to save, or a file job is busy."));
}
void CMainDialog::poll_protocols() {
    const auto schedule_status=lin_scheduler_.status();lin_scheduler_.poll(GetTickCount64());
    if(schedule_status!=lin_scheduler_.status())lin_page_.status(ui::tr(uart_wide(lin_scheduler_.status())));
    ProtocolFileResult result;std::wstring error;
    if(protocol_file_.take(result,error)) {
        if(protocol_file_epoch_!=protocol_epoch_){modbus_page_.status(ui::tr(L"Stale protocol file result discarded; current data retained."));lin_page_.status(ui::tr(L"Stale protocol file result discarded; current data retained."));}
        else if(!error.empty()){modbus_page_.status(ui::tr(error));lin_page_.status(ui::tr(error));}
        else if(result.kind==0){
            if(workbench_&&workbench_->rtu_[result.port].active())modbus_page_.status(ui::tr(L"Polling state changed; stop polling before loading definitions."));
            else{modbus_points_[result.port]=std::move(result.points);modbus_values_[result.port].clear();modbus_page_.set_rows({},false);refresh_modbus_page();modbus_page_.status(ui::tr(L"Definitions staged. Start polling is a separate explicit action."));}
        }else if(result.kind==1){lin_frames_=std::move(result.frames);refresh_lin_page();lin_page_.status(ui::tr(L"LIN trace loaded. Apply to charts is a separate explicit action."));}
        else if(result.kind==2){lin_signals_=std::move(result.signals);lin_database_={};lin_ldf_source_.clear();refresh_lin_page();lin_page_.status(ui::tr(L"LIN signals staged; chart data unchanged until Apply."));}
        else{lin_database_=std::move(result.database);lin_signals_=lin_database_.signals;lin_ldf_source_=std::move(result.source);refresh_lin_page();lin_page_.status(ui::tr(L"LDF subset validated and staged. Node configuration is not applied."));}
    }
    if(lin_chart_frames_&&lin_chart_position_<lin_chart_frames_->size()) {
        const auto begin=GetTickCount64();bool changed=false;
        for(size_t count=0;lin_chart_position_<lin_chart_frames_->size()&&count<64&&(count==0||GetTickCount64()-begin<8);++count,++lin_chart_position_){
            const auto& frame=(*lin_chart_frames_)[lin_chart_position_];const auto valid=validate_lin_frame(frame);
            if(!valid.chartable(frame,lin_chart_unknown_)){++lin_chart_skipped_;continue;}
            const auto decoded=decode_lin_values(frame,lin_chart_signals_);std::unordered_map<std::string,double> values;
            for(const auto& value:decoded)values[value.key]=value.value;
            if(values.empty()){++lin_chart_skipped_;continue;}
            lin_chart_now_=frame.seconds+1;accept_numeric("LIN frame "+std::to_string(frame.id)+": "+uart_hex(frame.data),values,lin_chart_now_,UINT64_MAX,0,2,lin_chart_generation_,UINT16_MAX);changed=true;
        }
        if(changed){sync_channels();if(::IsWindowVisible(data_page_.hwnd()))data_page_.refresh(now_seconds());if(!snapshot_&&::IsWindowVisible(plot_view_.hwnd()))plot_view_.update_from_model(now_seconds());}
        if(lin_chart_position_==lin_chart_frames_->size())lin_page_.status(ui::tr(L"LIN chart import complete; skipped frames: ")+std::to_wstring(lin_chart_skipped_)+ui::tr(L". Select channels in Chart / 4 Charts. File times are not wire measurements."));
    }
    if(GetTickCount64()>=protocol_refresh_at_){protocol_refresh_at_=GetTickCount64()+250;if(modbus_page_.GetSafeHwnd()&&modbus_page_.IsWindowVisible())refresh_modbus_page();}
    const bool busy=protocol_file_.busy()||exporting_||profile_job_.busy();
    const auto port=modbus_page_.target();const bool running=workbench_&&workbench_->rtu_[port].active();
    modbus_page_.state(busy,running,!modbus_points_[port].empty());
    lin_page_.state(busy||(lin_chart_frames_&&lin_chart_position_<lin_chart_frames_->size()),false,lin_frames_&&!lin_frames_->empty());
    if(close_after_protocol_&&!protocol_file_.busy()&&!lin_scheduler_.draining()){close_after_protocol_=false;PostMessage(WM_CLOSE);}
}
void CMainDialog::refresh_modbus_page() {
    if(!workbench_)return;const auto port=modbus_page_.target();const auto& poller=workbench_->rtu_[port];
    const bool running=poller.active();const auto& points=modbus_points_[port];const auto& plan=poller.plan();
    const auto size=running?plan.size():points.size();ProtocolPage::Rows rows;
    for(size_t n=0;n<size;++n){const auto& config=running?plan[n]:points[n].request;
        const auto field=config.fields.empty()?nullptr:&config.fields.front();
        const auto name=running?(field?uart_wide(field->name):ui::tr(L"Raw block ")+std::to_wstring(n+1)):uart_wide(points[n].name);
        const auto type=running?(field?uart_wide(field->type):L"raw"):uart_wide(points[n].type);
        const auto order=running?(field?uart_wide(field->order):L""):uart_wide(points[n].order);
        std::wstring value=ui::tr(L"Unavailable"),age=L"",state=ui::tr(running?L"Waiting":L"Stopped");
        const auto found=modbus_values_[port].find(n);
        if(found!=modbus_values_[port].end()){const auto& sample=found->second;
            if(sample.unit==config.unit&&sample.function==config.function&&sample.address==config.address){
                value.clear();if(!sample.values.empty())for(size_t k=0;k<std::min<size_t>(4,sample.values.size());++k){if(k)value+=L", ";value+=decimal(sample.values[k].second);}
                else if(sample.raw)for(size_t k=0;k<std::min<size_t>(8,sample.registers.size());++k){if(k)value+=L", ";value+=std::to_wstring(sample.registers[k]);}
                if(value.empty())value=ui::tr(L"Unavailable");
                else {
                    if(sample.registers.size()>8)value+=L" …";
                    if(n<points.size()&&!points[n].unit.empty()&&(!running||(field&&field->name==points[n].name)))value+=L" "+uart_wide(points[n].unit);
                }
                const auto elapsed=GetTickCount64()>=sample.observed_ms?GetTickCount64()-sample.observed_ms:0;age=std::to_wstring(elapsed);
                const bool current=running&&elapsed<=static_cast<uint64_t>(config.period_ms)+config.timeout_ms;
                state=ui::tr(current?L"Current":L"Stale; last successful value");
            }
        }
        rows.push_back({name,std::to_wstring(config.unit),std::to_wstring(config.function),std::to_wstring(config.address),std::to_wstring(modbus_reference(config.function,config.address)),std::to_wstring(config.count),type,order,value,age,state});
    }
    modbus_page_.set_rows(std::move(rows));const auto& d=poller.diagnostics();
    std::wstring summary=ui::tr(L"Read-only RTU | ")+ui::tr(running?L"Polling":L"Stopped")+L" | "+ui::tr(L"Requests: ")+std::to_wstring(d.requests)+L" | "+ui::tr(L"Responses: ")+std::to_wstring(d.responses)+L" | CRC: "+std::to_wstring(d.crc_errors)+L" | "+ui::tr(L"Length errors: ")+std::to_wstring(d.length_errors);
    if(!poller.transactions().empty())summary+=L"\r\n"+ui::tr(L"Last outcome: ")+ui::tr(uart_wide(rtu_outcome_name(poller.transactions().back().outcome)));
    modbus_page_.summary(summary);
}
