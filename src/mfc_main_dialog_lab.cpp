#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "packet_builder.h"
#include "lab_file_actions.h"
#include <sstream>
namespace {
std::wstring report_text(std::string_view bytes){MessageDisplayOptions options;options.encoding=MessageEncoding::Utf8;return format_message(bytes,0,{},options).text;}
}

void CMainDialog::show_uart_lab() {
    if(!lab_.show(this)){show_status_message(L"Cannot open UART Laboratory",5000);return;}
    workbench_feed_enabled_=true;
    load_lab_preferences();lab_input_request({UartLabWindow::Input,0,lab_.values(UartLabWindow::Input)});lab_.notice(UartLabWindow::Diagnostics,report_text(lab_diagnostics()));
}
std::string CMainDialog::lab_diagnostics()const {
    std::ostringstream out;const auto& raw=message_page_.buffer();
    out<<"PORT A "<<(replay_.data()?"OFFLINE":serial_mgr_->is_connected()?"CONNECTED":"DISCONNECTED")
        <<"\r\nRetained Messages bytes: "<<raw.bytes().size()<<"; expired offset: "<<raw.offset()<<"; skipped bytes: "<<raw.skipped()
        <<"\r\nPending line loss: "<<pending_line_loss_<<"; oversized lines: "<<oversized_lines_.load()
        <<"\r\nText pipeline (current numeric capture):\r\n"<<diagnostics_.report();
    if(workbench_) {
        out<<"\r\nPORT B "<<(workbench_->secondary_.connected()?"CONNECTED":"DISCONNECTED")<<"; text enabled: "<<workbench_->text_b_.config().enabled
            <<"\r\n"<<workbench_->text_b_.diagnostics().report()<<"B skipped/queue/oversized: "<<workbench_->text_b_.skipped()<<"\r\n";
        for(size_t i=0;i<2;++i){const auto& b=workbench_->binary_[i];out<<"Binary "<<(i?'B':'A')<<" enabled="<<b.enabled<<", frames="<<b.frames<<", CRC="<<b.decoder.checksum_errors()
            <<", layout="<<b.decoder.layout_errors()<<", codec="<<b.decoder.codec_errors()<<", unknown ID="<<b.decoder.unknown_types()<<", discarded bytes="<<b.decoder.discarded()<<", history/output gaps="<<b.lost<<"\r\n";}
    }
    if(replay_.data()&&replay_binary_enabled_)out<<"Offline binary: CRC="<<replay_crc_errors_<<", layout="<<replay_layout_errors_<<", codec="<<replay_codec_errors_<<", unknown ID="<<replay_unknown_types_<<", discarded bytes="<<replay_discarded_<<"\r\n";
    out<<"\r\nCapture rows: "<<capture_.rows().size()<<"; expired rows: "<<capture_.dropped()<<"; discovered keys: "<<capture_.keys().size()
        <<"\r\nChart selected: "<<model_.get_enabled_count()<<"; Snapshot: "<<snapshot_<<"; 4 Charts: ";
    for(size_t i=0;i<4;++i)out<<(i?", ":"")<<(data_page_.selected_field(i).empty()?"(none)":data_page_.selected_field(i));
    out<<"\r\nIf values are accepted but no line is visible: select a channel, inspect Snapshot, time window and fixed Y range. Binary/RTU-owned traffic bypasses the text pipeline. Counts are software observations, not wire-error measurements.";
    return out.str();
}
bool CMainDialog::start_lab_file(int page,int action,std::function<LabFileResult(const std::atomic<bool>&)> work) {
    if(closing_serial_||lab_file_.busy()||profile_job_.busy()){lab_.notice(page,L"Wait for current file/profile work, or Cancel file work.");return false;}
    if(!lab_file_.start(std::move(work))){lab_.notice(page,L"Cannot start file worker.");return false;}
    lab_file_page_=page;lab_file_action_=action;lab_.file_busy(true);lab_.notice(page,L"Working in background. Original files remain read-only; Cancel file work stops pending I/O.");return true;
}
void CMainDialog::poll_uart_lab() {
    if(workbench_)for(size_t i=0;i<2;++i)if(lab_tx_ids_[i]&&workbench_->runs_[i].id!=lab_tx_ids_[i]){lab_tx_ids_[i]=0;lab_.notice(UartLabWindow::Packet,workbench_->runs_[i].kind==UartWorkbench::ToolRun::None?workbench_->runs_[i].status:L"Packet operation finished; see Workbench for subsequent activity.");}
    LabFileResult result;std::wstring error;
    if(lab_file_.take(result,error,false)) {
        lab_.file_busy(false);
        if(!error.empty()){
            const auto prefix=lab_file_page_==UartLabWindow::Sessions&&lab_file_action_==3&&error.rfind(L"Readable logs requested:",0)==0?
                ui::tr(L"Some requested readable logs are missing beside the raw recordings. No manifest was created. Choose No for a raw/index-only session, or place reviewed copies beside the .bin files.")+L"\r\n":L"";
            lab_.notice(lab_file_page_,prefix+error);if(lab_file_page_==UartLabWindow::Waterfall){waterfall_running_=false;lab_.clear_waterfall();}}
        else {
            try {
                if(result.view)staged_view_=std::move(result.view);
                if(result.notes){workflow_notes_=std::move(result.notes);lab_.field(UartLabWindow::Notes,0,result.source.wstring());}
                if(result.spectrum&&waterfall_running_&&lab_.active(UartLabWindow::Waterfall))lab_.spectrum(*result.spectrum,waterfall_depth_,waterfall_low_,waterfall_high_);
                if(result.note_replay){if(replay_.data()!=result.note_replay||replay_io_.busy()||replay_load_.busy()||!advanced_idle())throw std::runtime_error("Replay changed during verification; note was not located.");replay_.pause(GetTickCount64());reset_offline_view();replay_.seek(result.note_record,GetTickCount64());}
                if(lab_file_page_==UartLabWindow::Parser&&lab_file_action_==2)lab_.field(UartLabWindow::Parser,7,uart_wide(result.payload));
                if(result.calculus){calculus_result_=std::move(result.calculus);lab_.calculus(calculus_result_);}
                if(result.transactions)transaction_result_=std::move(result.transactions);
                if(lab_file_page_==UartLabWindow::Sessions&&lab_file_action_==5){
                    staged_profile_=decode_profile(result.payload);has_staged_profile_=true;show_advanced_tools(AdvancedTools::Profiles);
                    advanced_tools_.state(AdvancedTools::Profiles,L"Verified session snapshot staged: "+result.source.filename().wstring()+L". Ports remain CLOSED on Apply; private history/commands remain unchanged.");
                    advanced_tools_.set_field(AdvancedTools::Replay,0,result.source.wstring());advanced_tools_.set_field(AdvancedTools::Replay,1,std::to_wstring(result.interval));
                    advanced_tools_.notice(AdvancedTools::Profiles,L"Review the Laboratory report. Apply staged explicitly while idle, then load/play the staged replay path separately. Verification reflects the files at check time.");
                }
                if(lab_file_page_==UartLabWindow::Packet&&lab_file_action_==2) {
                    PacketDefinition p;std::string detail;if(!decode_packet(result.payload,p,detail))throw std::runtime_error(detail);
                    const std::vector<std::wstring> checks{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"},ends{L"None",L"CR",L"LF",L"CRLF"},codecs{L"None",L"COBS",L"SLIP"};
                    lab_.field(UartLabWindow::Packet,1,p.command.hex?L"HEX":L"Text UTF-8");lab_.field(UartLabWindow::Packet,2,uart_wide(p.command.body));lab_.field(UartLabWindow::Packet,3,uart_wide(p.parameters));
                    lab_.field(UartLabWindow::Packet,4,ends[static_cast<int>(p.command.ending)]);lab_.field(UartLabWindow::Packet,5,checks[static_cast<int>(p.command.checksum)]);lab_.field(UartLabWindow::Packet,6,p.command.checksum_big?L"Big":L"Little");
                    lab_.field(UartLabWindow::Packet,7,std::to_wstring(p.sequence));lab_.field(UartLabWindow::Packet,8,codecs[static_cast<int>(p.codec)]);lab_.field(UartLabWindow::Packet,9,uart_wide(p.command.name));
                    lab_.field(UartLabWindow::Packet,10,p.custom_crc?uart_wide(encode_crc_spec(*p.custom_crc)):L"");lab_.field(UartLabWindow::Packet,11,std::to_wstring(p.checksum_from));lab_.field(UartLabWindow::Packet,12,std::to_wstring(p.checksum_count));
                }
                lab_.notice(lab_file_page_,report_text(result.report));
            }catch(const std::exception& e){lab_.notice(lab_file_page_,report_text(e.what()));}
        }
    }
    if(close_after_lab_&&!lab_file_.busy()){close_after_lab_=false;OnCancel();return;}
    poll_waterfall();
    if(GetTickCount64()-lab_tick_<1000)return;lab_tick_=GetTickCount64();
    if(lab_.active(UartLabWindow::Diagnostics))lab_.notice(UartLabWindow::Diagnostics,report_text(lab_diagnostics()));
    if(lab_.active(UartLabWindow::Quality))lab_.notice(UartLabWindow::Quality,uart_wide(lab_quality_report()));
    if(lab_.active(UartLabWindow::Digital))refresh_digital();
}
LRESULT CMainDialog::OnLabRequest(WPARAM,LPARAM parameter) {
    if(!parameter||closing_serial_)return 0;const auto r=*reinterpret_cast<const UartLabWindow::Request*>(parameter);
    if(r.page==-1){waterfall_running_=false;lab_file_.cancel();if(workbench_)for(size_t i=0;i<2;++i)if(lab_tx_ids_[i]&&workbench_->runs_[i].id==lab_tx_ids_[i]&&workbench_->runs_[i].kind==UartWorkbench::ToolRun::Command)workbench_->stop_tool(i);return 1;}
    if(r.page<0||r.page>=UartLabWindow::PageCount)return 0;
    if(lab_file_cancel(r.page,r.action)){if(lab_file_page_==UartLabWindow::Waterfall)waterfall_running_=false;lab_file_.cancel();return 1;}
    if(lab_file_.busy()&&!((r.page==UartLabWindow::Recording&&r.action==5)||(r.page==UartLabWindow::Sessions&&r.action==7)||(r.page==UartLabWindow::Waterfall&&r.action==1))){lab_.notice(r.page,L"Wait for file work or cancel it first.");return 0;}
    try {
        switch(r.page) {
        case UartLabWindow::Diagnostics:
            if(r.action==1)diagnostics_.reset();if(r.action==2&&workbench_)workbench_->text_b_.reset_diagnostics();
            if(r.action==3)show_advanced_tools(AdvancedTools::Extraction);
            if(r.action==4){show_workbench();if(workbench_)workbench_->text_dialog();}
            if(r.action==5)lab_.select(UartLabWindow::Schema);
            lab_.notice(r.page,report_text(lab_diagnostics()));break;
        case UartLabWindow::Recording:lab_recording_request(r);break;
        case UartLabWindow::Packet:lab_packet_request(r);break;
        case UartLabWindow::Parser:lab_parser_request(r);break;
        case UartLabWindow::Quality:lab_quality_request(r);break;
        case UartLabWindow::Digital:lab_digital_request(r);break;
        case UartLabWindow::Sessions:lab_session_request(r);break;
        case UartLabWindow::Devices:lab_devices_request(r);break;
        case UartLabWindow::Input:lab_input_request(r);break;
        case UartLabWindow::Checksums:lab_crc_request(r);break;
        case UartLabWindow::Calculus:lab_calculus_request(r);break;
        case UartLabWindow::Transactions:lab_transactions_request(r);break;
        case UartLabWindow::Views:lab_views_request(r);break;
        case UartLabWindow::Images:lab_images_request(r);break;
        case UartLabWindow::Notes:lab_notes_request(r);break;
        case UartLabWindow::Reports:lab_reports_request(r);break;
        case UartLabWindow::Waterfall:lab_waterfall_request(r);break;
        case UartLabWindow::Schema:
            if(r.fields.size()!=1)return 0;
            if(profile_job_.busy())throw std::runtime_error("Wait for profile work.");
            if(r.fields[0]==L"Offline replay")binary_replay_options();
            else {show_workbench();if(workbench_&&workbench_->GetSafeHwnd())workbench_->binary_dialog(r.fields[0]==L"Port B"?1:0);}
            break;
        }
    }catch(const std::exception& e){lab_.notice(r.page,report_text(e.what()));}
    return 1;
}
