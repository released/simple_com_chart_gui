#include "mfc_main_dialog.h"
#include "session_manifest.h"
#include "build_version.h"
void CMainDialog::lab_session_request(const UartLabWindow::Request& r){
    if(r.action==7){lab_file_.cancel();return;}if(r.fields.size()!=5)return;
    if(r.action<=2){const bool source=r.action==0,open=r.action!=2;CFileDialog pick(open,source?L"bin":L"sccsession",nullptr,OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(open?OFN_FILEMUSTEXIST:0),source?L"Recording (*.bin)|*.bin||":L"Session manifest (*.sccsession)|*.sccsession||",&lab_);if(pick.DoModal()==IDOK&&::IsWindow(lab_.m_hWnd))lab_.field(r.page,source?0:1,pick.GetPathName().GetString());return;}
    if(r.action==6){if(!has_staged_profile_)throw std::runtime_error("Verify and stage settings first.");show_advanced_tools(AdvancedTools::Profiles);advanced_tools_.notice(AdvancedTools::Profiles,L"Session settings staged. Apply requires idle ports/unloaded replay; no port will open. Replay path is staged separately.");return;}
    const auto manifest=std::filesystem::path(r.fields[1]);
    if(r.action==3){double interval=0;if(!uart_number(r.fields[2],1,60000,interval)||interval!=std::floor(interval))throw std::runtime_error("Interval must be a whole 1..60000 ms.");
        CaptureMetadata current;current.profile=current_profile();current.version=CHART_VERSION_STR;current.source="Current";const auto source=std::filesystem::path(r.fields[0]);const bool captured=r.fields[3]==L"Capture-start metadata",logs=r.fields[4]==L"Yes";
        start_lab_file(r.page,r.action,[manifest,source,current,captured,logs,interval](const std::atomic<bool>& cancel){auto result=create_session_manifest(source,manifest,current,captured,logs,static_cast<unsigned>(interval),cancel);return LabFileResult{session_manifest_report(result),{}};});return;
    }
    if(r.action!=4&&r.action!=5)return;
    start_lab_file(r.page,r.action,[manifest](const std::atomic<bool>& cancel){const auto s=verify_session_manifest(manifest,cancel);LabFileResult result;result.report="All listed files match size/SHA256; recording structure/segments checked.\r\n"+session_manifest_report(s);result.payload=encode_profile(s.metadata.profile);result.source=std::filesystem::absolute(manifest).parent_path()/s.source;result.interval=s.interval_ms;return result;});
}
