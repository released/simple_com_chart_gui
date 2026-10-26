#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "settings_fields.h"
#include <cmath>
#include <sstream>

void CMainDialog::load_lab_preferences() {
    for(size_t port=0;port<2;++port) {
        std::vector<wchar_t> buffer(16384);const auto n=GetPrivateProfileStringW(port?L"Lab.Sequence.B":L"Lab.Sequence.A",L"Configuration",L"",buffer.data(),static_cast<DWORD>(buffer.size()),settings_path_.c_str());
        try {
            if(n>=buffer.size()-1)throw std::runtime_error("Sequence settings truncated.");
            std::string key;uint64_t modulus=65536,step=1;
            if(n){const auto f=unpack_settings(buffer.data(),3);double m=0,s=0;key=message_utf8(f[0]);if(key.size()>160||!uart_number(f[1],2,4294967296.0,m)||m!=std::floor(m)||!uart_number(f[2],1,m-1,s)||s!=std::floor(s))throw std::runtime_error("Invalid sequence settings.");modulus=static_cast<uint64_t>(m);step=static_cast<uint64_t>(s);}
            if(key!=sequence_keys_[port]||modulus!=sequence_modulus_[port]||step!=sequence_step_[port]){std::string error;sequences_[port].configure(modulus,step,error);sequence_keys_[port]=key;sequence_modulus_[port]=modulus;sequence_step_[port]=step;}
        }catch(...){show_status_message(L"Invalid laboratory sequence preferences; previous settings retained.",6000);}
    }
    try {
        std::vector<wchar_t> buffer(16384);const auto n=GetPrivateProfileStringW(L"Lab.Digital",L"Configuration",L"",buffer.data(),static_cast<DWORD>(buffer.size()),settings_path_.c_str());
        if(n>=buffer.size()-1)throw std::runtime_error("Digital settings truncated.");
        std::wstring definitions;double window=30,hold=2;std::vector<DigitalLane> lanes;std::string error;
        if(n){const auto f=unpack_settings(buffer.data(),3);definitions=f[0];if(!parse_digital_lanes(message_utf8(definitions),lanes,error)||!uart_number(f[1],.1,86400,window)||!uart_number(f[2],.001,86400,hold))throw std::runtime_error("Invalid digital history settings.");}
        digital_lanes_=std::move(lanes);digital_window_=window;digital_hold_=hold;
        if(lab_.GetSafeHwnd()){lab_.field(UartLabWindow::Digital,0,definitions);lab_.field(UartLabWindow::Digital,1,std::to_wstring(window));lab_.field(UartLabWindow::Digital,2,std::to_wstring(hold));}
    }catch(...){show_status_message(L"Invalid laboratory digital preferences; previous settings retained.",6000);}
    if(lab_.GetSafeHwnd()){const auto f=lab_.values(UartLabWindow::Quality);const size_t port=f[0]==L"Port B"?1:0;lab_.field(UartLabWindow::Quality,1,uart_wide(sequence_keys_[port]));lab_.field(UartLabWindow::Quality,2,std::to_wstring(sequence_modulus_[port]));lab_.field(UartLabWindow::Quality,3,std::to_wstring(sequence_step_[port]));}
}
std::string CMainDialog::lab_quality_report(bool csv)const {
    std::ostringstream out;
    if(csv)out<<"Port,Metric,Value,Unit\r\n";
    for(size_t i=0;i<2;++i) {
        const auto& s=sequences_[i].stats();LatencyStats latency;size_t timeouts=0;
        if(workbench_){latency=latency_stats(workbench_->runs_[i].script.matched_timings());timeouts=workbench_->runs_[i].script.timeouts();}
        if(csv) {
            for(const auto& item:std::vector<std::pair<std::string,double>>{{"sequence_samples",static_cast<double>(s.samples)},{"observed_duplicates",static_cast<double>(s.duplicates)},{"observed_jumps",static_cast<double>(s.jumps)},{"backwards_or_reset",static_cast<double>(s.backwards)},{"wraps",static_cast<double>(s.wraps)},{"generation_baselines",static_cast<double>(s.resets)},{"invalid_values",static_cast<double>(s.invalid)},{"matched_steps",static_cast<double>(latency.count)},{"timeouts",static_cast<double>(timeouts)}})out<<(i?'B':'A')<<','<<item.first<<','<<item.second<<",count\r\n";
            for(const auto& item:std::vector<std::pair<std::string,double>>{{"min",latency.minimum},{"mean",latency.mean},{"max",latency.maximum},{"p50",latency.p50},{"p95",latency.p95},{"p99",latency.p99}})if(latency.count)out<<(i?'B':'A')<<",PC_step_"<<item.first<<','<<item.second<<",ms\r\n";
        } else {
            out<<"PORT "<<(i?'B':'A')<<" sequence key: "<<(sequence_keys_[i].empty()?"(none)":sequence_keys_[i])<<"; modulus="<<sequence_modulus_[i]<<"; step="<<sequence_step_[i]
                <<"\r\nSamples="<<s.samples<<", duplicates="<<s.duplicates<<", jumps="<<s.jumps<<", backwards/reset="<<s.backwards<<", wraps="<<s.wraps<<", generation baselines="<<s.resets<<", invalid="<<s.invalid
                <<"\r\nMost recent script: matched successful steps="<<latency.count<<", timeouts="<<timeouts;
            if(latency.count)out<<"; PC step ms min/mean/max="<<latency.minimum<<'/'<<latency.mean<<'/'<<latency.maximum<<", p50/p95/p99="<<latency.p50<<'/'<<latency.p95<<'/'<<latency.p99;
            else out<<"; response timing n/a (driver-only steps excluded)";out<<"\r\n\r\n";
        }
    }
    if(!csv)out<<"Nearest-rank percentiles of the most recent finite script, not a long-term/device/wire RTT measurement. Sequence jumps do not identify where data was lost; filtering and device resets may also cause discontinuity.";return out.str();
}
void CMainDialog::lab_quality_request(const UartLabWindow::Request& r) {
    if(r.fields.size()!=4)return;const size_t port=r.fields[0]==L"Port B"?1:0;
    if(r.action==4){lab_.field(r.page,1,uart_wide(sequence_keys_[port]));lab_.field(r.page,2,std::to_wstring(sequence_modulus_[port]));lab_.field(r.page,3,std::to_wstring(sequence_step_[port]));return;}
    if(r.action==0) {
        if(profile_job_.busy())throw std::runtime_error("Wait for profile work.");double modulus=0,step=0;
        const auto key=message_utf8(r.fields[1]);if(key.size()>160||key.find_first_of("\r\n\0",0,3)!=key.npos||!uart_number(r.fields[2],2,4294967296.0,modulus)||modulus!=std::floor(modulus)||!uart_number(r.fields[3],1,modulus-1,step)||step!=std::floor(step))throw std::runtime_error("Enter exact key <=160 bytes, whole modulus 2..4294967296 and step 1..modulus-1.");
        const auto encoded=pack_settings({r.fields[1],r.fields[2],r.fields[3]});if(!WritePrivateProfileStringW(port?L"Lab.Sequence.B":L"Lab.Sequence.A",L"Configuration",encoded.c_str(),settings_path_.c_str()))throw std::runtime_error("Cannot save sequence settings; previous settings retained.");
        std::string error;sequences_[port].configure(static_cast<uint64_t>(modulus),static_cast<uint64_t>(step),error);sequence_keys_[port]=key;sequence_modulus_[port]=static_cast<uint64_t>(modulus);sequence_step_[port]=static_cast<uint64_t>(step);
    }else if(r.action==1)sequences_[port].reset();
    else if(r.action==3){CFileDialog pick(FALSE,L"csv",nullptr,OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST,L"Statistics (*.csv)|*.csv||",&lab_);if(pick.DoModal()!=IDOK||!::IsWindow(lab_.m_hWnd))return;const auto path=std::filesystem::path(pick.GetPathName().GetString());const auto report=lab_quality_report(true);start_lab_file(r.page,r.action,[path,report](const std::atomic<bool>& cancel){uart_write_file(path,report,false,cancel);return LabFileResult{"Statistics saved; existing files were not overwritten.",{}};});return;}
    lab_.notice(r.page,uart_wide(lab_quality_report()));
}
void CMainDialog::lab_digital_request(const UartLabWindow::Request& r) {
    if(r.fields.size()!=3)return;
    if(r.action!=1){if(profile_job_.busy())throw std::runtime_error("Wait for profile work.");const auto definitions=r.action==2?std::wstring{}:r.fields[0];std::vector<DigitalLane> lanes;std::string error;double window=0,hold=0;
        if(!parse_digital_lanes(message_utf8(definitions),lanes,error))throw std::runtime_error(error);
        if(!uart_number(r.fields[1],.1,86400,window)||!uart_number(r.fields[2],.001,86400,hold))throw std::runtime_error("Window .1..86400 s; maximum hold .001..86400 s.");
        const auto encoded=pack_settings({definitions,r.fields[1],r.fields[2]});if(!WritePrivateProfileStringW(L"Lab.Digital",L"Configuration",encoded.c_str(),settings_path_.c_str()))throw std::runtime_error("Cannot save lanes; previous preferences retained.");
        digital_lanes_=std::move(lanes);digital_window_=window;digital_hold_=hold;if(r.action==2)lab_.field(r.page,0,L"");}
    refresh_digital();
}
void CMainDialog::refresh_digital(){if(!lab_.GetSafeHwnd())return;lab_.digital(capture_.snapshot(),digital_lanes_,digital_window_,digital_hold_);lab_.notice(UartLabWindow::Digital,L"Retained capture only; " + std::to_wstring(digital_lanes_.size())+L" lane(s). No waveform selected automatically; missing states remain blank.");}
