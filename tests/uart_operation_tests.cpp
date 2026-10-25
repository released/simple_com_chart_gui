#include "uart_operations.h"
#include "uart_preferences.h"
#include "serial_control_config.h"
#include "serial_open_job.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace { void check(bool value, const char* label) { if (!value) throw std::runtime_error(label); std::cout << "PASS: " << label << '\n'; } }
namespace {
class OpenControlsFixture : public SerialTransport {
public:
    explicit OpenControlsFixture(std::shared_ptr<SerialSettings> seen) : seen_(std::move(seen)) {}
    bool is_connected() const override { return true; }
    bool connect(const std::wstring&,int,int,int,int,std::wstring*) override { return false; }
    bool open(const SerialSettings& settings,std::wstring*) override { *seen_ = settings; return true; }
    void disconnect() override {}
    std::vector<std::string> read_lines(std::wstring*,std::string*) override { return {}; }
    int consume_rx_overflow() override { return 0; }
private:
    std::shared_ptr<SerialSettings> seen_;
};
}
void test_uart_native_io();
int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Expected isolated fixture folder");
        const auto folder = std::filesystem::path(argv[1]) / std::to_wstring(GetCurrentProcessId()); std::filesystem::create_directories(folder);
        const auto ini = (folder / L"console.ini").wstring();
        std::string bytes; std::wstring error;
        check(load_uart_controls(ini,error) == SerialControls{} && error.empty(), "missing INI uses legacy-compatible defaults without opening hardware");
        load_uart_controls(folder.wstring(),error); check(!error.empty(), "unreadable INI path reports error instead of silently selecting control pins");
        check(make_tx_payload({L"help", false, TxEnding::CRLF}, bytes, error) && bytes == "help\r\n", "UTF-8 text and exact CRLF append");
        check(make_tx_payload({L"中文", false, TxEnding::None}, bytes, error) && bytes == "\xE4\xB8\xAD\xE6\x96\x87", "Unicode text produces UTF-8 wire bytes");
        check(make_tx_payload({L"00 ff 0D0a", true, TxEnding::None}, bytes, error) && bytes == std::string("\0\xFF\r\n",4), "HEX including NUL FF and contiguous bytes");
        check(!make_tx_payload({L"0x41", true}, bytes, error) && bytes.empty() && !error.empty(), "invalid HEX never partially sends");
        check(!make_tx_payload({L"ABC", true}, bytes, error) && bytes.empty(), "odd HEX rejected");
        check(!make_tx_payload({L"", false}, bytes, error) && make_tx_payload({L"", false, TxEnding::CR}, bytes, error) && bytes == "\r", "empty text requires explicit line ending");
        check(!make_tx_payload({std::wstring(4096,L'a'),false,TxEnding::CRLF}, bytes, error), "wire limit includes appended ending");
        check(!make_tx_payload({std::wstring(1,0xD800), false}, bytes, error), "invalid UTF-16 rejected");
        TxEntry restored; const TxEntry text{L" \"中文\" ; = ",false,TxEnding::LF};
        check(decode_tx_entry(encode_tx_entry(text), restored) && restored.text == text.text && restored.ending == text.ending && !restored.hex, "portable TX encoding preserves quotes whitespace and Unicode");
        check(decode_tx_entry(encode_tx_entry({L"00 ff",true,TxEnding::CRLF}), restored) && restored.hex && restored.text == L"00 FF" && restored.ending == TxEnding::CRLF, "HEX preset normalizes safely without duplicating suffix");
        check(!decode_tx_entry(L"T0:00",restored) && !decode_tx_entry(L"T9:41",restored) && !decode_tx_entry(L"T0:FF",restored), "malformed or invisible config input rejected");
        check(save_tx_history(ini,{text,{L"help",false,TxEnding::CR}}) && load_tx_history(ini).size() == 2 && load_tx_history(ini)[0].text == text.text, "history portable round trip");
        check(save_tx_preset(ini,7,&text) && load_tx_presets(ini)[7].text == text.text && save_tx_preset(ini,7,nullptr) && load_tx_presets(ini)[7].text.empty(), "preset save clear never executes data");
        SerialControls controls; controls.flow = SerialFlow::RtsCts; controls.dtr = false;
        check(save_uart_controls(ini,controls) && load_uart_controls(ini,error) == controls && error.empty(), "single-key serial option round trip");
        WritePrivateProfileStringW(L"SerialControls",L"Configuration",L"v1:9:1:1",ini.c_str());
        check(load_uart_controls(ini,error) == SerialControls{} && !error.empty(), "malformed controls fall back visibly to compatible defaults");
        DCB dcb{}; dcb.DCBlength = sizeof(dcb); dcb.fOutX = dcb.fInX = dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = TRUE;
        check(configure_serial_dcb(dcb,{}) && !dcb.fOutX && !dcb.fInX && !dcb.fOutxCtsFlow && !dcb.fOutxDsrFlow && dcb.fDtrControl == DTR_CONTROL_ENABLE && dcb.fRtsControl == RTS_CONTROL_ENABLE, "None explicitly clears inherited flow flags with compatible DTR RTS defaults");
        check(configure_serial_dcb(dcb,controls) && dcb.fOutxCtsFlow && dcb.fRtsControl == RTS_CONTROL_HANDSHAKE && dcb.fDtrControl == DTR_CONTROL_DISABLE, "RTS CTS owns RTS while DTR remains manual");
        controls.flow = SerialFlow::DsrDtr; controls.rts = false;
        check(configure_serial_dcb(dcb,controls) && dcb.fOutxDsrFlow && dcb.fDtrControl == DTR_CONTROL_HANDSHAKE && dcb.fRtsControl == RTS_CONTROL_DISABLE, "DSR DTR owns DTR while RTS remains manual");
        controls.flow = SerialFlow::XonXoff;
        check(configure_serial_dcb(dcb,controls) && dcb.fOutX && dcb.fInX && dcb.XonChar == 17 && dcb.XoffChar == 19 && !dcb.fNull && !dcb.fErrorChar && !dcb.fAbortOnError, "XON XOFF explicit policy preserves raw received bytes");
        SerialSettings configured; configured.port = {L"COM77",L"Fixture",L"USB\\CONTROL-FIXTURE",L"fake://controls"}; configured.controls = controls;
        auto seen = std::make_shared<SerialSettings>(); SerialOpenJob opener;
        check(opener.start(configured,[seen] { return std::make_unique<OpenControlsFixture>(seen); }), "background open fixture starts");
        std::unique_ptr<SerialTransport> transport; const auto deadline = GetTickCount64() + 1000;
        while (!opener.poll(transport,error) && GetTickCount64() < deadline) Sleep(1);
        check(transport && seen->controls == controls && seen->port.open_path == configured.port.open_path, "background open receives complete control settings instead of losing them in legacy tuple");
        SerialConnectionState connection; connection.begin(configured,false); connection.connected(configured.port);
        controls.flow = SerialFlow::RtsCts; connection.update_controls(controls); connection.lost(100);
        check(connection.retrying() && connection.active().controls == controls, "same-device recovery retains confirmed live flow settings");
        SerialOperationQueue queue; SerialOperation operation; operation.bytes = "status"; uint64_t id = 0;
        check(!queue.submit(operation,id,error), "disconnected operation rejected"); queue.begin();
        check(queue.submit(operation,id,error) && id == 1 && queue.take(operation), "connected bounded FIFO accepts request");
        const auto old = operation;
        for (int i = 0; i < 16; ++i) { SerialOperation item; item.bytes = "x"; check(queue.submit(item,id,error), "bounded queue fixture enqueue"); }
        check(!queue.submit({},id,error), "queue overflow rejected without dropping accepted requests");
        queue.cancel(); queue.begin(); check(queue.queued() == 0 && !queue.current(old.generation), "disconnect cancels queued work and invalidates generation");
        queue.complete({old,2,false,false,L"partial"}); const auto completed = queue.results();
        check(completed.size() == 17 && completed.back().cancelled && completed.back().accepted == 2, "late partial result cannot replay into new connection");
        UartStatisticsStore stats; stats.receive(100,100); stats.transmit(4,true,150); stats.observe({CE_FRAME|CE_RXPARITY|CE_OVERRUN|CE_RXOVER|CE_BREAK,MS_CTS_ON,true});
        const auto before = stats.snapshot(200), after = stats.snapshot(2000);
        check(before.rx_bytes == 100 && before.tx_bytes == 4 && before.tx_failures == 1 && before.frame == 1 && before.parity == 1 && before.overrun == 1 && before.rx_overflow == 1 && before.breaks == 1, "driver error observations and accepted bytes counted independently");
        check(before.rx_per_second == 100 && before.tx_per_second == 4 && after.rx_per_second == 0 && after.tx_per_second == 0 && after.rx_bytes == 100, "one-second traffic window expires without losing totals");
        stats.reset(); check(stats.snapshot(2000).rx_bytes == 0 && !stats.snapshot(2000).line.modem_valid, "statistics reset is independent of capture");
        test_uart_native_io();
        std::cout << "PASS: UART operations core; no device access\n"; return 0;
    } catch (const std::exception& ex) { std::cerr << "FAIL: " << ex.what() << '\n'; return 1; }
}
