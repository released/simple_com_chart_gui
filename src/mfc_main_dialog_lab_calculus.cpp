#include "mfc_main_dialog.h"
#include "capture_filename.h"
void CMainDialog::lab_calculus_request(const UartLabWindow::Request& r){
    if(r.fields.size()!=4)return;
    if(r.action==1){if(!calculus_result_)throw std::runtime_error("Calculate a retained snapshot first.");SYSTEMTIME now{};GetLocalTime(&now);auto name=capture_filename(now);name.replace(name.size()-4,4,L"_calculus.csv");CFileDialog pick(FALSE,L"csv",name.c_str(),OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST,L"New CSV (*.csv)|*.csv||",&lab_);if(pick.DoModal()!=IDOK||!::IsWindow(lab_.m_hWnd))return;const auto path=std::filesystem::path(pick.GetPathName().GetString());const auto csv=calculus_result_->csv;start_lab_file(r.page,r.action,[path,csv](const std::atomic<bool>& cancel){uart_write_file(path,csv,false,cancel);return LabFileResult{"Calculus CSV saved; original capture is unchanged.",{}};});return;}
    double gap=0;if(!uart_number(r.fields[3],.000001,86400,gap))throw std::runtime_error("Maximum sample gap must be positive and <=86400 seconds.");const auto data=capture_.snapshot();const auto key=message_utf8(r.fields[1]);const unsigned source=r.fields[0]==L"Port B"?1:0;const bool integral=r.fields[2]==L"Integral";
    start_lab_file(r.page,r.action,[data,key,source,integral,gap](const std::atomic<bool>& cancel){LabFileResult result;result.calculus=std::make_shared<CalculusResult>(time_calculus(data,key,source,integral,gap,cancel));result.report=result.calculus->report;return result;});
}
