#include "mfc_main_dialog.h"
#include "ui_localized_controls.h"
#include "uart_workbench.h"
#include "capture_filename.h"
#include "recording_settings.h"
#include "session_metadata.h"
#include "build_version.h"

namespace {
class RecordingDialog : public CDialog {
public:
    RecordingDialog(CWnd* parent, RecordingOptions o) : CDialog(IDD_RECORD_OPTIONS, parent), mib_(static_cast<unsigned>(o.rotate_bytes/1048576)), daily_(o.rotate_daily), limits_(o.limits) {}
    unsigned mib_; bool daily_; RecordingLimits limits_;
    BOOL OnInitDialog() override {
        CDialog::OnInitDialog();ui::localize_dialog(m_hWnd); SetDlgItemInt(IDC_RECORD_MIB, mib_, FALSE);
        CheckDlgButton(IDC_RECORD_DAILY, daily_ ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(IDC_RECORD_TOTAL,static_cast<UINT>(limits_.bytes/1048576));SetDlgItemInt(IDC_RECORD_SECONDS,static_cast<UINT>(limits_.seconds));SetDlgItemInt(IDC_RECORD_FREE,static_cast<UINT>(limits_.free_bytes/1048576));return TRUE;
    }
    void OnOK() override {
        BOOL valid = FALSE; const auto mib = GetDlgItemInt(IDC_RECORD_MIB, &valid, FALSE);
        if (!valid || mib < 1 || mib > 1024) { AfxMessageBox(L"Choose a rotation size from 1 to 1024 MiB.", MB_ICONWARNING); return; }
        RecordingLimits limits; const int ids[]={IDC_RECORD_TOTAL,IDC_RECORD_SECONDS,IDC_RECORD_FREE};uint64_t* fields[]={&limits.bytes,&limits.seconds,&limits.free_bytes};
        for(int i=0;i<3;++i){const auto value=GetDlgItemInt(ids[i],&valid,FALSE);if(!valid||value>static_cast<UINT>(i==1?604800:1048576)){AfxMessageBox(L"Limits: 0..1048576 MiB / 0..604800 seconds.",MB_ICONWARNING);return;}*fields[i]=static_cast<uint64_t>(value)*(i==1?1:1048576);}
        limits_=limits;mib_ = mib; daily_ = IsDlgButtonChecked(IDC_RECORD_DAILY) == BST_CHECKED; CDialog::OnOK();
    }
};
}

void CMainDialog::start_recording(unsigned source) {
    if (replay_.data() || replay_load_.busy()) { show_status_message(ui::tr(L"Unload offline replay before arming live RX recording."), 5000); return; }
    if(source>1||(source&&!workbench_))return;
    auto& recorder=source?workbench_->secondary_.recorder:recorder_;
    if (recorder.status().active) return;
    SYSTEMTIME time{}; GetLocalTime(&time);
    const auto filename = std::wstring(source?L"UART_B_":L"") + capture_filename(time);
    std::optional<std::filesystem::path> selected;
    if(recording_picker_)selected=recording_picker_(filename,source);
    else {
    CFileDialog dialog(FALSE, L"log", filename.c_str(), OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
        ui::caption(L"Recording log (*.log)|*.log||"), source?static_cast<CWnd*>(workbench_.get()):this);
    dialog.m_ofn.lpstrTitle = ui::caption(L"Start RX recording");
    if (dialog.DoModal() != IDOK) return;
    selected=std::filesystem::path(dialog.GetPathName().GetString());
    }
    if(!selected)return;
    auto readable_base=*selected;readable_base.replace_extension();
    const auto raw_directory=std::filesystem::path(settings_path_).parent_path()/L"log";
    std::error_code directory_error;std::filesystem::create_directories(raw_directory,directory_error);
    if(directory_error||!std::filesystem::is_directory(raw_directory)){
        AfxMessageBox((ui::tr(L"Recording failed. Check the filename, folder permissions and available disk space. Existing files were kept.")+L"\n\n"+raw_directory.wstring()).c_str(),MB_ICONWARNING);return;
    }
    RecordingOptions options;try{options=saved_recording_options(settings_path_,raw_directory/readable_base.filename());}catch(const std::exception& e){AfxMessageBox(uart_wide(e.what()).c_str(),MB_ICONWARNING);return;}
    options.readable_base=std::move(readable_base);
    try{options.metadata=encode_capture_metadata(current_profile(),CHART_VERSION_STR,source?"B":"A");}catch(const std::exception& e){AfxMessageBox(uart_wide(e.what()).c_str(),MB_ICONWARNING);return;}
    if (!recorder.start(options)) { uart_notice_(m_hWnd,ui::caption(L"Cannot start another recording."),ui::caption(L"Recording"),MB_OK|MB_ICONWARNING); return; }
    if(!source)recording_interactive_=true;
    recorder.event("PC receipt timestamps; raw file contains RX only; text escapes non-ASCII/control bytes", message_unix_ms());
    poll_recording();
}

void CMainDialog::recording_options() {
    RecordingOptions options;try{options=saved_recording_options(settings_path_);}catch(...){show_status_message(L"Invalid saved recording limits; review and save valid options.",6000);}
    RecordingDialog dialog(this,options);
    if (dialog.DoModal() != IDOK) return;
    if (!WritePrivateProfileStringW(L"Recording", L"RotateMiB", std::to_wstring(dialog.mib_).c_str(), settings_path_.c_str()) ||
        !WritePrivateProfileStringW(L"Recording", L"RotateDaily", dialog.daily_ ? L"1" : L"0", settings_path_.c_str()) ||
        !WritePrivateProfileStringW(L"Recording", L"Limits", uart_wide(encode_recording_limits(dialog.limits_)).c_str(), settings_path_.c_str()))
        AfxMessageBox(L"Cannot save recording options beside EXE. Use a writable folder.", MB_ICONWARNING);
    else show_status_message(L"Recording options saved; apply on the next recording.", 5000);
}

void CMainDialog::poll_recording() {
    const auto status = recorder_.status();
    const bool can_start=!closing_serial_&&!replay_.data()&&!replay_load_.busy()&&!profile_job_.busy();
    message_page_.set_recording_status(status,can_start);
    if (auto menu = GetMenu()) {
        menu->EnableMenuItem(ID_UART_RECORD_START, MF_BYCOMMAND | (status.active||!can_start ? MF_GRAYED : MF_ENABLED));
        menu->EnableMenuItem(ID_UART_RECORD_STOP, MF_BYCOMMAND | (status.active && !status.stopping ? MF_ENABLED : MF_GRAYED));
        menu->CheckMenuItem(ID_UART_RECORD_START, MF_BYCOMMAND | (status.active ? MF_CHECKED : MF_UNCHECKED));
    }
    if (status.detail != recorder_notice_) {
        recorder_notice_ = status.detail;
        if (status.detail.find(L"ERROR") != std::wstring::npos || status.lost || status.limit_reached) {
            log_line(status.detail);show_status_message(ui::tr(status.limit_reached?L"Recording stopped at a configured limit. Existing files were kept.":status.lost?L"Recording stopped because input was lost. Existing files were kept.":L"Recording failed. Check the filename, folder permissions and available disk space. Existing files were kept."),30000);
            if(recording_interactive_&&status.detail.find(L"ERROR")!=status.detail.npos){recording_interactive_=false;uart_notice_(m_hWnd,ui::caption(L"Recording failed. Check the filename, folder permissions and available disk space. Existing files were kept."),ui::caption(L"Recording"),MB_OK|MB_ICONWARNING);}
        } else if (status.active || status.segments) show_status_message(ui::tr(status.active?L"Recording started. Current log filename and KB are shown on Messages.":L"Recording stopped. Log filename and KB remain visible on Messages."),5000);
        if(!status.active&&(status.segments||status.detail.find(L"ERROR")!=status.detail.npos))set_right_status(ui::tr(status.detail.find(L"ERROR")!=status.detail.npos?L"Recording error":L"Recording stopped"));
    }
    if (status.active) set_right_status(ui::tr(L"Segments:")+L" " + std::to_wstring(status.segments) +
        (status.stopping ? L" | "+ui::tr(L"Finishing...") : L""));
    if (close_after_recording_ && !status.active) {
        close_after_recording_ = false;
        if (status.lost || status.detail.find(L"ERROR") != std::wstring::npos) {
            if(workbench_)workbench_->cancel_shutdown();
            AfxMessageBox((L"Recording did not finish cleanly. Existing partial files were preserved.\n\n" + status.detail + L"\n\nReview the files before closing.").c_str(), MB_ICONWARNING);
        }
        else OnCancel();
    }
}

LRESULT CMainDialog::OnMessageLogRequest(WPARAM action,LPARAM){
    if(!ui_ready_||TabCtrl_GetCurSel(tabs_)!=kMessagesTab)return 0;
    if(action==MessagePage::SaveLog)save_capture_as();
    else if(action==MessagePage::StartLog)start_recording();
    else if(action==MessagePage::StopLog)recorder_.stop();
    poll_recording();return 0;
}
