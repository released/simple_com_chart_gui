#include "mfc_main_dialog.h"
#include <cmath>
void CMainDialog::lab_waterfall_request(const UartLabWindow::Request& r){
    if(r.action==1){waterfall_running_=false;if(lab_file_page_==UartLabWindow::Waterfall)lab_file_.cancel();lab_.notice(r.page,ui::tr(L"Waterfall stopped; retained history preserved."));return;}
    if(r.action==2){waterfall_running_=false;lab_.clear_waterfall();lab_.notice(r.page,ui::tr(L"Waterfall history cleared."));return;}
    if(r.fields.size()!=6)return;double samples=0,rate=0,depth=0,low=0,high=0;
    if(!uart_number(r.fields[1],32,4096,samples)||std::floor(samples)!=samples||(static_cast<size_t>(samples)&(static_cast<size_t>(samples)-1))||!uart_number(r.fields[2],0,1e9,rate)||!uart_number(r.fields[3],1,256,depth)||std::floor(depth)!=depth||!uart_number(r.fields[4],-200,200,low)||!uart_number(r.fields[5],-200,200,high)||low>=high)throw std::runtime_error("FFT: power of two 32..4096; depth 1..256; increasing dB bounds -200..200.");
    const auto key=message_utf8(r.fields[0]);if(key.empty()||key.size()>160)throw std::runtime_error("Select an exact numeric key.");
    waterfall_key_=key;waterfall_points_=static_cast<size_t>(samples);waterfall_depth_=static_cast<size_t>(depth);waterfall_rate_=rate;waterfall_low_=low;waterfall_high_=high;waterfall_tick_=0;waterfall_running_=true;lab_.clear_waterfall();poll_waterfall();
}
void CMainDialog::poll_waterfall(){
    if(!waterfall_running_)return;if(!lab_.active(UartLabWindow::Waterfall)||closing_serial_){waterfall_running_=false;if(lab_file_page_==UartLabWindow::Waterfall)lab_file_.cancel();return;}
    if(lab_file_.busy()||profile_job_.busy()||GetTickCount64()-waterfall_tick_<500)return;waterfall_tick_=GetTickCount64();
    const auto snapshot=capture_.snapshot();const auto found=std::find(snapshot.keys.begin(),snapshot.keys.end(),waterfall_key_);if(found==snapshot.keys.end()){lab_.clear_waterfall();lab_.notice(UartLabWindow::Waterfall,ui::tr(L"Waiting for selected field samples."));return;}
    const auto column=static_cast<size_t>(found-snapshot.keys.begin());std::vector<std::pair<double,double>> samples;uint64_t generation=UINT64_MAX;unsigned source=UINT_MAX;double at=0;
    for(auto i=snapshot.rows.rbegin();i!=snapshot.rows.rend()&&samples.size()<waterfall_points_;++i){double value=0;const auto& row=**i;if(!CaptureStore::value(row,column,value)){if(source!=UINT_MAX&&row.numeric_source==source)break;continue;}if(generation==UINT64_MAX){generation=row.generation;source=row.numeric_source;at=row.source_seconds;}if(row.generation!=generation||row.numeric_source!=source)break;samples.emplace_back(row.source_seconds,value);}
    if(samples.size()<waterfall_points_){lab_.clear_waterfall();lab_.notice(UartLabWindow::Waterfall,ui::tr(L"Waiting for enough contiguous samples; gaps do not interpolate."));return;}
    const auto& history=lab_.waterfall_.history();if(!history.rows().empty()&&history.newest()==at&&history.generation()==generation)return;
    std::reverse(samples.begin(),samples.end());const auto points=waterfall_points_;const auto rate=waterfall_rate_;
    start_lab_file(UartLabWindow::Waterfall,0,[samples=std::move(samples),points,rate,generation,at](const std::atomic<bool>& cancel){if(cancel)throw std::runtime_error("Cancelled.");auto frame=std::make_shared<WaterfallFrame>();std::string error;if(!signal_fft(samples,points,rate,frame->spectrum,error))throw std::runtime_error(error);if(cancel)throw std::runtime_error("Cancelled.");frame->generation=generation;frame->seconds=at;LabFileResult result;result.spectrum=frame;result.report="Hann / mean removed | "+std::to_string(points)+" points | fs "+std::to_string(frame->spectrum.sample_rate)+" Hz | "+frame->spectrum.provenance+". Newest row at bottom; equal timestamps add no duplicate history.";return result;});
}
