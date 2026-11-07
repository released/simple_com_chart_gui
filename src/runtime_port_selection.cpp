#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include <chrono>
#include <fstream>
#include <thread>

bool CMainDialog::verify_port_selection() {
    const auto evidence=std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence/L"port-selection-result.txt");
    if(!report)return false;
    bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';report.flush();pass=pass&&ok;};
    const auto path=settings_path_;const auto ports=known_ports_;const auto hint=selection_hint_;
    const auto remembered=remembered_serial_;const auto scanner=port_scanner_;const auto factory=transport_factory_;
    const auto notice=connection_notice_;const bool demo=demo_;
    disconnect();demo_=false;
    auto transport=std::move(serial_mgr_);auto fake=std::make_shared<FakeSerialState>();
    const SerialPortInfo first{L"COM3",L"First fixture",L"USB\\FIRST",L"fake://first"};
    const SerialPortInfo target{L"COM4",L"Saved fixture",L"USB\\SAVED",L"fake://saved"};
    auto renamed=target;renamed.device=L"COM9";
    fake->ports={first,target};
    port_scanner_=[fake](bool* ok){std::lock_guard<std::mutex> lock(fake->mutex);*ok=fake->scan_ok;return fake->ports;};
    transport_factory_=[fake]{return std::make_unique<FakeSerialTransport>(fake);};
    serial_mgr_=std::make_unique<FakeSerialTransport>(fake);
    std::wstring warning;
    connection_notice_=[&](HWND,LPCWSTR text,LPCWSTR,UINT){warning=text;return IDOK;};
    settings_path_=(evidence/L"port-selection-test.ini").wstring();
    const auto selected=[&]{return static_cast<int>(::SendMessageW(combo_port_,CB_GETCURSEL,0,0));};
    const auto caption=[&]{wchar_t value[512]{};::GetWindowTextW(combo_port_,value,512);return std::wstring(value);};
    const auto shown=[&](int index,const SerialPortInfo& port){return selected()==index&&caption()==port.device+L" - "+port.description;};
    const auto catalog=[&](std::vector<SerialPortInfo> value){std::lock_guard<std::mutex> lock(fake->mutex);fake->ports=std::move(value);fake->scan_ok=true;};
    const auto load=[&](const SerialPortInfo& value){
        connection_.cancel();known_ports_.clear();::SendMessageW(combo_port_,CB_RESETCONTENT,0,0);
        WritePrivateProfileStringW(L"Serial",L"Port",value.device.c_str(),settings_path_.c_str());
        WritePrivateProfileStringW(L"Serial",L"DeviceInstanceId",value.instance_id.c_str(),settings_path_.c_str());
        load_serial_settings();scan_ports();
    };
    load(target);
    check("available saved device remains selected instead of first port",shown(1,target));
    catalog({first,renamed});load(target);
    check("saved instance follows a changed COM number",shown(1,renamed));
    const SerialPortInfo missing{L"COM77",L"Missing fixture",L"USB\\MISSING",L""};
    catalog({first,target});load(missing);
    check("stale INI selects first scanned port and visible native text",shown(0,first));
    check("fallback updates selection hint without replacing remembered successful device",selection_hint_.instance_id==first.instance_id&&remembered_serial_.port.instance_id==missing.instance_id);
    wchar_t prior[32]{};GetPrivateProfileStringW(L"Serial",L"Port",L"",prior,32,settings_path_.c_str());
    check("preselection does not overwrite the saved INI port",std::wstring(prior)==missing.device);
    check("scanning and INI restoration never open a port",fake->attempt_count()==0);
    auto reused=first;reused.device=target.device;
    catalog({renamed,reused});load(missing);
    check("first scanned order is retained rather than sorting the fallback",shown(0,renamed));
    catalog({reused});load(target);
    check("idle saved identity missing on a reused COM number still shows available first port",shown(0,reused));
    catalog({first,target});load(missing);
    ::SendMessageW(combo_port_,CB_SETCURSEL,1,0);scan_ports();
    check("rescan preserves an explicit available selection",shown(1,target));
    {std::lock_guard<std::mutex> lock(fake->mutex);fake->scan_ok=false;}
    scan_ports();check("failed enumeration preserves existing selection and text",shown(1,target));
    catalog({});scan_ports();
    check("empty successful scan leaves no fabricated COM choice",selected()==CB_ERR&&caption().empty());
    check("connect with no scanned ports reports error without opening",!connect_with_validation()&&fake->attempt_count()==0&&warning.find(L"No available COM port selected")!=std::wstring::npos);
    catalog({first,target});load({});
    check("missing stored port selects first without an automatic open",shown(0,first)&&fake->attempt_count()==0);
    using Mode=SerialConnectionState::Mode;
    SerialSettings active;active.port=target;
    for(const auto mode:{Mode::Opening,Mode::Connected,Mode::Waiting,Mode::Reopening}){
        connection_.begin(active,mode==Mode::Reopening);
        if(mode==Mode::Connected||mode==Mode::Waiting)connection_.connected(target);
        if(mode==Mode::Waiting)connection_.lost(0);
        update_port_combo({first});
        check("non-idle missing active device never selects another first port",selected()==CB_ERR&&caption().empty()&&connection_.active().port.instance_id==target.instance_id);
    }
    connection_.begin(active,false);connection_.connected(target);connection_.lost(0);update_port_combo({first,renamed});
    check("recovery selection can still find the same identity after renumbering",shown(1,renamed)&&fake->attempt_count()==0);
    connection_.cancel();catalog({first,target});load(missing);
    OnCommand(MAKEWPARAM(103,BN_CLICKED),0);
    const auto pump=[&](const std::function<bool()>& ready){
        const auto end=GetTickCount64()+3000;
        while(GetTickCount64()<end){poll_serial_connection();if(ready())return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        return ready();
    };
    check("Connect works directly after fallback without dropdown interaction",pump([&]{return connection_.mode()==Mode::Connected;}));
    {std::lock_guard<std::mutex> lock(fake->mutex);check("manual connection uses exactly first enumerated fake device",fake->attempts.size()==1&&connection_.active().port.instance_id==first.instance_id&&fake->attempts.front().port.open_path==first.open_path);}
    wchar_t saved[32]{};GetPrivateProfileStringW(L"Serial",L"Port",L"",saved,32,settings_path_.c_str());
    check("only successful manual connect persists the fallback port",std::wstring(saved)==first.device);
    disconnect();fake->release();
    const bool stopped=pump([&]{return !disconnect_pending_&&!serial_open_.busy();});
    check("fake workers stop before restoring fixture state",stopped);
    if(!stopped){connection_notice_=notice;return false;}
    serial_mgr_=std::move(transport);port_scanner_=scanner;transport_factory_=factory;connection_notice_=notice;
    settings_path_=path;remembered_serial_=remembered;selection_hint_=hint;
    ::SendMessageW(combo_port_,CB_SETCURSEL,static_cast<WPARAM>(-1),0);update_port_combo(ports);load_serial_settings();
    demo_=demo;update_connection_ui();
    report<<(pass?"PASS: ":"FAIL: ")<<"port-selection-only native checks; no physical COM opens\n";
    return pass;
}
