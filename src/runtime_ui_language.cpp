#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
namespace {
std::wstring shown(HWND h){std::wstring s(static_cast<size_t>(::GetWindowTextLengthW(h))+1,0);::GetWindowTextW(h,s.data(),static_cast<int>(s.size()));s.pop_back();return s;}
struct Writes {std::atomic<unsigned> count{0};};
class LanguageTransport:public FakeSerialTransport {
public:LanguageTransport(std::shared_ptr<FakeSerialState> state,std::shared_ptr<Writes> writes):FakeSerialTransport(std::move(state)),writes_(std::move(writes)){}
    size_t write_bytes(std::string_view bytes,std::wstring*)override{++writes_->count;return bytes.size();}
private:std::shared_ptr<Writes> writes_;
};
}
bool CMainDialog::verify_ui_language(){
    demo_=false;const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"language-result.txt");report<<std::unitbuf;bool pass=true;const bool chinese=ui::active_language()=="tw";
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto pump=[&](auto ready){const auto until=GetTickCount64()+7000;do{MSG m{};unsigned n=0;while(n++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}if(ready())return true;Sleep(2);}while(GetTickCount64()<until);return false;};
    check("selected UTF-8 catalog loaded without fallback warning",ui::language_notice().empty());
    check("actual main controls use selected language",shown(btn_scan_)==(chinese?L"\u6383\u63cf":L"Scan")&&shown(btn_save_)==(chinese?L"\u53e6\u5b58\u65b0\u6a94\u2026":L"Save As...")&&shown(label_include_)==(chinese?L"\u5305\u542b":L"Include"));
    wchar_t tab_text[64]{};TCITEMW tab{};tab.mask=TCIF_TEXT;tab.pszText=tab_text;tab.cchTextMax=64;TabCtrl_GetItem(tabs_,0,&tab);check("actual Messages tab translated without changing its index",std::wstring(tab_text)==(chinese?L"\u8a0a\u606f":L"Messages"));
    const auto label_fits=[&](HWND h){CRect rect;::GetWindowRect(h,&rect);CClientDC dc(CWnd::FromHandle(h));const auto old=dc.SelectObject(CFont::FromHandle(reinterpret_cast<HFONT>(::SendMessageW(h,WM_GETFONT,0,0))));const auto text=shown(h);const auto width=dc.GetTextExtent(text.c_str()).cx;dc.SelectObject(old);return rect.Width()>=width;};
    check("main engineering toolbar labels have measured widths",label_fits(label_include_)&&label_fits(label_auto_)&&label_fits(label_time_));
    SendMessage(WM_COMMAND,ID_UART_LAB);lab_.SetWindowPos(nullptr,0,0,760,600,SWP_NOMOVE|SWP_NOZORDER);
    for(int page=0;page<UartLabWindow::PageCount;++page){lab_.select(page);bool values=true,labels=true;for(const auto& f:lab_.fields_[page]){labels=labels&&label_fits(f.label);if(!f.choices.empty()){const auto index=::SendMessageW(f.input,CB_GETCURSEL,0,0);values=values&&index>=0&&ui::control_value(f.input)==f.choices[static_cast<size_t>(index)];}}lab_.OnVScroll(SB_BOTTOM,0,nullptr);check(("native Laboratory page "+std::to_string(page)+" labels fit and choices remain canonical").c_str(),labels&&values);}
    lab_.select(UartLabWindow::Input);lab_.field(UartLabWindow::Input,0,L"Port B");lab_.field(UartLabWindow::Input,1,L"CRLF");check("translated source choice reads exact Port B backend token",lab_.values(UartLabWindow::Input)[0]==L"Port B"&&shown(lab_.fields_[UartLabWindow::Input][0].input)==(chinese?L"\u9023\u63a5\u57e0 B":L"Port B"));
    show_advanced_tools(AdvancedTools::Extraction);::SendMessageW(advanced_tools_.field_hwnd(AdvancedTools::Extraction,0),CB_SETCURSEL,3,0);advanced_tools_.set_field(AdvancedTools::Extraction,1,L"ptr:rpm=/fan/rpm");check("Advanced options keep parser/path data canonical",advanced_tools_.field(AdvancedTools::Extraction,0)==L"JSON"&&advanced_tools_.field(AdvancedTools::Extraction,1)==L"ptr:rpm=/fan/rpm");advanced_tools_.ShowWindow(SW_HIDE);
    show_workbench();check("Workbench static options retain backend values",ui::control_value(workbench_->groups_[0].view)==L"ASCII");workbench_->ShowWindow(SW_HIDE);lab_.ShowWindow(SW_HIDE);
    auto state=std::make_shared<FakeSerialState>();auto writes=std::make_shared<Writes>();const std::vector<SerialPortInfo> ports{{L"COM161",L"Language fixture",L"LANGUAGE_FAKE",L"fake-language"}};
    port_scanner_=[ports](bool* ok){if(ok)*ok=true;return ports;};transport_factory_=[state,writes]{return std::make_unique<LanguageTransport>(state,writes);};uart_notice_=[](HWND,LPCWSTR,LPCWSTR,UINT){return IDOK;};scan_ports();::SendMessageW(combo_port_,CB_SETCURSEL,0,0);on_connect_toggle();check("language has no effect on explicit fake COM connection",pump([&]{return serial_mgr_->is_connected();}));
    const std::string raw="Connect\r\nPort A\r\nvalue:201\r\n";{std::lock_guard<std::mutex> lock(state->mutex);state->raw=raw;}
    check("raw RX English-looking data and numeric keys stay unchanged",pump([&]{double v=0;return model_.latest_value("value",v)&&v==201;})&&message_page_.buffer().bytes()==raw&&model_.get_enabled_count()==0);
    const auto rows=capture_.rows().size();const auto bytes=message_page_.buffer().bytes();const auto before=ui::active_language();SendMessage(WM_COMMAND,chinese?ID_LANGUAGE_EN:ID_LANGUAGE_TW);
    check("language selection saves without reconnect or TX",ui::saved_language(settings_path_)==(chinese?"en":"tw")&&ui::active_language()==before&&serial_mgr_->is_connected()&&state->attempt_count()==1&&writes->count==0);
    check("current capture and raw data survive next-launch selection",capture_.rows().size()==rows&&message_page_.buffer().bytes()==bytes&&shown(btn_scan_)==(chinese?L"\u6383\u63cf":L"Scan"));
    TabCtrl_SetCurSel(tabs_,kMessagesTab);switch_page();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("localized main screenshot",save_runtime_frame(m_hWnd,folder/L"language-main.png"));
    lab_.ShowWindow(SW_SHOW);lab_.select(UartLabWindow::Input);lab_.SetWindowPos(nullptr,0,0,1040,820,SWP_NOMOVE|SWP_NOZORDER);lab_.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);check("localized settings screenshot",save_runtime_frame(lab_.m_hWnd,folder/L"language-options.png"));lab_.ShowWindow(SW_HIDE);
    disconnect();check("fake workers drain after language checks",pump([&]{return !disconnect_pending_&&!serial_mgr_->is_connected();}));workbench_->shutdown();check("owned Workbench drains before fixture exit",pump([&]{return workbench_->shutdown_complete();}));return pass;
}
