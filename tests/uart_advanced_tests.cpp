#include "uart_offline.h"
#include "uart_profiles.h"
#include "uart_automation.h"
#include "uart_preferences.h"
#include "chart_analysis.h"
#include "background_job.h"
#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
namespace {
void check(bool result,const char* name) { if (!result) throw std::runtime_error(name); std::cout<<"PASS: "<<name<<'\n'; }
template<class F> bool rejects(F work) { try { work(); return false; } catch (...) { return true; } }
}
int wmain(int argc,wchar_t** argv) {
    try {
        if (argc!=2) throw std::runtime_error("Expected test-owned evidence directory");
        const auto folder=std::filesystem::path(argv[1])/std::to_wstring(GetCurrentProcessId()); std::filesystem::create_directories(folder); std::atomic<bool> cancel{false};
        const auto text=folder/L"input.log"; uart_write_file(text,"a:1\r\na:2\na:3",false,cancel);
        auto data=load_replay(text,100,cancel);
        check(!data->receipt_times && data->records.size()==3 && data->records[2].seconds==0.2,"plain file uses explicit synthetic timing and preserves final unterminated line");
        ReplayClock clock; clock.load(data); std::string bytes;
        check(!clock.playing() && clock.due(0)==0,"offline load never starts automatically");
        clock.play(100,2); check(clock.due(100)==1 && clock.due(149)==0 && clock.due(150)==1,"replay speed uses virtual file time");
        clock.pause(160); check(clock.due(99999)==0,"pause does not advance position");
        clock.seek(1,100000); clock.play(100000,1); check(clock.due(100000)==1 && clock.position()==2,"seek starts selected record without old wall-time catchup");
        check(clock.due(100100)==1 && !clock.playing(),"replay ends without looping");
        const auto binary=folder/L"raw.bin", index=folder/L"raw.idx";
        uart_write_file(binary,std::string("a:1\n\0a:2\n",9),false,cancel);
        uart_write_file(index,"SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,4,1000,RX\r\n4,0,1010,TX metadata\r\n4,5,1250,RX\r\n",false,cancel);
        data=load_replay(binary,20,cancel);
        check(data->receipt_times && data->records.size()==2 && data->records[1].seconds==0.25 && data->read_record(1,cancel)[0]=='\0',"recording index replays exact RX bytes and receipt intervals; metadata is not RX");
        uart_write_file(index,"SCC-RX-INDEX,1\noffset,length,unix_ms,event\n0,99,1000,RX\n",true,cancel);
        check(rejects([&]{load_replay(binary,20,cancel);}),"mismatched/out-of-bounds index rejected rather than silently using fake time");
        cancel=true; check(rejects([&]{load_replay(text,100,cancel);}),"background import cancellation"); cancel=false;
        check(rejects([&]{uart_read_file(text,2,cancel);}) && rejects([&]{uart_read_file(folder,100,cancel);}),"size cap and non-file inputs rejected");
        check(rejects([&]{uart_write_file(text,"overwritten",false,cancel);}) && uart_read_file(text,100,cancel)=="a:1\r\na:2\na:3","create-new output collision preserves existing data");
        UartProfile p; p.include=L"fan|ac"; p.regex=true; p.charts[1]="fg_rpm"; p.serial.port.device=L"COM9"; p.serial.port.instance_id=L"USB\\fixture"; p.serial.controls.flow=SerialFlow::RtsCts; p.display.encoding=MessageEncoding::Utf8; p.wrap=true;
        const auto encoded=encode_profile(p); const auto restored=decode_profile(encoded);
        check(restored.include==p.include && restored.charts==p.charts && restored.serial.controls==p.serial.controls && restored.wrap,"portable profile roundtrip across supported fields");
        check(rejects([&]{decode_profile(encoded+"Unknown=1\n");}) && rejects([&]{decode_profile(encoded+"Baud=1\n");}) && rejects([&]{decode_profile("SimpleComChartProfile=99\n");}),"profile unknown/duplicate/version rejection");
        check(valid_profile_name(L"Lab_UART-1") && !valid_profile_name(L"..\\escape") && !valid_profile_name(L"CON") && !valid_profile_name(L"com1") && !valid_profile_name(L"name."),"named profile path/device-name validation");
        p.serial.port.open_path=L"\\\\.\\COM9"; std::wstring error; check(!validate_profile(p,error),"profile cannot provide a hardware-open path"); p.serial.port.open_path.clear();
        const auto ini=(folder/L"SimpleComChart.ini").wstring();
        const unsigned char bom[]={255,254}; uart_write_file(ini,std::string(reinterpret_cast<const char*>(bom),2),false,cancel);
        WritePrivateProfileStringW(L"Private",L"Keep",L"original",ini.c_str()); write_profile_ini(ini,p,cancel);
        wchar_t kept[64]{}; GetPrivateProfileStringW(L"Private",L"Keep",L"",kept,64,ini.c_str());
        check(std::wstring(kept)==L"original" && load_uart_controls(ini,error)==p.serial.controls,"profile apply preserves unrelated INI data and writes grouped controls");
        const auto saved=uart_read_file(ini,100000,cancel); cancel=true; check(rejects([&]{write_profile_ini(ini,p,cancel);}),"cancelled profile commit rejected"); cancel=false;
        check(uart_read_file(ini,100000,cancel)==saved,"cancelled profile leaves original INI intact");
        SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_READONLY);
        const bool read_only_rejected=rejects([&]{write_profile_ini(ini,p,cancel);}); SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_NORMAL);
        check(read_only_rejected && uart_read_file(ini,100000,cancel)==saved,"read-only profile apply preserves the destination");
        p.include=L"line\n[Injected]"; check(!validate_profile(p,error),"profile rejects INI newline injection"); p.include=L"fan|ac";
        CaptureStore capture; capture.append("a:1",{{"a",1}},10); capture.append("other:5",{{"other",5}},11); capture.append("a:3",{{"a",3}},12);
        const auto analysis=analyze_capture(capture.snapshot(),0,0,2);
        check(analysis.count==2 && analysis.minimum==1 && analysis.maximum==3 && analysis.mean==2 && analysis.delta==2 && std::abs(analysis.rms-std::sqrt(5))<1e-9 && analysis.stddev==1,"inclusive interval statistics skip sparse blanks, snap cursors and preserve decimals");
        check(analyze_capture(capture.snapshot(),0,5,6).count==0,"empty interval is identified, not filled with zero samples");
        CaptureStore extremes; extremes.append("x",{{"x",1e300}},1); extremes.append("x",{{"x",-1e300}},2);
        check(std::isfinite(analyze_capture(extremes.snapshot(),0,0,1).rms),"scaled RMS avoids overflow from finite large values");
        check(automation_payload({L"123456789",false},TxChecksum::ModbusCrc16,bytes,error) && uart_hex(bytes.substr(9))=="374B","CRC16 Modbus known vector, little-endian append");
        check(automation_payload({L"010203",true,TxEnding::CRLF},TxChecksum::Sum8,bytes,error) && uart_hex(bytes)=="010203060D0A","Sum8 covers payload before ending");
        check(automation_payload({L"010203",true},TxChecksum::Xor8,bytes,error) && uart_hex(bytes)=="01020300","XOR8 checksum");
        check(!automation_payload({L"01",true,static_cast<TxEnding>(99)},TxChecksum::None,bytes,error),"invalid ending cannot bypass automation payload validation");
        AutomationConfig config; config.commands={"one","two"}; config.mode=AutomationConfig::Mode::Batch; config.count=2; config.interval_ms=100; config.wait_reply="OK"; config.timeout_ms=100; config.retries=1;
        UartAutomation automation; check(automation.start(config,0,error) && automation.next(0,bytes) && bytes=="one","explicit automation starts bounded batch");
        automation.receive("O"); automation.receive("K"); automation.complete(true,10); check(!automation.next(109,bytes) && automation.next(110,bytes) && bytes=="two","split early reply survives TX completion and respects interval");
        automation.complete(true,120); check(!automation.next(219,bytes) && automation.next(220,bytes) && bytes=="two","explicit timeout retry resends same step only up to configured limit");
        automation.complete(true,230); check(!automation.next(330,bytes) && !automation.active(),"timeout limit stops automation without unbounded retries");
        config.commands={"reply"}; config.mode=AutomationConfig::Mode::AutoReply; config.trigger="READY"; config.wait_reply.clear(); config.count=2;
        check(automation.start(config,1000,error) && !automation.next(1000,bytes),"auto reply requires explicit arming and a future trigger");
        automation.receive("REA"); automation.receive("DY"); check(automation.next(1001,bytes),"auto-reply trigger crosses read boundaries");
        automation.complete(false,1002); check(!automation.active() && !automation.next(9999,bytes),"partial/failed driver TX never automatically retries");
        config.mode=AutomationConfig::Mode::Periodic; config.wait_reply.clear(); config.count=1;
        check(automation.start(config,2000,error) && automation.next(2000,bytes),"periodic explicit run"); automation.complete(true,2001); check(!automation.active(),"finite configured count stops");
        config.count=2; automation.start(config,3000,error); automation.stop(L"disconnect"); automation.receive("READY"); check(!automation.next(9000,bytes),"disconnect stop never resumes from RX or time");
        config.retries=4; check(!automation.start(config,0,error),"unbounded retry configuration rejected");
        SerialOperationQueue queue; queue.begin(); SerialOperation op; op.bytes="cancel me"; uint64_t id=0; queue.submit(op,id,error); check(queue.cancel_pending(id) && queue.queued()==0 && queue.results().front().cancelled,"stop cancels a queued operation without cancelling unrelated transport state");
        BackgroundJob<int> job; check(job.start([](const std::atomic<bool>&){return 7;}),"background job starts"); int value=0;
        for (int tries=0;tries<1000 && job.busy();++tries) { job.take(value,error); Sleep(1); }
        check(!job.busy() && value==7 && error.empty(),"background completion is polled and joined only after OS thread signal");
        UartProfile workspace=p;workspace.workspace=true;for(const auto& key:workspace_keys())workspace.workspace_settings[key]="";workspace.workspace_settings["DualCOM.B/Port"]="COM72";workspace.workspace_settings["DualCOM.B/Baud"]="115200";
        const auto workspace_encoded=encode_profile(workspace);const auto workspace_copy=decode_profile(workspace_encoded);check(workspace_copy.workspace&&workspace_copy.workspace_settings.at("DualCOM.B/Port")=="COM72","v2 complete workspace round trip");
        write_profile_ini(ini,workspace_copy,cancel);check(read_workspace_settings(ini).at("DualCOM.B/Port")=="COM72","workspace atomically applies allowed B settings");
        workspace.workspace_settings["Private/Secret"]="never";check(rejects([&]{encode_profile(workspace);}),"unknown/private workspace keys rejected");workspace.workspace_settings.erase("Private/Secret");workspace.workspace_settings["Workbench.TX.A/Configuration"]="v1:99999:0";check(rejects([&]{encode_profile(workspace);}),"invalid workspace pacing rejected");
        return 0;
    } catch (const std::exception& ex) { std::cerr<<"FAIL: "<<ex.what()<<'\n'; return 1; }
}
