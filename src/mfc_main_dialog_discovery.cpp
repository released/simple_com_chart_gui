#include "mfc_main_dialog.h"
#include "discovery_list.h"
#include "uart_tool_form.h"
#include "uart_workbench.h"
#include <sstream>
#include <cmath>
namespace {
struct ModalFlag {bool& value;explicit ModalFlag(bool& v):value(v){value=true;}~ModalFlag(){value=false;}};
std::wstring number(double v){std::wostringstream s;s<<v;return s.str();}
std::wstring search_summary(const MessageSearchResults& result){
    auto text=std::to_wstring(result.matches)+L" "+ui::tr(result.limited?L"matches; limit reached (4096)":result.skipped_rows?L"matches; partial retained snapshot":L"matches; complete retained snapshot");
    if(result.skipped_rows)text+=L" | "+std::to_wstring(result.skipped_rows)+L" "+ui::tr(L"oversized/work-limited rows skipped");
    return text;
}
}
void CMainDialog::discovery_lens(){
    if(discovery_modal_||closing_serial_)return;ModalFlag modal(discovery_modal_);
    const auto c=message_page_.lens_config();
    UartToolForm form(L"Messages display lens",L"Display only; capture and recording remain unchanged. Raw text mode is required. Include/Exclude use literal case-sensitive text or the bounded chart Regex subset. Exclude wins, including context. Context is 0..10 logical rows. Session only; disabling restores retained messages.",
        {{L"Enabled",message_page_.lens_enabled()?L"On":L"Off",{L"Off",L"On"}},{L"Include",uart_wide(c.include)},{L"Exclude",uart_wide(c.exclude)},{L"Filter mode",c.regex?L"Regex":L"Text",{L"Text",L"Regex"}},{L"Before rows",std::to_wstring(c.before)},{L"After rows",std::to_wstring(c.after)}},{},
        [this](const auto& f,std::wstring& error){double before=0,after=0;if(!uart_number(f[4],0,10,before)||!uart_number(f[5],0,10,after)||before!=std::floor(before)||after!=std::floor(after)){error=ui::tr(L"Context must be whole rows from 0 to 10.");return false;}MessageLensConfig config{message_utf8(f[1]),message_utf8(f[2]),f[3]==L"Regex",static_cast<unsigned>(before),static_cast<unsigned>(after)};return message_page_.set_display_lens(f[0]==L"On",config,error);},this);
    form.DoModal();
}
void CMainDialog::discovery_search(){
    if(discovery_modal_||closing_serial_||discovery_search_.busy()){show_status_message(ui::tr(L"Wait for the current search."),4000);return;}
    ModalFlag modal(discovery_modal_);UartToolForm form(L"Retained message search",L"Search an immutable retained raw-text snapshot (up to 4096 matches). Terminal edits and the display lens do not alter this source. Results include receipt times and 64 frequency buckets; locating a result rechecks retained source identity.",
        {{L"Pattern",L""},{L"Search mode",L"Text",{L"Text",L"Regex"}},{L"Match case",L"Yes",{L"Yes",L"No"}}},{},[](const auto& f,std::wstring& e){if(f[0].empty()||f[0].size()>256){e=ui::tr(L"Enter 1..256 search characters.");return false;}return true;},this);
    form.accept_caption(L"Search snapshot");if(form.DoModal()!=IDOK||closing_serial_)return;
    const auto f=form.values();const auto bytes=message_page_.buffer();const auto options=message_page_.retained_display_options();const auto epoch=message_page_.history_epoch(),end=message_page_.source_end();const auto synthetic=message_page_.synthetic_source();
    if(!discovery_search_.start([bytes,options,epoch,end,synthetic,f](const std::atomic<bool>& cancel){DiscoverySearch out;out.epoch=epoch;out.offset=bytes.offset();if(end!=UINT64_MAX&&end>=bytes.bytes().size())out.base=end-bytes.bytes().size();if(cancel)return out;out.results=search_retained_messages(bytes,options,f[0],f[1]==L"Regex",f[2]==L"Yes");if(synthetic){out.results.timed=false;for(auto& row:out.results.rows)row.receipt=0;out.results.frequency=L"Synthetic source: receipt frequency unavailable.";}return out;},discovery_thread_launcher_)){show_status_message(ui::tr(L"Cannot start background operation; try again."),8000);return;}
    show_status_message(ui::tr(L"Searching retained snapshot in background..."),5000);
}
void CMainDialog::discovery_link(){
    if(discovery_modal_||closing_serial_)return;ModalFlag modal(discovery_modal_);
    UartToolForm form(L"Linked time cursor",L"Four plots keep independent fields, windows and Y axes. Link only the time marker. Values require a sample within the selected maximum age and the cursor's source/generation; other sources or missing data show n/a. Session only; off by default.",
        {{L"Enabled",linked_cursor_?L"On":L"Off",{L"Off",L"On"}},{L"Maximum sample age (s)",number(linked_cursor_age_)},{L"Capture time (s; empty = keep)",L""}},{},[](const auto& f,std::wstring& e){double age=0;if(!uart_number(f[1],.001,300,age)){e=ui::tr(L"Maximum age must be 0.001..300 seconds.");return false;}return true;},this);
    if(form.DoModal()!=IDOK||closing_serial_)return;const auto f=form.values();double requested=0;if(f[0]==L"On"&&!f[2].empty()&&(capture_.rows().empty()||!uart_number(f[2],capture_.rows().front()->seconds,capture_.rows().back()->seconds,requested))){show_status_message(ui::tr(L"Capture time is outside retained data; previous link settings retained."),7000);return;}linked_cursor_=f[0]==L"On";uart_number(f[1],.001,300,linked_cursor_age_);data_page_.show_linked_cursor(false,0,linked_cursor_age_);if(linked_cursor_&&!f[2].empty()){const auto& row=capture_.rows().front();const double time=row->source_seconds-row->seconds+requested;data_page_.select_source_time(time);data_page_.show_linked_cursor(true,time,linked_cursor_age_);}
    ::CheckMenuItem(::GetMenu(m_hWnd),ID_DISCOVERY_LINK,MF_BYCOMMAND|(linked_cursor_?MF_CHECKED:MF_UNCHECKED));
}
void CMainDialog::report_watches(const std::vector<NumericWatchEvent>& events){for(const auto& e:events){const bool has_value=e.state=="Below minimum"||e.state=="Above maximum"||e.state=="Returned to range"||e.state=="Data resumed";monitor_event(e.source?L"B":L"A",ui::tr(L"Numeric watch"),uart_wide(e.key)+L": "+ui::tr(uart_wide(e.state))+(has_value?L" | "+number(e.value):L""));}}
void CMainDialog::discovery_watches(){
    if(discovery_modal_||closing_serial_||profile_job_.busy())return;ModalFlag modal(discovery_modal_);
    std::wstring stored(33000,0);const auto n=GetPrivateProfileStringW(L"Monitor",L"WatchRules",L"",stored.data(),static_cast<DWORD>(stored.size()),settings_path_.c_str());stored.resize(n);std::wstring definition;
    const bool damaged=n>=32999||!monitor_unhex_text(stored,definition);if(damaged)definition.clear();
    auto help=ui::tr(L"One row: A/B;exact key;low;high;hysteresis;dwell seconds;stale seconds. Up to16 rules. Hysteresis <= half range; dwell 0..60 s; stale >0..300 s and >= dwell. Only local events: no TX or recording. Saved rules do not arm on launch. Capture reset/disconnect disarms. Example: A;temperature;0;80;2;0.5;2");
    if(damaged)help+=L"\r\n"+ui::tr(L"Saved watch definitions are invalid. Cancel preserves the file; Save explicitly replaces them.");
    UartToolForm form(L"Numeric watches",help,
        {{L"Action",L"Save definitions",{L"Save definitions",L"Arm now"}},{L"Rules",definition,{},8}},{},
        [this](const auto& f,std::wstring& error){std::vector<NumericWatchRule> rules;std::string detail;if(!parse_numeric_watches(message_utf8(f[1]),rules,detail)){error=ui::tr(L"Invalid numeric watch definitions.")+L"\r\n"+uart_wide(detail);return false;}
            if(f[0]==L"Arm now"){if(rules.empty()){error=ui::tr(L"Enter at least one watch rule.");return false;}for(const auto& r:rules)if(r.source?(!workbench_||!workbench_->secondary_connected()):(!serial_mgr_->is_connected()&&!replay_.data())){error=ui::tr(L"Connect the selected source or load replay before arming watches.");return false;}}
            if(!save_ini_patch(settings_path_,{{L"Monitor",L"WatchRules",monitor_hex_text(f[1])}},error))return false;
            watches_.stop();if(f[0]==L"Arm now")watches_.arm(std::move(rules),GetTickCount64());return true;},this);
    form.DoModal();
}
void CMainDialog::discovery_library(){
    if(discovery_library_.busy()||discovery_save_.busy()||closing_serial_){show_status_message(ui::tr(L"Wait for recording library work."),4000);return;}
    const auto path=std::filesystem::path(settings_path_).replace_extension(L".scclibrary");const auto recent=monitor_recent_;
    if(!discovery_library_.start([path,recent](const std::atomic<bool>& cancel){return read_recording_library(path,recent,cancel);},discovery_thread_launcher_)){show_status_message(ui::tr(L"Cannot start background operation; try again."),8000);return;}show_status_message(ui::tr(L"Reading recording references in background..."),5000);
}
void CMainDialog::poll_discovery(){
    if(!serial_mgr_->is_connected()&&!replay_.data())report_watches(watches_.disconnect(0));
    if(!workbench_||!workbench_->secondary_connected())report_watches(watches_.disconnect(1));
    report_watches(watches_.poll(GetTickCount64()));
    if(discovery_modal_||!::IsWindowEnabled(m_hWnd))return;
    std::wstring error;int saved=0;if(discovery_save_.take(saved,error)){if(!error.empty())show_status_message(error,8000);else show_status_message(ui::tr(L"Recording reference saved; raw files unchanged."),5000);}
    DiscoverySearch found;if(discovery_search_.take(found,error)&&!discovery_close_pending_){
        if(!error.empty()||!found.results.error.empty())show_status_message(error.empty()?found.results.error:error,8000);
        else {ModalFlag modal(discovery_modal_);DiscoveryListDialog::Rows rows;
            for(const auto& r:found.results.rows)rows.push_back({std::to_wstring(r.offset),r.receipt?format_message_timestamp(r.receipt,0,0,MessageClock::Local):ui::tr(L"Unavailable"),std::to_wstring(r.hits),r.text});
            DiscoveryListDialog list(this,L"Search results / frequency",{L"Byte offset",L"PC receipt time",L"Matches",L"Text"},std::move(rows),search_summary(found.results),L"Locate source",L"Frequency buckets");
            if(list.DoModal()==IDOK&&!closing_serial_){if(list.alternate()){
                    DiscoveryListDialog::Rows counts;if(found.results.timed){const double span=static_cast<double>(static_cast<long double>(found.results.end_ms)-found.results.begin_ms)/1000;for(size_t i=0;i<found.results.buckets.size();++i){const auto n=found.results.buckets[i];counts.push_back({std::to_wstring(i),number(span*i/64),number(span*(i+1)/64),std::to_wstring(n),std::wstring(std::min<size_t>(40,n),L'|')});}}
                    auto summary=ui::tr(found.results.timed?L"PC receipt frequency; offsets start at:":L"PC receipt time range unavailable; no time histogram is shown.");
                    if(found.results.timed)summary+=L" "+format_message_timestamp(found.results.begin_ms,0,0,MessageClock::Local)+L"\r\n"+ui::tr(L"64 equal intervals; counts are hits, not packets. Final end included; bars capped at40.");
                    if(found.results.skipped_rows||found.results.limited)summary+=L"\r\n"+search_summary(found.results);
                    DiscoveryListDialog frequency(this,L"Search frequency",{L"Bucket (0..63)",L"Start offset (s)",L"End offset (s)",L"Matches",L"Histogram"},std::move(counts),std::move(summary),L"Close");frequency.DoModal();}
                else {const auto& r=found.results.rows.at(list.selection());if(found.epoch!=message_page_.history_epoch()||found.base==UINT64_MAX||!message_page_.navigate_source(found.base+r.offset-found.offset,r.count))show_status_message(ui::tr(L"Source is unavailable: expired, cleared, paused or light-terminal display."),8000);else {TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();}}}}
    }
    RecordingLibrary library;if(discovery_library_.take(library,error)&&!discovery_close_pending_){
        if(!error.empty())show_status_message(error,8000);else {ModalFlag modal(discovery_modal_);DiscoveryListDialog::Rows rows;
            for(const auto& r:library.entries)rows.push_back({r.resource.name,r.tags,r.version,r.source,r.modified,r.resource.path,ui::tr(r.status)+L" | "+std::to_wstring(r.bytes)+L" bytes"});
            DiscoveryListDialog list(this,L"Recording library",{L"Name",L"Tags",L"Version",L"Source",L"File modified",L"Path",L"Status"},std::move(rows),L"Search names, tags, versions, sources and paths. Recent entries are suggestions until explicitly saved. No scan, raw copies, deletion or automatic playback.",L"Stage selected",L"Tags / save reference...");
            if(list.DoModal()==IDOK&&!closing_serial_){auto entry=library.entries.at(list.selection());if(list.alternate()){
                    UartToolForm tags(L"Recording reference tags",L"Save this private reference only. Name up to64 and tags up to128 characters; files are not renamed or changed.",{{L"Name",entry.resource.name},{L"Tags",entry.tags}},{},[](const auto& f,std::wstring& e){if(f[0].size()>64||f[1].size()>128){e=ui::tr(L"Name is limited to 64 and tags to 128 characters.");return false;}return true;},this);
                    if(tags.DoModal()==IDOK){entry.resource.name=tags.values()[0];entry.tags=tags.values()[1];const auto path=std::filesystem::path(settings_path_).replace_extension(L".scclibrary");if(!discovery_save_.start([path,library,entry](const std::atomic<bool>& cancel){save_recording_reference(path,library,entry,cancel);return 1;},discovery_thread_launcher_))show_status_message(ui::tr(L"Cannot start background operation; try again."),8000);}}
                else {remember_monitor_resource(entry.resource);MonitorCenter::Request request;request.action=MonitorCenter::Stage;request.resource=entry.resource;OnMonitorRequest(0,reinterpret_cast<LPARAM>(&request));}}}
    }
    if(discovery_close_pending_&&!discovery_search_.busy()&&!discovery_library_.busy()&&!discovery_save_.busy()){discovery_close_pending_=false;PostMessage(WM_CLOSE);}
}
void CMainDialog::discovery_palette(){
    if(discovery_modal_||closing_serial_)return;
    struct Entry {std::wstring name,category,aliases;std::function<void()> open;};std::vector<Entry> entries;
    const auto add=[&](const wchar_t* name,const wchar_t* category,const wchar_t* aliases,std::function<void()> action){entries.push_back({ui::tr(name),ui::tr(category),std::wstring(name)+L" "+aliases+L" "+ui::tr(aliases),std::move(action)});};
    for(const auto& page:std::vector<std::pair<const wchar_t*,int>>{{L"Messages",kMessagesTab},{L"Chart",kChartTab},{L"4 Charts",kDataTab}})add(page.first,L"Main views",L"",[this,page]{TabCtrl_SetCurSel(tabs_,page.second);switch_page();});
    add(L"Messages display lens",L"Messages",L"filter context",[this]{discovery_lens();});add(L"Search results / frequency",L"Messages",L"find histogram",[this]{discovery_search();});
    add(L"Linked time cursor",L"Charts",L"tracker",[this]{discovery_link();});add(L"Numeric watches",L"Monitor",L"alarm threshold",[this]{discovery_watches();});
    add(L"Recording library",L"Monitor",L"tags version",[this]{discovery_library();});add(L"Monitor events",L"Monitor",L"errors",[this]{show_monitor_center(true);});
    add(L"Console / Serial options...",L"UART",L"send settings",[this]{show_uart_console();});add(L"Workbench / Dual COM...",L"UART",L"binary packet inspector FFT compare",[this]{show_workbench();});
    const wchar_t* advanced[]={L"Replay",L"Profiles",L"Analysis",L"Automation",L"Extraction",L"Channels",L"File TX"};
    for(int i=0;i<AdvancedTools::PageCount;++i)add(advanced[i],L"Advanced UART tools",L"",[this,i]{show_advanced_tools(i);});
    const wchar_t* lab[]={L"Diagnostics",L"Recording / range export",L"Multi-parameter packet",L"Sequence / response statistics",L"Offline parser cases",L"Binary schemas / codecs",L"Digital state history",L"Recording sessions",L"COM device information",L"Numeric input framing",L"Custom CRC",L"Time derivative / integral",L"Passive transactions",L"Named chart views",L"Chart image export",L"Recording notes",L"HTML analysis report",L"FFT waterfall"};
    for(int i=0;i<UartLabWindow::PageCount;++i)add(lab[i],L"UART Laboratory",L"",[this,i]{show_uart_lab();lab_.select(i);});
    DiscoveryListDialog::Rows rows;for(const auto& e:entries)rows.push_back({e.name,e.category,e.aliases});
    size_t chosen=SIZE_MAX;{ModalFlag modal(discovery_modal_);DiscoveryListDialog list(this,L"Search functions",{L"Function",L"Location"},std::move(rows),L"Type to search. Enter opens the selected tool only; no Connect, TX, Apply, Play or recording. Esc closes.");if(list.DoModal()==IDOK)chosen=list.selection();}
    if(chosen<entries.size()&&!closing_serial_&&::IsWindow(m_hWnd))entries[chosen].open();
}
