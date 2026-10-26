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
    if (replay_.data() || replay_load_.busy()) { show_status_message(L"Unload offline replay before arming live RX recording.", 5000); return; }
    if(source>1||(source&&!workbench_))return;
    auto& recorder=source?workbench_->secondary_.recorder:recorder_;
    if (recorder.status().active) return;
    SYSTEMTIME time{}; GetLocalTime(&time);
    const auto filename = std::wstring(source?L"UART_B_":L"UART_A_") + capture_filename(time);
    CFileDialog dialog(FALSE, L"log", filename.c_str(), OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
        L"Recording prefix (*.log)|*.log||", source?static_cast<CWnd*>(workbench_.get()):this);
    dialog.m_ofn.lpstrTitle = L"Choose prefix (.bin/.log/.idx + private capture-start .sccmeta; existing files kept)";
    if (dialog.DoModal() != IDOK) return;
    RecordingOptions options;try{options=saved_recording_options(settings_path_,std::filesystem::path(dialog.GetPathName().GetString()).replace_extension());}catch(const std::exception& e){AfxMessageBox(uart_wide(e.what()).c_str(),MB_ICONWARNING);return;}
    try{options.metadata=encode_capture_metadata(current_profile(),CHART_VERSION_STR,source?"B":"A");}catch(const std::exception& e){AfxMessageBox(uart_wide(e.what()).c_str(),MB_ICONWARNING);return;}
    if (!recorder.start(options)) { AfxMessageBox(L"Cannot start another recording.", MB_ICONWARNING); return; }
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
    if (auto menu = GetMenu()) {
        menu->EnableMenuItem(ID_UART_RECORD_START, MF_BYCOMMAND | (status.active ? MF_GRAYED : MF_ENABLED));
        menu->EnableMenuItem(ID_UART_RECORD_STOP, MF_BYCOMMAND | (status.active && !status.stopping ? MF_ENABLED : MF_GRAYED));
        menu->CheckMenuItem(ID_UART_RECORD_START, MF_BYCOMMAND | (status.active ? MF_CHECKED : MF_UNCHECKED));
    }
    if (status.detail != recorder_notice_) {
        recorder_notice_ = status.detail;
        if (status.detail.find(L"ERROR") != std::wstring::npos || status.lost || status.limit_reached) {
            log_line(status.detail); show_status_message(status.detail, 30000);
        } else if (status.active || status.segments) show_status_message(status.detail, 5000);
    }
    if (status.active) set_right_status(L"Segments: " + std::to_wstring(status.segments) +
        (status.stopping ? L" | Finishing..." : L""));
    if (close_after_recording_ && !status.active) {
        close_after_recording_ = false;
        if (status.lost || status.detail.find(L"ERROR") != std::wstring::npos) {
            if(workbench_)workbench_->cancel_shutdown();
            AfxMessageBox((L"Recording did not finish cleanly. Existing partial files were preserved.\n\n" + status.detail + L"\n\nReview the files before closing.").c_str(), MB_ICONWARNING);
        }
        else OnCancel();
    }
}
