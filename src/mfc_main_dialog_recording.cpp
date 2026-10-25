#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "capture_filename.h"

namespace {
class RecordingDialog : public CDialog {
public:
    RecordingDialog(CWnd* parent, unsigned mib, bool daily) : CDialog(IDD_RECORD_OPTIONS, parent), mib_(mib), daily_(daily) {}
    unsigned mib_; bool daily_;
    BOOL OnInitDialog() override {
        CDialog::OnInitDialog(); SetDlgItemInt(IDC_RECORD_MIB, mib_, FALSE);
        CheckDlgButton(IDC_RECORD_DAILY, daily_ ? BST_CHECKED : BST_UNCHECKED); return TRUE;
    }
    void OnOK() override {
        BOOL valid = FALSE; const auto mib = GetDlgItemInt(IDC_RECORD_MIB, &valid, FALSE);
        if (!valid || mib < 1 || mib > 1024) { AfxMessageBox(L"Choose a rotation size from 1 to 1024 MiB.", MB_ICONWARNING); return; }
        mib_ = mib; daily_ = IsDlgButtonChecked(IDC_RECORD_DAILY) == BST_CHECKED; CDialog::OnOK();
    }
};
}

void CMainDialog::start_recording() {
    if (replay_.data() || replay_load_.busy()) { show_status_message(L"Unload offline replay before arming live RX recording.", 5000); return; }
    if (recorder_.status().active) return;
    SYSTEMTIME time{}; GetLocalTime(&time);
    const auto filename = L"UART_" + capture_filename(time);
    CFileDialog dialog(FALSE, L"log", filename.c_str(), OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
        L"Recording prefix (*.log)|*.log||", this);
    dialog.m_ofn.lpstrTitle = L"Choose recording prefix (.bin / .log / .idx created; existing files never overwritten)";
    if (dialog.DoModal() != IDOK) return;
    RecordingOptions options; options.base = std::filesystem::path(dialog.GetPathName().GetString()).replace_extension();
    options.rotate_bytes = static_cast<uint64_t>(std::clamp<unsigned>(GetPrivateProfileIntW(L"Recording", L"RotateMiB", 32, settings_path_.c_str()), 1, 1024)) * 1024 * 1024;
    options.rotate_daily = GetPrivateProfileIntW(L"Recording", L"RotateDaily", 1, settings_path_.c_str()) != 0;
    if (!recorder_.start(options)) { AfxMessageBox(L"Cannot start another recording.", MB_ICONWARNING); return; }
    recorder_.event("PC receipt timestamps; raw file contains RX only; text escapes non-ASCII/control bytes", message_unix_ms());
    poll_recording();
}

void CMainDialog::recording_options() {
    RecordingDialog dialog(this, std::clamp<unsigned>(GetPrivateProfileIntW(L"Recording", L"RotateMiB", 32, settings_path_.c_str()), 1, 1024),
        GetPrivateProfileIntW(L"Recording", L"RotateDaily", 1, settings_path_.c_str()) != 0);
    if (dialog.DoModal() != IDOK) return;
    if (!WritePrivateProfileStringW(L"Recording", L"RotateMiB", std::to_wstring(dialog.mib_).c_str(), settings_path_.c_str()) ||
        !WritePrivateProfileStringW(L"Recording", L"RotateDaily", dialog.daily_ ? L"1" : L"0", settings_path_.c_str()))
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
        if (status.detail.find(L"ERROR") != std::wstring::npos || status.lost) {
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
