#include "mfc_main_dialog.h"
#include "capture_export.h"
#include "capture_filename.h"
#include <filesystem>

namespace {
class CaptureSaveDialog : public CFileDialog {
public:
    CaptureSaveDialog(CWnd* parent, const std::wstring& filename, bool messages)
        : CFileDialog(FALSE, L"log", filename.c_str(), OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
            messages ? L"Text log (*.log)|*.log|Text file (*.txt)|*.txt||" :
            L"Text log (*.log)|*.log|Text file (*.txt)|*.txt|Excel 97-2003 (*.xls)|*.xls|CSV (*.csv)|*.csv|CVS alias (*.cvs)|*.cvs||", parent),
            messages_(messages) { m_ofn.lpstrTitle = messages ? L"Save Messages (current ASCII / HEX view)" : L"Save filtered capture"; }
protected:
    void OnTypeChange() override {
        const wchar_t* extensions[] = {L"log", L"txt", L"xls", L"csv", L"cvs"};
        const DWORD index = GetOFN().nFilterIndex;
        if (::IsWindow(m_hWnd) && index >= 1 && index <= (messages_ ? 2U : 5U)) SetDefExt(extensions[index - 1]);
    }
private:
    bool messages_;
};
}

void CMainDialog::save_capture_as() {
    if (exporting_) return;
    const bool messages = TabCtrl_GetCurSel(tabs_) == kMessagesTab;
    if (messages) flush_pending_lines();
    if (messages ? message_page_.buffer().bytes().empty() : capture_.rows().empty()) {
        show_status_message(messages ? L"No messages to save." : L"No captured lines to save. Connect or start --demo first.", 5000); return;
    }
    SYSTEMTIME time{}; GetLocalTime(&time);
    const auto filename = capture_filename(time);
    CaptureSaveDialog dialog(this, filename, messages);
    if (dialog.DoModal() != IDOK) return;
    const std::filesystem::path path(dialog.GetPathName().GetString());
    CaptureFormat format;
    try { format = capture_format(path); }
    catch (const std::exception& error) {
        AfxMessageBox(messages ? CString(L"Messages supports .log and .txt only.") : CString(error.what()), MB_ICONWARNING); return;
    }
    if (messages) {
        if (format != CaptureFormat::Text) { AfxMessageBox(L"Messages supports .log and .txt only.", MB_ICONWARNING); return; }
        flush_pending_lines(); begin_message_export(path); return;
    }
    if (format == CaptureFormat::Xls && capture_.keys().size() > 254) {
        AfxMessageBox(L"This capture has more than 254 telemetry fields. Choose CSV or log instead.", MB_ICONWARNING); return;
    }
    begin_capture_export(path, format);
}

bool CMainDialog::begin_capture_export(const std::filesystem::path& path, CaptureFormat format) {
    if (exporting_) return false;
    auto snapshot = capture_.snapshot();
    const auto count = snapshot.rows.size(), dropped = snapshot.dropped;
    auto success = L"Saved " + std::to_wstring(count) + L" rows: " + path.wstring();
    if (dropped) success += L" (older discarded rows are not included)";
    return begin_export_job([path, format, snapshot = std::move(snapshot)] { export_capture(snapshot, path, format); }, std::move(success));
}

bool CMainDialog::begin_message_export(const std::filesystem::path& path) {
    if (exporting_) return false;
    auto text = message_page_.export_text();
    if (text.empty()) { show_status_message(L"No messages to save.", 5000); return false; }
    auto success = std::wstring(L"Saved Messages (") + (message_page_.hex_mode() ? L"HEX" : L"ASCII") + L"): " + path.wstring();
    if (message_page_.buffer().offset()) success += L" (older dropped/lost bytes are not included)";
    return begin_export_job([path, text = std::move(text)] { export_message_text(text, path); }, std::move(success));
}

bool CMainDialog::begin_export_job(std::function<void()> write, std::wstring success) {
    if (exporting_) return false;
    exporting_ = true; ::EnableWindow(btn_save_, FALSE);
    show_status_message(L"Saving...", 3000);
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
        export_job_ = [this, write = std::move(write), success = std::move(success)] {
            std::wstring message;
            try {
                write(); message = success;
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
