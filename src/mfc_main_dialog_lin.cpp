#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "uart_files.h"
#include "discovery_list.h"
#include <sstream>
#include <iomanip>

namespace {
std::wstring hex_byte(unsigned n){const wchar_t digits[]=L"0123456789ABCDEF";return {digits[(n>>4)&15],digits[n&15]};}
std::wstring number(double n){std::wostringstream out;out<<std::setprecision(12)<<n;return out.str();}
std::string trace_header(){return "SCC-LIN-TRACE,1\nseconds,id,pid,data,checksum,checksum_type,direction,break\n";}
}
void CMainDialog::refresh_lin_page() {
    ProtocolPage::Rows rows;size_t invalid=0,unverified=0;
    if(lin_frames_){const auto first=lin_frames_->size()>4096?lin_frames_->size()-4096:0;
        for(size_t at=0;at<lin_frames_->size();++at){const auto& frame=(*lin_frames_)[at];const auto valid=validate_lin_frame(frame);
            if(!valid.chartable(frame,true))++invalid;else if(!valid.chartable(frame))++unverified;
            if(at<first)continue;std::wstring signals;
            if(valid.chartable(frame,true))for(const auto& value:decode_lin_values(frame,lin_signals_)){if(!signals.empty())signals+=L"; ";signals+=uart_wide(value.key)+L"="+number(value.value);if(!value.unit.empty())signals+=L" "+uart_wide(value.unit);if(!value.text.empty())signals+=L" ("+uart_wide(value.text)+L")";if(signals.size()>1024){signals+=L" …";break;}}
            const auto checksum=frame.checksum<0?ui::tr(L"Unknown"):hex_byte(static_cast<unsigned>(frame.checksum));
            rows.push_back({number(frame.seconds),hex_byte(frame.id),frame.pid<0?ui::tr(L"Unknown"):hex_byte(static_cast<unsigned>(frame.pid)),uart_wide(uart_hex(frame.data)),checksum,
                ui::tr(uart_wide(lin_check_name(valid.pid))),ui::tr(uart_wide(lin_check_name(valid.checksum))),ui::tr(uart_wide(lin_check_name(frame.break_check))),uart_wide(frame.direction),signals});
        }
    }
    lin_page_.set_rows(std::move(rows),false);
    lin_page_.summary(ui::tr(L"Source frames: ")+std::to_wstring(lin_frames_?lin_frames_->size():0)+ui::tr(L" | Displayed: ")+std::to_wstring(lin_page_.rows().size())+
        ui::tr(L" | Invalid/header-only: ")+std::to_wstring(invalid)+ui::tr(L" | Unverified: ")+std::to_wstring(unverified)+ui::tr(L" | Signals: ")+std::to_wstring(lin_signals_.size())+
        (lin_ldf_source_.empty()?L"":ui::tr(L" | LDF subset staged")));
}
bool CMainDialog::apply_lin_chart() {
    if(protocol_file_.busy()||exporting_||profile_job_.busy()||!advanced_idle()||replay_.data()||replay_load_.busy()||recorder_.status().active||observation_only_||lin_scheduler_.active()||lin_scheduler_.draining()){
        lin_page_.status(ui::tr(L"Disconnect both ports, unload replay and finish file/recording work before applying LIN charts."));return false;
    }
    if(!lin_frames_||lin_frames_->empty()||lin_signals_.empty()){lin_page_.status(ui::tr(L"Load LIN frames and define signals first."));return false;}
    const bool allow=lin_page_.allow_unknown();bool available=false;
    for(const auto& frame:*lin_frames_)if(validate_lin_frame(frame).chartable(frame,allow)&&!decode_lin_values(frame,lin_signals_).empty()){available=true;break;}
    if(!available){lin_page_.status(ui::tr(L"No accepted LIN frames match the signal definitions. Review metadata, checksum and bit ranges."));return false;}
    reset_numeric_capture();lin_chart_frames_=lin_frames_;lin_chart_signals_=lin_signals_;lin_chart_position_=lin_chart_skipped_=0;
    lin_chart_unknown_=allow;lin_offline_chart_=true;++lin_chart_generation_;lin_chart_now_=1;
    lin_page_.status(ui::tr(L"Applying LIN signals in bounded batches. Raw Messages and recordings are unchanged."));return true;
}
void CMainDialog::protocol_lin_action(unsigned action) {
    if(closing_serial_)return;
    if(action==8){protocol_file_.cancel();++protocol_epoch_;lin_scheduler_.stop();
        if(lin_offline_chart_)reset_numeric_capture();lin_frames_.reset();lin_chart_frames_.reset();refresh_lin_page();
        lin_page_.status(ui::tr(L"LIN trace cleared; staged definitions retained. Pending file results will be discarded."));return;
    }
    if(protocol_file_.busy()||exporting_||profile_job_.busy()){lin_page_.status(ui::tr(L"Finish the current protocol file or export operation first."));return;}
    if(lin_scheduler_.active()||lin_scheduler_.draining()){lin_page_.status(ui::tr(L"Stop or drain the LIN schedule before editing or loading data."));return;}
    lin_page_.status(L"");
    if(action==1||action==4||action==11){MonitorModal modal(discovery_modal_);
        const auto kind=action==1?1U:action==4?3U:2U;
        CFileDialog dialog(TRUE,kind==1?L"csv":kind==3?L"ldf":L"scclin",nullptr,OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR,
            ui::caption(kind==1?L"LIN frame CSV (*.csv;*.log)|*.csv;*.log||":kind==3?L"LIN definition (*.ldf)|*.ldf||":L"LIN signals (*.scclin)|*.scclin||"),this);
        if(dialog.DoModal()==IDOK)begin_protocol_file(std::filesystem::path(dialog.GetPathName().GetString()),kind);return;
    }
    if(action==5||action==7){save_protocol_file(true,action==7);return;}
    if(action==6){apply_lin_chart();return;}
    if(action==9){lin_schedule_dialog();return;}
    if(action==10){MonitorModal modal(discovery_modal_);const auto caps=lin_transport_?lin_transport_->capabilities():LinCapabilities{};
        const auto yes=[](bool flag){return ui::tr(flag?L"Available":L"Unavailable");};
        DiscoveryListDialog dialog(this,L"LIN interface capabilities",{L"Capability",L"State"},
            {{ui::tr(L"Interface"),ui::tr(uart_wide(caps.name))},{ui::tr(L"Receive frames"),yes(caps.receive)},
             {ui::tr(L"Transmit headers"),yes(caps.header)},{ui::tr(L"Publish responses"),yes(caps.publish)},
             {ui::tr(L"Hardware schedule"),yes(caps.hardware_schedule)},{ui::tr(L"Hardware timestamps"),yes(caps.hardware_clock)}},
            ui::tr(L"Offline files are available without a driver. Live LIN needs a selected compatible adapter and its driver or documented gateway protocol. A generic UART Break is not a LIN timing measurement."),L"Close");dialog.DoModal();return;
    }
    if(action==2){MonitorModal modal(discovery_modal_);auto initial=trace_header();if(lin_frames_&&lin_frames_->size()<=100)initial=encode_lin_trace(*lin_frames_);
        UartToolForm form(L"LIN frame input",L"Native LIN trace CSV: seconds,id,pid,data,checksum,checksum_type,direction,break. ID/PID/checksum are HEX; missing PID/checksum stay blank. Types: Unknown/Classic/Enhanced; direction: RX/TX/Unknown; Break: Unknown/Valid/Invalid. Saving replaces the staged trace, not the chart capture.",
            {{L"LIN frame CSV",uart_wide(initial),{},10,65536}},
            [](const auto& f){const auto frames=decode_lin_trace(message_utf8(f[0]));size_t valid=0;for(const auto& frame:frames)if(validate_lin_frame(frame).chartable(frame))++valid;return ui::tr(L"Source frames: ")+std::to_wstring(frames.size())+ui::tr(L" | Verified frames: ")+std::to_wstring(valid);},
            [](const auto& f,std::wstring& error){try{(void)decode_lin_trace(message_utf8(f[0]));return true;}catch(const std::exception& e){error=ui::tr(uart_wide(e.what()));return false;}},this);
        form.accept_caption(L"Stage LIN trace");if(form.DoModal()!=IDOK)return;
        lin_frames_=std::make_shared<const std::vector<LinFrame>>(decode_lin_trace(message_utf8(form.values()[0])));refresh_lin_page();lin_page_.status(ui::tr(L"LIN trace staged; chart data unchanged until Apply."));return;
    }
    if(action==3){MonitorModal modal(discovery_modal_);auto manual=lin_signals_;for(auto& signal:manual){signal.minimum.reset();signal.maximum.reset();signal.logical.clear();}
        UartToolForm form(L"LIN signal definitions",L"Little-endian scalar bits: name,id,start_bit,bits,signed,scale,add,unit. ID is HEX; bits 1..32, signed 0/1. Saving manual definitions replaces staged LDF ranges, logical text and schedules. Existing charts retain their previous capture until Apply.",
            {{L"LIN signal CSV",uart_wide(encode_lin_signals(manual)),{},10,65536}},
            [](const auto& f){return ui::tr(L"Signals: ")+std::to_wstring(decode_lin_signals(message_utf8(f[0])).size());},
            [](const auto& f,std::wstring& error){try{(void)decode_lin_signals(message_utf8(f[0]));return true;}catch(const std::exception& e){error=ui::tr(uart_wide(e.what()));return false;}},this);
        form.accept_caption(L"Stage signal definitions");if(form.DoModal()!=IDOK)return;
        lin_signals_=decode_lin_signals(message_utf8(form.values()[0]));lin_database_={};lin_ldf_source_.clear();refresh_lin_page();lin_page_.status(ui::tr(L"LIN signals staged; chart data unchanged until Apply."));
    }
}
void CMainDialog::lin_schedule_dialog() {
    if(lin_database_.schedules.empty()){lin_page_.status(ui::tr(L"Load an LDF with unconditional schedule tables first."));return;}
    const auto definitions=lin_database_.schedules;std::vector<std::wstring> names;for(const auto& s:definitions)names.push_back(uart_wide(s.name));
    const auto select=[definitions,names](const std::vector<std::wstring>& f,LinSchedule& out,unsigned& cycles,std::wstring& error){
        if(f.size()!=2||f[1].empty()||f[1].size()>5||f[1].find_first_not_of(L"0123456789")!=std::wstring::npos){error=ui::tr(L"Cycles must be a decimal integer 1..10000.");return false;}
        const auto found=std::find(names.begin(),names.end(),f[0]);if(found==names.end())return false;cycles=static_cast<unsigned>(std::stoul(f[1]));out=definitions[static_cast<size_t>(found-names.begin())];std::string detail;
        if(!validate_lin_schedule(out,cycles,detail)){error=ui::tr(uart_wide(detail));return false;}return true;
    };
    const auto generation=lin_transport_?lin_transport_->generation():0;const auto caps=lin_transport_?lin_transport_->capabilities():LinCapabilities{};MonitorModal modal(discovery_modal_);
    UartToolForm form(L"LIN schedule",L"Preview unconditional LDF slots and finite cycles. Hardware execution requires a configured adapter with hardware scheduling. It stops on cancellation, error or connection change and never resumes automatically. One-hour maximum; accepted effects cannot be revoked.",
        {{L"Schedule",names.front(),names},{L"Cycles",L"1"}},
        [select](const auto& f){LinSchedule schedule;unsigned cycles=0;std::wstring error;if(!select(f,schedule,cycles,error))return error;uint64_t duration=0;std::wstring out=ui::tr(L"Preview only; no transmission.");
            for(const auto& slot:schedule.slots){duration+=slot.delay_ms;out+=L"\r\nID "+hex_byte(slot.id)+L" | "+ui::tr(slot.publish?L"Header + response":L"Header only")+L" | "+std::to_wstring(slot.delay_ms)+L" ms | "+uart_wide(uart_hex(slot.data));}
            return out+L"\r\n"+ui::tr(L"Requested duration (ms): ")+std::to_wstring(duration*cycles);},
        [select](const auto& f,std::wstring& error){LinSchedule schedule;unsigned cycles=0;return select(f,schedule,cycles,error);},this);
    form.accept_caption(L"Start hardware schedule");form.accept_enabled(lin_transport_&&caps.hardware_schedule&&caps.header&&!observation_only_&&advanced_idle()&&!replay_.data());
    if(form.DoModal()!=IDOK)return;
    if(!lin_transport_||generation!=lin_transport_->generation()||observation_only_||!advanced_idle()||replay_.data()){lin_page_.status(ui::tr(L"LIN interface state changed; review the schedule again."));return;}
    LinSchedule schedule;unsigned cycles=0;std::wstring error;std::string detail;
    if(!select(form.values(),schedule,cycles,error)){lin_page_.status(error);return;}
    if(!lin_scheduler_.start(schedule,cycles,lin_transport_.get(),GetTickCount64(),detail))lin_page_.status(ui::tr(uart_wide(detail)));
    else lin_page_.status(ui::tr(L"Hardware LIN schedule started; use Clear LIN view to request Stop."));
}
