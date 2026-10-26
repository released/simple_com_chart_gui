#include "mfc_main_dialog.h"
#include "packet_builder.h"
#include "uart_workbench.h"
#include <cmath>
namespace {
PacketDefinition from_fields(const std::vector<std::wstring>& f) {
    if(f.size()!=13)throw std::runtime_error("Invalid packet fields.");PacketDefinition p;
    p.command.hex=f[1]==L"HEX";p.command.body=message_utf8(f[2]);p.parameters=message_utf8(f[3]);p.command.name=message_utf8(f[9]);
    const std::vector<std::wstring> endings{L"None",L"CR",L"LF",L"CRLF"},checks{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"},codecs{L"None",L"COBS",L"SLIP"};
    p.command.ending=static_cast<TxEnding>(std::find(endings.begin(),endings.end(),f[4])-endings.begin());p.command.checksum=static_cast<FrameChecksum>(std::find(checks.begin(),checks.end(),f[5])-checks.begin());p.command.checksum_big=f[6]==L"Big";
    double sequence=0;if(!uart_number(f[7],0,4294967295.0,sequence)||sequence!=std::floor(sequence))throw std::runtime_error("Sequence must be an unsigned 32-bit integer.");
    p.sequence=static_cast<uint32_t>(sequence);p.codec=static_cast<WireCodec>(std::find(codecs.begin(),codecs.end(),f[8])-codecs.begin());
    if(!f[10].empty()){p.custom_crc=parse_crc_spec(message_utf8(f[10]));double from=0,count=0;if(!uart_number(f[11],0,65536,from)||!uart_number(f[12],0,65536,count)||from!=std::floor(from)||count!=std::floor(count))throw std::runtime_error("Custom CRC range must use whole bytes 0..65536.");p.checksum_from=static_cast<size_t>(from);p.checksum_count=static_cast<size_t>(count);}return p;
}
}
void CMainDialog::lab_packet_request(const UartLabWindow::Request& r) {
    if(r.action==4){
        if(r.fields.size()!=13)return;WorkflowGrid::Rows rows;
        for(const auto& row:packet_parameters(message_utf8(r.fields[3]))){std::vector<std::wstring> fields;for(const auto& cell:row)fields.push_back(uart_wide(cell));rows.push_back(std::move(fields));}
        const auto validate=[](const WorkflowGrid::Rows& input){std::vector<PacketParameterRow> rows;for(const auto& r:input){if(r.size()!=4)throw std::runtime_error("Four parameter columns required.");rows.push_back({message_utf8(r[0]),message_utf8(r[1]),message_utf8(r[2]),message_utf8(r[3])});}return encode_packet_parameters(rows);};
        WorkflowGrid grid(L"Packet parameters",{L"Name",L"Value",L"Minimum",L"Maximum"},std::move(rows),[validate](const auto& input){validate(input);},&lab_);
        if(workflow_grid_present_(grid)==IDOK&&::IsWindow(lab_.m_hWnd))lab_.field(r.page,3,uart_wide(validate(grid.rows())));return;
    }
    if(r.action==2||r.action==3) {
        const bool load=r.action==2;CFileDialog pick(load,L"sccpacket",nullptr,OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(load?OFN_FILEMUSTEXIST:0),L"Private packet definition (*.sccpacket)|*.sccpacket||",&lab_);
        if(pick.DoModal()!=IDOK||!::IsWindow(lab_.m_hWnd))return;const auto path=std::filesystem::path(pick.GetPathName().GetString());
        if(load)start_lab_file(r.page,r.action,[path](const std::atomic<bool>& cancel){auto bytes=uart_read_file(path,65536,cancel);PacketDefinition p;std::string error;if(!decode_packet(bytes,p,error))throw std::runtime_error(error);return LabFileResult{"Definition loaded; no port opened or bytes sent.",std::move(bytes)};});
        else {const auto bytes=encode_packet(from_fields(r.fields));start_lab_file(r.page,r.action,[path,bytes](const std::atomic<bool>& cancel){uart_write_file(path,bytes,false,cancel);return LabFileResult{"Private definition saved; existing files are never overwritten.",{}};});}return;
    }
    const auto definition=from_fields(r.fields);std::string bytes,error;if(!packet_payload(definition,bytes,error))throw std::runtime_error(error);
    const auto preview=std::to_string(bytes.size())+" wire bytes\r\nHEX: "+uart_hex(bytes);
    if(r.action==0){lab_.notice(r.page,uart_wide(preview+"\r\nPreview only; no UART action."));return;}
    if(profile_job_.busy()||replay_.data()||closing_serial_)throw std::runtime_error("Finish profile/offline/closing work first.");
    if(!workbench_||!workbench_->GetSafeHwnd())show_workbench();
    if(!workbench_)throw std::runtime_error("Workbench is unavailable.");
    poll_workbench();const size_t port=r.fields[0]==L"Port B"?1:0;
    const auto generation=port?workbench_->secondary_.generation():uart_operations_.generation();
    const auto target=port?workbench_->secondary_.settings().port.device:connection_.active().port.device;
    const bool connected=port?workbench_->secondary_.connected():serial_mgr_->is_connected();
    if(!connected||workbench_->tx_reserved(port))throw std::runtime_error("Connect the selected port and finish its current TX/tool before Send once.");
    const auto question=L"Send this immutable packet ONCE to "+target+L"?\r\n\r\n"+uart_wide(preview)+L"\r\nDriver acceptance is not target acknowledgement. No automatic retry.";
    if(uart_notice_(lab_.m_hWnd,question.c_str(),L"Confirm packet TX",MB_OKCANCEL|MB_ICONWARNING)!=IDOK)return;
    if(closing_serial_||!::IsWindow(lab_.m_hWnd)||(port?workbench_->secondary_.generation():uart_operations_.generation())!=generation||
        (port?workbench_->secondary_.settings().port.device:connection_.active().port.device)!=target)throw std::runtime_error("Target connection changed; packet was not sent.");
    std::wstring detail;if(!workbench_->command_send(port,std::move(bytes),detail))throw std::runtime_error(message_utf8(detail));
    lab_tx_ids_[port]=workbench_->runs_[port].id;
    lab_.notice(r.page,L"Packet queued once for "+target+L". See Workbench command status; no automatic retry or sequence increment.");
}
