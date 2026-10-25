#include "mfc_main_dialog.h"
#include <cmath>
LRESULT CMainDialog::OnLocateSource(WPARAM,LPARAM value) {
    if(!value) return 0;const auto offset=*reinterpret_cast<const uint64_t*>(value);
    for(const auto& row:capture_.rows()) if(row->source_offset!=UINT64_MAX && offset>=row->source_offset && offset-row->source_offset<row->source_count) {
        data_page_.select_sequence(row->sequence);plot_view_.navigate_to(row->source_seconds+plot_view_.view_window()/4);
        TabCtrl_SetCurSel(tabs_,kDataTab);TabCtrl_SetCurFocus(tabs_,kDataTab);switch_page();show_status_message(L"Located retained capture row and chart time from original RX position.",5000);return 0;
    }
    show_status_message(L"No retained/accepted capture row at this RX position (filtered, expired, or cleared).",6000);return 0;
}

LRESULT CMainDialog::OnSourceNavigate(WPARAM,LPARAM value) {
    if(!value) return 0;const auto row=*reinterpret_cast<const CaptureRow*>(value);
    plot_view_.navigate_to(row.source_seconds+plot_view_.view_window()/4);data_page_.select_sequence(row.sequence);
    TabCtrl_SetCurSel(tabs_,kMessagesTab);TabCtrl_SetCurFocus(tabs_,kMessagesTab);switch_page();
    if(row.source_offset==UINT64_MAX || !message_page_.navigate_source(row.source_offset,row.raw.size())) show_status_message(L"Original Messages bytes expired, were cleared, or display is paused; no guessed match was selected. Capture row/chart time remain available.",9000);
    else show_status_message(L"Located original RX bytes. Chart time and retained table row are aligned.",5000);
    return 0;
}
LRESULT CMainDialog::OnPlotCursor(WPARAM,LPARAM value) {
    if(!value || capture_.rows().empty()) return 0;double time=*reinterpret_cast<const double*>(value);
    // Coincident plot samples receive a small monotonic epsilon; RX row clocks remain original.
    const auto end=capture_.rows().back()->source_seconds;
    if(time>end && time<=model_.newest_timestamp()) time=end;
    if(!data_page_.select_source_time(time)) {show_status_message(L"No retained capture row at this chart time (expired/outside capture).",5000);return 0;}
    const int selected=ListView_GetNextItem(data_page_.table(),-1,LVNI_SELECTED);
    if(selected>=0 && static_cast<size_t>(selected)<capture_.rows().size()) {const auto& row=capture_.rows()[selected];if(row->source_offset!=UINT64_MAX) message_page_.navigate_source(row->source_offset,row->raw.size());show_status_message(L"Chart cursor aligned to retained table / RX. Double-click a table row to show original Messages.",5000);}return 0;
}
