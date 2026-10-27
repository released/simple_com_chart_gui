#include "mfc_main_dialog.h"
#include "capture_export.h"
#include "capture_filename.h"
#include <filesystem>

namespace {
class CaptureSaveDialog : public CFileDialog {
public:
    CaptureSaveDialog(CWnd* parent, const std::wstring& filename, bool messages)
        : CFileDialog(FALSE, L"log", filename.c_str(), OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR,
            ui::caption(messages ? L"Text log (*.log)|*.log|Text file (*.txt)|*.txt||" :
            L"Text log (*.log)|*.log|Text file (*.txt)|*.txt|Excel 97-2003 (*.xls)|*.xls|CSV (*.csv)|*.csv|CVS alias (*.cvs)|*.cvs||"), parent),
            messages_(messages) { m_ofn.lpstrTitle = ui::caption(messages ? L"Save Messages (current display)" : L"Save filtered capture"); }
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
        show_status_message(ui::tr(messages ? L"No messages to save." : L"No captured lines to save. Receive or replay data first."), 5000);
        if(messages)uart_notice_(m_hWnd,ui::caption(L"No messages to save yet. Receive or replay data first. To save upcoming data continuously, click Start recording."),ui::caption(L"Save As..."),MB_OK|MB_ICONINFORMATION);
        return;
    }
    SYSTEMTIME time{}; GetLocalTime(&time);
    const auto filename = capture_filename(time);
    std::optional<std::filesystem::path> selected;
    if(capture_picker_)selected=capture_picker_(filename,messages);
    else {CaptureSaveDialog dialog(this, filename, messages);if(dialog.DoModal()!=IDOK)return;selected=std::filesystem::path(dialog.GetPathName().GetString());}
    if(!selected)return;const auto path=*selected;
    CaptureFormat format;
    try { format = capture_format(path); }
    catch (const std::exception& error) {
        uart_notice_(m_hWnd,messages?ui::caption(L"Messages supports .log and .txt only."):CString(error.what()).GetString(),ui::caption(L"Save As..."),MB_OK|MB_ICONWARNING); return;
    }
    if (messages) {
        if (format != CaptureFormat::Text) { uart_notice_(m_hWnd,ui::caption(L"Messages supports .log and .txt only."),ui::caption(L"Save As..."),MB_OK|MB_ICONWARNING); return; }
        flush_pending_lines();message_export_interactive_=begin_message_export(path);return;
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
    if (text.empty()) { show_status_message(ui::tr(L"No messages to save."), 5000); return false; }
    const auto bytes=text.size();
    auto success = std::wstring(L"Saved Messages (") + (message_page_.hex_mode() ? L"HEX" : L"ASCII") + L"): " + path.wstring();
    if (message_page_.buffer().offset()) success += L" (older dropped/lost bytes are not included)";
    if(!begin_export_job([path, text = std::move(text)] { export_message_text(text, path); }, std::move(success)))return false;
    message_export_pending_=true;message_page_.stage_log_export(path.filename().wstring(),bytes);return true;
}

bool CMainDialog::begin_export_job(std::function<void()> write, std::wstring success) {
    if (exporting_) return false;
    exporting_ = true; ::EnableWindow(btn_save_, FALSE);
    message_page_.set_exporting(true);show_status_message(ui::tr(L"Saving..."), 3000);
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
        message_page_.set_exporting(false);show_status_message(ui::tr(L"Cannot start export worker."), 5000);
        return false;
    }
    return true;
}

LRESULT CMainDialog::OnExportComplete(WPARAM, LPARAM) {
    exporting_ = false; ::EnableWindow(btn_save_, TRUE);
    message_page_.set_exporting(false);
    std::wstring result;{std::lock_guard<std::mutex> lock(export_mutex_);result=export_result_;}
    if(message_export_pending_){
        const bool success=result.rfind(L"Saved Messages",0)==0;message_page_.finish_log_export(success);message_export_pending_=false;
        show_status_message(ui::tr(success?L"Message log saved. Filename and size are shown beside the recording controls.":L"Log save failed. Check the filename, folder permissions and whether the file is in use."),15000);
        const bool notify=message_export_interactive_;message_export_interactive_=false;
        if(!success&&notify)uart_notice_(m_hWnd,ui::caption(L"Log save failed. Check the filename, folder permissions and whether the file is in use."),ui::caption(L"Save As..."),MB_OK|MB_ICONWARNING);
    } else show_status_message(result,15000);
#ifdef CHART_RUNTIME_CHECKS
    if (smoke_export_pending_) {
        smoke_export_pending_ = false;
        const auto path = std::filesystem::path(settings_path_).parent_path() / L"runtime-capture.log";
        std::ifstream file(path, std::ios::binary);
        const std::string contents{std::istreambuf_iterator<char>(file), {}};
        finish_smoke_test(smoke_checks_passed_ && export_result_.find(L"Saved ") == 0 &&
            contents.find("fan,duty:") != std::string::npos && contents.find("\r\n") != std::string::npos);
    }
#endif
    return 0;
}
