#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
#include "uart_files.h"
#include "discovery_list.h"
#include "error_message.h"
#include <cmath>

namespace {
unsigned integer(const std::wstring& text,unsigned maximum) {
    if(text.empty()||text.size()>9||text.find_first_not_of(L"0123456789")!=text.npos)throw std::runtime_error("Enter an unsigned decimal integer.");
    const auto value=std::stoul(text);if(value>maximum)throw std::runtime_error("Integer exceeds its allowed range.");return static_cast<unsigned>(value);
}
bool point_from_fields(const std::vector<std::wstring>& f,ModbusPoint& point,std::wstring& error) {
    try{if(f.size()!=13)throw std::runtime_error("Invalid Modbus field count.");
        point.name=message_utf8(f[0]);point.request.unit=integer(f[1],247);point.request.function=integer(f[2].substr(0,2),4);
        std::string detail;unsigned address=0;if(!modbus_address(point.request.function,message_utf8(f[4]),f[3]==L"Reference address",address,detail))throw std::runtime_error(detail);
        if(f[3]!=L"Reference address"&&f[3]!=L"Wire offset")throw std::runtime_error("Unsupported address notation.");point.request.address=address;
        point.request.count=integer(f[5],2000);point.type=message_utf8(f[6]);point.order=message_utf8(f[7]);
        if(!uart_number(f[8],-1e100,1e100,point.scale)||!uart_number(f[9],-1e100,1e100,point.offset))throw std::runtime_error("Enter finite scale and offset.");
        point.unit=message_utf8(f[10]);point.request.period_ms=integer(f[11],60000);point.request.timeout_ms=integer(f[12],60000);
        if(!validate_modbus_points({point},detail))throw std::runtime_error(detail);return true;
    }catch(const std::exception& e){error=ui::tr(uart_wide(e.what()));return false;}
}
}
void CMainDialog::edit_modbus_point(bool add) {
    if(!workbench_)return;const auto port=modbus_page_.target();auto& points=modbus_points_[port];const int selected=modbus_page_.selected();
    if(workbench_->rtu_[port].active()||protocol_file_.busy()||exporting_)return;
    if(add&&points.size()>=32){modbus_page_.status(ui::tr(L"Define 1..32 Modbus read blocks."));return;}
    if(!add&&(selected<0||static_cast<size_t>(selected)>=points.size())){modbus_page_.status(ui::tr(L"Select a read block first."));return;}
    ModbusPoint point;if(add)point.name="field"+std::to_string(points.size()+1);else point=points[static_cast<size_t>(selected)];
    const auto function=point.request.function==1?L"01 Coils":point.request.function==2?L"02 Discrete":point.request.function==4?L"04 Input":L"03 Holding";
    const auto existing=points;const auto validate=[existing,selected,add](const std::vector<std::wstring>& f,std::wstring& error){ModbusPoint candidate;if(!point_from_fields(f,candidate,error))return false;auto all=existing;if(add)all.push_back(candidate);else all[static_cast<size_t>(selected)]=candidate;std::string detail;if(!validate_modbus_points(all,detail)){error=ui::tr(uart_wide(detail));return false;}return true;};
    MonitorModal modal(discovery_modal_);
    UartToolForm form(add?L"Add Modbus read block":L"Edit Modbus read block",
        L"Read definitions only. Wire offset starts at zero; references use 00001 / 10001 / 30001 / 40001 by function. Typed counts: bit/u16/i16=1, u32/i32/f32=2, f64=4. raw reads a range. Preview and Save do not transmit.",
        {{L"Name",uart_wide(point.name)},{L"Unit ID",std::to_wstring(point.request.unit)},
         {L"Function",function,{L"01 Coils",L"02 Discrete",L"03 Holding",L"04 Input"}},
         {L"Address notation",L"Wire offset",{L"Wire offset",L"Reference address"}},{L"Address",std::to_wstring(point.request.address)},
         {L"Count",std::to_wstring(point.request.count)},{L"Type",uart_wide(point.type),{L"raw",L"bit",L"u16",L"i16",L"u32",L"i32",L"f32",L"f64"}},
         {L"Word / byte order",uart_wide(point.order),{L"ABCD",L"CDAB",L"BADC",L"DCBA"}},
         {L"Scale",std::to_wstring(point.scale)},{L"Offset",std::to_wstring(point.offset)},{L"Unit",uart_wide(point.unit)},
         {L"Poll interval (ms)",std::to_wstring(point.request.period_ms)},{L"Timeout (ms)",std::to_wstring(point.request.timeout_ms)}},
        [](const auto& f){ModbusPoint p;std::wstring error;if(!point_from_fields(f,p,error))return error;
            return ui::tr(L"Wire offset: ")+std::to_wstring(p.request.address)+ui::tr(L" | Reference: ")+std::to_wstring(modbus_reference(p.request.function,p.request.address))+L"\r\n"+ui::tr(L"Request HEX (preview only): ")+uart_wide(uart_hex(rtu_request(modbus_plan({p})[0])));},validate,this);
    form.accept_caption(L"Save definition");
    if(form.DoModal()!=IDOK||workbench_->rtu_[port].active()||protocol_file_.busy()||exporting_)return;
    std::wstring error;if(!validate(form.values(),error)||!point_from_fields(form.values(),point,error)){modbus_page_.status(error);return;}
    if(add)points.push_back(std::move(point));else points[static_cast<size_t>(selected)]=std::move(point);
    modbus_values_[port].clear();modbus_page_.set_rows({},false);refresh_modbus_page();modbus_page_.status(ui::tr(L"Definitions staged. Start polling is a separate explicit action."));
}
void CMainDialog::protocol_action(bool lin,unsigned action) {
    try{
    if(lin){protocol_lin_action(action);return;}
    if(!workbench_||closing_serial_)return;const auto port=modbus_page_.target();
    if(action==8){if(workbench_->runs_[port].kind==UartWorkbench::ToolRun::Modbus)workbench_->stop_tool(port);refresh_modbus_page();return;}
    if(action==50){modbus_page_.set_rows({},false);refresh_modbus_page();return;}
    if(protocol_file_.busy()||exporting_||profile_job_.busy()){modbus_page_.status(ui::tr(L"Finish the current protocol file or export operation first."));return;}
    modbus_page_.status(L"");
    if(action==1){show_workbench();return;}
    if(action==9){MonitorModal modal(discovery_modal_);DiscoveryListDialog::Rows rows;const auto now=GetTickCount64();
        for(const auto& r:workbench_->rtu_[port].transactions())rows.push_back({std::to_wstring(r.sequence),std::to_wstring(r.unit),std::to_wstring(r.function),std::to_wstring(r.address),ui::tr(uart_wide(rtu_outcome_name(r.outcome))),std::to_wstring(r.exception),std::to_wstring((r.outcome==RtuOutcome::Pending?now:r.finished_ms)-r.started_ms),uart_wide(uart_hex(r.request)),uart_wide(uart_hex(r.response))});
        DiscoveryListDialog dialog(this,L"Modbus transactions",{L"Sequence",L"Unit ID",L"Function",L"Wire offset",L"Outcome",L"Exception",L"Observed ms",L"Request HEX",L"Response HEX"},std::move(rows),ui::tr(L"Latest 128 transactions. Times are observed by the PC, not wire measurements. Driver acceptance is not a device response."),L"Close");dialog.DoModal();return;
    }
    if(workbench_->rtu_[port].active()){modbus_page_.status(ui::tr(L"Stop polling before editing definitions."));return;}
    if(action==2||action==3){edit_modbus_point(action==2);return;}
    if(action==4){const auto selected=modbus_page_.selected();auto& points=modbus_points_[port];if(selected>=0&&static_cast<size_t>(selected)<points.size()){points.erase(points.begin()+selected);modbus_values_[port].clear();modbus_page_.set_rows({},false);refresh_modbus_page();}return;}
    if(action==5){MonitorModal modal(discovery_modal_);CFileDialog dialog(TRUE,L"sccmodbus",nullptr,OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR,ui::caption(L"Modbus definitions (*.sccmodbus)|*.sccmodbus||"),this);if(dialog.DoModal()==IDOK)begin_protocol_file(std::filesystem::path(dialog.GetPathName().GetString()),0,port);return;}
    if(action==6){save_protocol_file(false);return;}
    if(action==7){std::string detail;if(!validate_modbus_points(modbus_points_[port],detail)){modbus_page_.status(ui::tr(uart_wide(detail)));return;}poll_workbench();
        std::wstring error;if(!workbench_->start_rtu_plan(port,modbus_plan(modbus_points_[port]),error)){modbus_page_.status(ui::tr(error));return;}
        reset_numeric_capture();modbus_values_[port].clear();refresh_modbus_page();modbus_page_.status(ui::tr(L"Polling started. Raw recording remains independent."));
    }
    }catch(const std::exception& e){(lin?lin_page_:modbus_page_).status(ui::tr(error_message(e.what())));}
    catch(...){(lin?lin_page_:modbus_page_).status(ui::tr(L"Protocol operation failed; current data retained."));}
}
