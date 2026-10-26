#include "finite_script.h"
#include "uart_files.h"
#include "message_format.h"
#include "build_version.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
std::atomic<bool> cancelled{false};
BOOL WINAPI control(DWORD code){if(code==CTRL_C_EVENT||code==CTRL_BREAK_EVENT||code==CTRL_CLOSE_EVENT){cancelled=true;return TRUE;}return FALSE;}
struct Guard {
    HANDLE handle=nullptr;bool owned=false;
    ~Guard(){if(owned)ReleaseMutex(handle);if(handle)CloseHandle(handle);}
    void acquire(){handle=CreateMutexW(nullptr,FALSE,L"Local\\Released.SimpleComChart.UARTChart.v1.Owner");if(!handle)throw std::runtime_error("Cannot acquire GUI/CLI session guard.");const auto r=WaitForSingleObject(handle,0);owned=r==WAIT_OBJECT_0||r==WAIT_ABANDONED;if(!owned)throw std::runtime_error("Simple COM Chart GUI/CLI already owns this session. No port opened.");}
};
class Report {
public:
    explicit Report(const std::filesystem::path& path){handle_=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(handle_==INVALID_HANDLE_VALUE)throw std::runtime_error("Report must be a NEW writable filename; no port opened.");}
    ~Report(){CloseHandle(handle_);}
    void save(std::string_view text){while(!text.empty()){DWORD n=0;const auto size=static_cast<DWORD>(std::min<size_t>(65536,text.size()));if(!WriteFile(handle_,text.data(),size,&n,nullptr)||!n)throw std::runtime_error("Report write failed. In-flight bytes cannot be undone; no retry.");text.remove_prefix(n);}if(!FlushFileBuffers(handle_))throw std::runtime_error("Report flush failed; no retry.");}
private:HANDLE handle_=INVALID_HANDLE_VALUE;
};
unsigned integer(const std::wstring& s,unsigned low,unsigned high){double n=0;if(!uart_number(s,low,high,n)||n!=static_cast<unsigned>(n))throw std::runtime_error("Invalid integer option.");return static_cast<unsigned>(n);}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==1||(argc==2&&std::wstring(argv[1])==L"--help")) {
            std::cout<<"SimpleComChartCli " CHART_VERSION_STR "\n"
                "validate --script <private.sccsteps>\n"
                "run --script <private.sccsteps> --port COMn --baud 115200 --report <NEW.csv> --confirm-tx\n"
                "Optional: --data 5..8 --parity NONE|ODD|EVEN --stop 1|1.5|2 --max-ms 100..3600000\n"
                "Default 8/N/1, no flow control, DTR/RTS enabled (opening may reset a target), max 60000 ms.\n"
                "Explicit finite TX only; no reconnect, retry, personal INI or GUI automation. Ctrl+C cancels.\n"
                "CSV reports PC step timing/driver acceptance/raw matches, not wire timing or protocol ACK.\n";return 0;
        }
        const std::wstring mode=argv[1];if(mode!=L"run"&&mode!=L"validate")throw std::runtime_error("Use --help, validate, or run.");
        std::map<std::wstring,std::wstring> options;bool confirmed=false;
        const std::vector<std::wstring> allowed{L"--script",L"--port",L"--baud",L"--report",L"--data",L"--parity",L"--stop",L"--max-ms"};
        for(int i=2;i<argc;++i){const std::wstring key=argv[i];if(key==L"--confirm-tx"){if(confirmed)throw std::runtime_error("Duplicate confirmation.");confirmed=true;continue;}if(std::find(allowed.begin(),allowed.end(),key)==allowed.end()||i+1==argc||!options.emplace(key,argv[++i]).second)throw std::runtime_error("Unknown/duplicate/missing option.");}
        if(!options.count(L"--script"))throw std::runtime_error("--script is required.");
        const auto encoded=uart_read_file(options[L"--script"],128*1024,cancelled);std::vector<ScriptStep> steps;std::string error;if(!decode_script(encoded,steps,error))throw std::runtime_error(error);
        if(mode==L"validate"){if(options.size()!=1||confirmed)throw std::runtime_error("validate accepts --script only; no hardware options.");std::cout<<"VALID: "<<steps.size()<<" finite steps; no port opened.\n";return 0;}
        if(!confirmed||!options.count(L"--port")||!options.count(L"--baud")||!options.count(L"--report"))throw std::runtime_error("run requires --port, --baud, --report and --confirm-tx.");
        const auto get=[&](const wchar_t* key,const wchar_t* fallback){const auto it=options.find(key);return it==options.end()?std::wstring(fallback):it->second;};
        SerialSettings settings;settings.baud=static_cast<int>(integer(options[L"--baud"],1,4000000));settings.data_bits=static_cast<int>(integer(get(L"--data",L"8"),5,8));const auto parity=get(L"--parity",L"NONE"),stop=get(L"--stop",L"1");
        if(parity!=L"NONE"&&parity!=L"ODD"&&parity!=L"EVEN")throw std::runtime_error("Invalid parity.");if(stop!=L"1"&&stop!=L"1.5"&&stop!=L"2")throw std::runtime_error("Invalid stop bits.");
        settings.parity=parity==L"ODD"?ODDPARITY:parity==L"EVEN"?EVENPARITY:NOPARITY;settings.stop_bits=stop==L"1.5"?ONE5STOPBITS:stop==L"2"?TWOSTOPBITS:ONESTOPBIT;
        const auto maximum=integer(get(L"--max-ms",L"60000"),100,3600000);
        Guard guard;guard.acquire();bool scanned=false;const auto ports=SerialManager::scan_ports(&scanned);size_t matches=0;
        for(const auto& port:ports)if(same_serial_name(port.device,options[L"--port"])) {settings.port=port;++matches;}
        if(!scanned||matches!=1||!valid_serial_settings(settings))throw std::runtime_error("Selected COM port/settings are unavailable or ambiguous; no port opened.");
        Report report(options[L"--report"]);SetConsoleCtrlHandler(control,TRUE);SerialManager transport;std::wstring detail;
        if(cancelled||!transport.open(settings,&detail)){report.save("Status,Detail\r\nOPEN_FAILED,"+uart_csv_cell(message_utf8(detail))+"\r\n");std::cerr<<"Port open failed; check other terminal owners, cable, driver and settings.\n";return 3;}
        const auto result=execute_finite_script(transport,std::move(steps),cancelled,maximum);transport.disconnect();report.save(result.csv);std::cout<<result.status<<'\n';return result.passed?0:3;
    }catch(const std::exception& e){std::cerr<<"ERROR: "<<e.what()<<'\n';return 2;}
}
