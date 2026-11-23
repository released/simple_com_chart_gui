#include "mfc_main_dialog.h"
#include "capture_export.h"
#include "capture_filename.h"
#include "error_message.h"
#include <filesystem>

namespace {
struct EmptyMessageExport : std::exception {};
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
    if(TabCtrl_GetCurSel(tabs_)==kModbusTab){save_protocol_file(false);return;}
    if(TabCtrl_GetCurSel(tabs_)==kLinTab){save_protocol_file(true,true);return;}
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
    if (message_page_.buffer().bytes().empty()) { show_status_message(ui::tr(L"No messages to save."), 5000); return false; }
    auto snapshot = message_page_.export_snapshot();
    auto bytes=std::make_shared<uint64_t>(0);
    auto success = std::wstring(L"Saved Messages (") + (message_page_.hex_mode() ? L"HEX" : L"ASCII") + L"): " + path.wstring();
    if (message_page_.buffer().offset()) success += L" (older dropped/lost bytes are not included)";
    if(!begin_export_job([path, snapshot=std::move(snapshot),bytes] {const auto text=snapshot();if(text.empty())throw EmptyMessageExport{};export_message_text(text,path);*bytes=text.size();},std::move(success),bytes))return false;
    message_export_pending_=true;message_page_.stage_log_export(path.filename().wstring(),0);return true;
}

bool CMainDialog::begin_export_job(std::function<void()> write, std::wstring success,std::shared_ptr<uint64_t> bytes) {
    if (exporting_) return false;
    exporting_ = true; ::EnableWindow(btn_save_, FALSE);
    message_page_.set_exporting(true);show_status_message(ui::tr(L"Saving..."), 3000);
    const HWND owner=m_hWnd;
    bool started=false;
    try { started=export_job_.start([owner, write=std::move(write), success=std::move(success),bytes](const std::atomic<bool>&) mutable {
            ExportResult result;
            try {
                write(); result.saved=true; result.detail=std::move(success);if(bytes)result.bytes=*bytes;
            } catch (const EmptyMessageExport&) {
                result.empty=true;result.detail=L"No messages to save.";
            } catch (const std::exception& error) {
                result.detail=L"Save failed: "+error_message(error.what());
            } catch (...) {
                result.detail=L"Save failed: Background operation failed";
            }
            // This is only a wake-up hint. The owner also polls the retained result.
            ::PostMessageW(owner,WM_APP+2,0,0);
            return result;
        }); }catch(...){ /* Keep allocation/start failure on the same visible path. */ }
    if(!started) {
        exporting_ = false; ::EnableWindow(btn_save_, TRUE);
        message_page_.set_exporting(false);show_status_message(ui::tr(L"Cannot start export worker."), 5000);
        return false;
    }
    return true;
}

LRESULT CMainDialog::OnExportComplete(WPARAM, LPARAM) {
    ExportResult completed;std::wstring error;
    if(!exporting_||!export_job_.take(completed,error,false))return 0;
    // A committed save remains successful even if close was requested meanwhile.
    export_result_=error.empty()?completed.detail:L"Save failed: "+error;
    exporting_ = false; ::EnableWindow(btn_save_, TRUE);
    message_page_.set_exporting(false);
    const auto& result=export_result_;
    if(message_export_pending_){
        const bool success=completed.saved&&error.empty();message_page_.finish_log_export(success,completed.bytes);message_export_pending_=false;
        show_status_message(ui::tr(completed.empty?L"No messages to save.":success?L"Message log saved. Filename and size are shown beside the recording controls.":L"Log save failed. Check the filename, folder permissions and whether the file is in use."),15000);
        const bool notify=message_export_interactive_;message_export_interactive_=false;
        if(!success&&!completed.empty&&notify)uart_notice_(m_hWnd,ui::caption(L"Log save failed. Check the filename, folder permissions and whether the file is in use."),ui::caption(L"Save As..."),MB_OK|MB_ICONWARNING);
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
