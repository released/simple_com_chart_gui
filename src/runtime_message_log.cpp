#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "capture_filename.h"
#include "ui_metrics.h"
#include <iomanip>
#include <sstream>

namespace {
std::wstring shown(HWND h){std::wstring s(static_cast<size_t>(::GetWindowTextLengthW(h))+1,0);::GetWindowTextW(h,s.data(),static_cast<int>(s.size()));s.pop_back();return s;}
std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct Restore {std::function<void()> action;~Restore(){action();}};
}
bool CMainDialog::verify_message_logs(){
    const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"message-log-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool good){report<<(good?"PASS: ":"FAIL: ")<<name<<'\n';pass=good&&pass;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+7000;do{MSG m{};unsigned count=0;while(count++<64&&::PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){::TranslateMessage(&m);::DispatchMessageW(&m);}poll_recording();if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    const auto notice=uart_notice_;const auto capture_picker=capture_picker_;const auto recording_picker=recording_picker_;const bool demo=demo_;demo_=false;
    Restore restore{[&]{recorder_.stop();recorder_.wait_idle(5000);uart_notice_=notice;capture_picker_=capture_picker;recording_picker_=recording_picker;recording_interactive_=false;demo_=demo;}};
    try {
        std::vector<std::wstring> notices;uart_notice_=[&](HWND,LPCWSTR text,LPCWSTR,UINT){notices.emplace_back(text);return IDOK;};
        TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();message_page_.reset();
        MessageDisplayOptions display;display.clock=MessageClock::None;message_page_.configure_display(display);poll_recording();
        const auto save=message_page_.save_log_control(),start=message_page_.start_log_control(),stop=message_page_.stop_log_control();
        const auto click=[](HWND h){::SendMessageW(h,BM_CLICK,0,0);};
        check("Messages owns visible Save As/start/stop; global duplicate is hidden",::IsWindowVisible(save)&&::IsWindowVisible(start)&&::IsWindowVisible(stop)&&!::IsWindowVisible(btn_save_));
        check("idle recording controls are mutually enabled",::IsWindowEnabled(start)&&!::IsWindowEnabled(stop));
        const auto menu=message_page_.build_options_menu();bool translated=true;
        const auto walk=[&](auto&& self,HMENU current)->void{for(int i=0;i<GetMenuItemCount(current);++i){wchar_t text[512]{};MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_STRING|MIIM_SUBMENU|MIIM_FTYPE;item.dwTypeData=text;item.cch=512;GetMenuItemInfoW(current,i,TRUE,&item);if(!(item.fType&MFT_SEPARATOR)&&ui::active_language()=="tw"){const std::wstring value(text);bool chinese=value==L"LF"||value==L"CR"||value==L"CRLF";for(const auto c:value)chinese=chinese||(c>=0x3400&&c<=0x9fff);translated=translated&&chinese;}if(item.hSubMenu)self(self,item.hSubMenu);}};
        walk(walk,menu);DestroyMenu(menu);check("dynamic options and every nested menu use the selected language",translated);
        unsigned picks=0;std::wstring default_name;std::filesystem::path destination=folder/L"messages_033.log";
        capture_picker_=[&](const std::wstring& name,bool messages)->std::optional<std::filesystem::path>{++picks;default_name=name;check("message button requests message snapshot picker",messages);return destination;};
        click(save);check("empty Save As gives a visible localized explanation without creating files",notices.size()==1&&notices.back()==ui::tr(L"No messages to save yet. Receive or replay data first. To save upcoming data continuously, click Start recording.")&&picks==0&&!std::filesystem::exists(destination));
        message_page_.append(std::string(1536,'a'));message_page_.refresh();SYSTEMTIME before{},after{};GetLocalTime(&before);click(save);GetLocalTime(&after);
        check("Save As defaults to current local computer date and seconds",picks==1&&default_name>=capture_filename(before)&&default_name<=capture_filename(after));
        check("both Save As entry points lock while the shared worker is busy",exporting_&&!::IsWindowEnabled(save)&&!::IsWindowEnabled(btn_save_));
        check("Save As writes a real immutable log and unlocks",pump([&]{return !exporting_;})&&read(destination)==std::string(1536,'a')&&::IsWindowEnabled(save));
        auto info=shown(message_page_.log_info_control());check("saved filename and exact KB display omit the parent path",info.find(L"messages_033.log")!=info.npos&&info.find(L"1.50 KB")!=info.npos&&info.find(folder.wstring())==info.npos);
        const auto locked=folder/L"locked_033.log";{std::ofstream out(locked,std::ios::binary);out<<"keep";}
        HANDLE handle=CreateFileW(locked.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);destination=locked;const auto warnings=notices.size();click(save);
        const bool failed=pump([&]{return !exporting_;});CloseHandle(handle);
        check("interactive write failure is visible and preserves existing file and saved metadata",failed&&notices.size()==warnings+1&&read(locked)=="keep"&&shown(message_page_.log_info_control()).find(L"messages_033.log")!=std::wstring::npos);
        std::filesystem::path recording;std::wstring recording_default;
        recording_picker_=[&](const std::wstring& name,unsigned source)->std::optional<std::filesystem::path>{recording_default=name;check("Messages recording targets primary RX only",source==0);recording=folder/name;return recording;};
        GetLocalTime(&before);click(start);GetLocalTime(&after);
        check("recording default also uses the current computer timestamp",recording_default>=capture_filename(before)&&recording_default<=capture_filename(after));
        check("start immediately locks Start and enables Stop without opening COM",!::IsWindowEnabled(start)&&::IsWindowEnabled(stop)&&!serial_mgr_->is_connected());
        const std::string payload=std::string(300000,'z')+"\r\nrpm:201\r\n";recorder_.append(payload,message_unix_ms());
        message_page_.reset();check("recording continues independently of cleared retained messages",pump([&]{const auto s=recorder_.status();return s.bytes==payload.size()&&s.log_bytes>payload.size();})&&message_page_.buffer().bytes().empty());
        const auto actual=recorder_.status();const auto log=recording.parent_path()/actual.log_filename;const auto raw_folder=folder/L"log";std::wostringstream kb;kb.imbue(std::locale::classic());kb<<std::fixed<<std::setprecision(2)<<static_cast<double>(actual.log_bytes)/1024<<L" KB";
        info=shown(message_page_.log_info_control());check("recording filename and KB match the actual readable file",!actual.log_filename.empty()&&actual.log_bytes==std::filesystem::file_size(log)&&info.find(actual.log_filename)!=info.npos&&info.find(kb.str())!=info.npos&&info.find(folder.wstring())==info.npos);
        auto stopping=actual;stopping.stopping=true;message_page_.set_recording_status(stopping,true);check("both recording buttons gray during stop drain",!::IsWindowEnabled(start)&&!::IsWindowEnabled(stop));
        poll_recording();click(stop);check("Stop drains exact RX data and restores idle controls",pump([&]{return !recorder_.status().active&&::IsWindowEnabled(start)&&!::IsWindowEnabled(stop);})&&read(raw_folder/(recording.stem().wstring()+L"_1.bin"))==payload);
        check("GUI creates the private raw folder and keeps bin/index/metadata out of the chosen readable-log folder",std::filesystem::exists(raw_folder/(recording.stem().wstring()+L"_1.idx"))&&std::filesystem::exists(raw_folder/(recording.stem().wstring()+L".sccmeta"))&&!std::filesystem::exists(recording.parent_path()/(recording.stem().wstring()+L"_1.bin")));
        message_page_.append("UART Messages log controls\r\nrpm:201,temp:25\r\n");message_page_.refresh();
        SetWindowPos(nullptr,0,0,1500,900,SWP_NOMOVE|SWP_NOZORDER);::RedrawWindow(m_hWnd,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
        check("localized Messages toolbar screenshot",save_runtime_frame(m_hWnd,folder/L"message-log-toolbar.png"));
        for(const int width:{760,1120,1500}){
            SetWindowPos(nullptr,0,0,width,780,SWP_NOMOVE|SWP_NOZORDER);RECT page{},previous{};::GetWindowRect(message_page_.hwnd(),&page);bool fit=true;
            for(const auto h:{save,start,stop}){RECT b{},c{};::GetWindowRect(h,&b);::GetClientRect(h,&c);fit=fit&&b.left>=page.left&&b.right<=page.right&&b.top>=page.top&&b.bottom<=page.bottom&&c.right>=control_text_width(h,20);if(previous.right)fit=fit&&(b.top>previous.top||b.left>=previous.right);previous=b;}
            RECT label{},receiver{};::GetWindowRect(message_page_.log_info_control(),&label);::GetWindowRect(message_page_.view(),&receiver);check("wrapped toolbar, filename summary and receive view stay inside the page",fit&&label.right<=page.right&&label.bottom<=page.bottom&&receiver.bottom<=page.bottom&&receiver.top>=label.bottom&&receiver.bottom-receiver.top>=40);
        }
        const auto saved_log=read(log);recording_picker_=[&](const std::wstring&,unsigned)->std::optional<std::filesystem::path>{return recording;};const auto errors=notices.size();click(start);
        check("existing recording is never overwritten and failure is visible",pump([&]{return !recorder_.status().active&&notices.size()>=errors+1&&::IsWindowEnabled(start)&&!::IsWindowEnabled(stop);})&&notices.size()==errors+1&&read(log)==saved_log);
        check("affected existing immutable ASCII/HEX export regression",verify_message_export());
        if(ui::active_language()=="en")check("affected existing message-font alignment and persistence",verify_message_font());
    }catch(const std::exception& e){report<<"FAIL: "<<e.what()<<'\n';pass=false;}
    return pass;
}
