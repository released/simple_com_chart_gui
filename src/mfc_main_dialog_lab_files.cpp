#include "mfc_main_dialog.h"
#include "recording_tools.h"
#include "capture_filename.h"
#include <cmath>

void CMainDialog::lab_recording_request(const UartLabWindow::Request& r) {
    if(r.action==5){lab_file_.cancel();return;}if(r.fields.size()!=6)return;
    if(r.action==0||r.action==4) {
        SYSTEMTIME now{};GetLocalTime(&now);auto name=capture_filename(now);name.resize(name.size()-4);name+=L".bin";
        CFileDialog pick(r.action==0,L"bin",r.action==4?name.c_str():nullptr,OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(r.action==0?OFN_FILEMUSTEXIST:0),L"Raw recording (*.bin)|*.bin|All files (*.*)|*.*||",&lab_);
        if(pick.DoModal()==IDOK&&::IsWindow(lab_.m_hWnd))lab_.field(r.page,r.action==0?0:5,pick.GetPathName().GetString());return;
    }
    const auto source=std::filesystem::path(r.fields[0]),output=std::filesystem::path(r.fields[5]);
    if(r.action==1||r.action==3) {
        start_lab_file(r.page,r.action,[source,output,salvage=r.action==3](const std::atomic<bool>& cancel){auto health=inspect_recording(source,cancel);LabFileResult result;result.report=health.report();
            if(salvage){if(!health.prefix||health.prefix->records.empty())throw std::runtime_error("No valid indexed RX prefix to salvage; original files unchanged.");result.report+="\r\n"+export_recording_range(*health.prefix,0,health.prefix->records.size(),output,cancel);}return result;});return;
    }
    double ms=0,first=0,end=0;
    if(!uart_number(r.fields[1],1,60000,ms)||ms!=std::floor(ms)||!uart_number(r.fields[3],0,1e15,first)||(!r.fields[4].empty()&&!uart_number(r.fields[4],0,1e15,end)))throw std::runtime_error("Enter valid interval and nonnegative range bounds.");
    const bool time=r.fields[2]==L"Seconds",eof=r.fields[4].empty();
    if(!time&&(first!=std::floor(first)||(!eof&&end!=std::floor(end))||first>50000000||(!eof&&end>50000000)))throw std::runtime_error("Record bounds must be whole numbers 0..50000000.");
    start_lab_file(r.page,r.action,[source,output,ms,first,end,time,eof](const std::atomic<bool>& cancel){auto data=load_replay(source,static_cast<unsigned>(ms),cancel);
        const size_t begin=time?replay_seek_time(*data,first):static_cast<size_t>(first),past=eof?data->records.size():time?replay_seek_time(*data,end):static_cast<size_t>(end);
        return LabFileResult{export_recording_range(*data,begin,past,output,cancel),{}};});
}
