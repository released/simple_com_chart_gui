#include "mfc_main_dialog.h"
#include "capture_export.h"
#include <filesystem>

namespace {
class CaptureSaveDialog : public CFileDialog {
public:
    explicit CaptureSaveDialog(CWnd* parent)
        : CFileDialog(FALSE, L"log", L"capture.log", OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
            L"Text log (*.log)|*.log|Text file (*.txt)|*.txt|Excel 97-2003 (*.xls)|*.xls|CSV (*.csv)|*.csv|CVS alias (*.cvs)|*.cvs||", parent) {}
protected:
    void OnTypeChange() override {
        const wchar_t* extensions[] = {L"log", L"txt", L"xls", L"csv", L"cvs"};
        const DWORD index = GetOFN().nFilterIndex;
        if (::IsWindow(m_hWnd) && index >= 1 && index <= 5) SetDefExt(extensions[index - 1]);
    }
};
}

void CMainDialog::save_capture_as() {
    if (exporting_) return;
    if (capture_.rows().empty()) {
        show_status_message(L"No captured lines to save. Connect or start --demo first.", 5000); return;
    }
    CaptureSaveDialog dialog(this);
    if (dialog.DoModal() != IDOK) return;
    const std::filesystem::path path(dialog.GetPathName().GetString());
    CaptureFormat format;
    try { format = capture_format(path); }
    catch (const std::exception& error) { AfxMessageBox(CString(error.what()), MB_ICONWARNING); return; }
    if (format == CaptureFormat::Xls && capture_.keys().size() > 254) {
        AfxMessageBox(L"This capture has more than 254 telemetry fields. Choose CSV or log instead.", MB_ICONWARNING); return;
    }
    begin_capture_export(path, format);
}

bool CMainDialog::begin_capture_export(const std::filesystem::path& path, CaptureFormat format) {
    if (exporting_) return false;
    auto snapshot = capture_.snapshot();
    const auto count = snapshot.rows.size(), dropped = snapshot.dropped;
    exporting_ = true; ::EnableWindow(btn_save_, FALSE);
    show_status_message(L"Saving capture...", 3000);
    try {
        if (!export_thread_.joinable()) export_thread_ = std::thread([this] {
            for (;;) {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(export_mutex_);
                    export_wake_.wait(lock, [this] { return stop_export_ || static_cast<bool>(export_job_); });
                    if (stop_export_) return;
                    job = std::move(export_job_); export_job_ = {};
                }
                job();
            }
        });
        std::lock_guard<std::mutex> job_lock(export_mutex_);
        export_job_ = [this, path, format, snapshot = std::move(snapshot), count, dropped] {
            std::wstring message;
            try {
                export_capture(snapshot, path, format);
                message = L"Saved " + std::to_wstring(count) + L" rows: " + path.wstring();
                if (dropped) message += L" (older discarded rows are not included)";
            } catch (const std::exception& error) {
                const std::string detail = error.what();
                message = L"Save failed: " + std::wstring(detail.begin(), detail.end());
            }
            { std::lock_guard<std::mutex> lock(export_mutex_); export_result_ = message; }
            PostMessage(WM_APP + 2);
        };
        export_wake_.notify_one();
    } catch (...) {
        exporting_ = false; ::EnableWindow(btn_save_, TRUE);
        show_status_message(L"Cannot start export worker.", 5000);
        return false;
    }
    return true;
}

LRESULT CMainDialog::OnExportComplete(WPARAM, LPARAM) {
    exporting_ = false; ::EnableWindow(btn_save_, TRUE);
    std::lock_guard<std::mutex> lock(export_mutex_);
    show_status_message(export_result_, 15000);
    if (smoke_export_pending_) {
        smoke_export_pending_ = false;
        const auto path = std::filesystem::path(settings_path_).parent_path() / L"runtime-capture.log";
        std::ifstream file(path, std::ios::binary);
        const std::string contents{std::istreambuf_iterator<char>(file), {}};
        finish_smoke_test(smoke_checks_passed_ && export_result_.find(L"Saved ") == 0 &&
            contents.find("fan,duty:") != std::string::npos && contents.find("\r\n") != std::string::npos);
    }
    return 0;
}
