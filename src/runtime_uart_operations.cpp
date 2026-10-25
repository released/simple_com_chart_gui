#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "runtime_serial_fixture.h"
#include "ui_metrics.h"

namespace {
struct OperationFixture {
    std::mutex mutex; std::condition_variable wake;
    bool hold_write = false, entered = false, released = false;
    size_t partial = SIZE_MAX; std::vector<std::string> writes;
    unsigned controls = 0, breaks = 0; DWORD worker = 0;
    SerialControls last_controls;
    std::atomic<bool> error_with_tail{false};
};
class OperationsTransport : public FakeSerialTransport {
public:
    OperationsTransport(std::shared_ptr<FakeSerialState> serial, std::shared_ptr<OperationFixture> ops)
        : FakeSerialTransport(std::move(serial)), operations_(std::move(ops)) {}
    size_t write_bytes(std::string_view bytes, std::wstring* error) override {
        std::unique_lock<std::mutex> lock(operations_->mutex); operations_->worker = GetCurrentThreadId(); operations_->entered = true;
        operations_->wake.wait(lock, [&] { return !operations_->hold_write || operations_->released; });
        const auto n = std::min(bytes.size(), operations_->partial); operations_->writes.emplace_back(bytes.substr(0,n));
        if (n != bytes.size() && error) *error = L"Simulated partial TX; no retry"; return n;
    }
    bool set_controls(const SerialControls& value, std::wstring*) override {
        std::lock_guard<std::mutex> lock(operations_->mutex); ++operations_->controls; operations_->last_controls = value; operations_->worker = GetCurrentThreadId(); return true;
    }
    bool send_break(std::wstring*) override { std::lock_guard<std::mutex> lock(operations_->mutex); ++operations_->breaks; operations_->worker = GetCurrentThreadId(); return true; }
    SerialLineStatus sample_status() override { return {0, MS_CTS_ON | MS_RLSD_ON, true}; }
    std::vector<std::string> read_lines(std::wstring* error, std::string* raw) override {
        if (operations_->error_with_tail.exchange(false)) {
            if (raw) *raw = "tail-on-error\r\n"; if (error) *error = L"Simulated receive completion with final bytes"; return {};
        }
        return FakeSerialTransport::read_lines(error,raw);
    }
private:
    std::shared_ptr<OperationFixture> operations_;
};
}

bool UartConsole::verify(const std::wstring& folder) {
    std::ofstream report(std::filesystem::path(folder) / L"uart-console-result.txt"); bool pass = true;
    const auto check = [&](const char* name, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << name << '\n'; pass = pass && ok; };
    set_input({L"help",false,TxEnding::CRLF});
    ::SendMessageW(save_preset_, BM_CLICK, 0, 0); set_input({}); ::SendMessageW(load_preset_, BM_CLICK, 0, 0);
    check("actual preset buttons restore text/mode/ending without sending", input().text == L"help" && input().ending == TxEnding::CRLF);
    ::SendMessageW(send_, BM_CLICK, 0, 0);
    check("actual Send button routes to owner queue and portable history", !history_.empty() && history_.front().text == L"help" && !load_tx_history(ini_).empty());
    set_input({L"0xGG",true}); ::SendMessageW(send_, BM_CLICK, 0, 0);
    check("invalid HEX reports a visible error", ::GetWindowTextLengthW(transcript_) > 0);
    set_input({L"help",false,TxEnding::CRLF});
    ::SendMessageW(flow_, CB_SETCURSEL, 1, 0); SendMessage(WM_COMMAND, MAKEWPARAM(kFlow, CBN_SELCHANGE));
    check("RTS manual checkbox disabled during RTS CTS handshake", !::IsWindowEnabled(rts_));
    ::SendMessageW(flow_, CB_SETCURSEL, 3, 0); SendMessage(WM_COMMAND, MAKEWPARAM(kFlow, CBN_SELCHANGE));
    check("DTR manual checkbox disabled during DSR DTR handshake", !::IsWindowEnabled(dtr_));
    set_controls({});
    for (const auto width : {1000,1400}) {
        SetWindowPos(nullptr,0,0,width,720,SWP_NOMOVE | SWP_NOZORDER); layout();
        CRect client; GetClientRect(&client); bool fits = true, captions = true;
        for (auto child = ::GetWindow(m_hWnd,GW_CHILD); child; child = ::GetWindow(child,GW_HWNDNEXT)) {
            RECT bounds{}; ::GetWindowRect(child,&bounds); ::MapWindowPoints(nullptr,m_hWnd,reinterpret_cast<POINT*>(&bounds),2);
            fits = fits && bounds.left >= 0 && bounds.right <= client.right && bounds.top >= 0 && bounds.bottom <= client.bottom;
        }
        for (auto child : {apply_,break_,reset_,send_,save_preset_,load_preset_,clear_preset_,clear_view_,clear_history_}) {
            RECT r{}; ::GetClientRect(child,&r); captions = captions && r.right >= control_text_width(child,20);
        }
        check("console controls fit tested window size", fits); check("console captions have measured padding", captions);
    }
    check("console screenshot", save_runtime_frame(m_hWnd,std::filesystem::path(folder)/L"uart-console.png"));
    OnCancel(); check("console close only hides the modeless window", GetSafeHwnd() && !IsWindowVisible()); ShowWindow(SW_SHOW);
    return pass;
}

bool CMainDialog::verify_uart_operations() {
    const auto folder = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(folder / L"uart-operations-result.txt"); bool pass = true;
    const auto check = [&](const char* name, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << name << '\n'; report.flush(); pass = pass && ok; };
    const bool demo = demo_; demo_ = false;
    const auto original_path = settings_path_; const auto original_controls = desired_controls_; const auto original_notice = uart_notice_;
    auto original = std::move(serial_mgr_); settings_path_ = (folder / L"uart-operation-fixture.ini").wstring();
    auto serial = std::make_shared<FakeSerialState>(); auto ops = std::make_shared<OperationFixture>();
    serial_mgr_ = std::make_unique<OperationsTransport>(serial,ops);
    unsigned notices = 0;
    uart_notice_ = [&](HWND owner, LPCWSTR text, LPCWSTR, UINT flags) { ++notices; check("hardware action has owned cancelable warning", owner == uart_console_.GetSafeHwnd() && flags == (MB_OKCANCEL|MB_ICONWARNING) && std::wstring(text).find(L"reset") != std::wstring::npos); return IDOK; };
    const auto pump = [&](const std::function<bool()>& ready) {
        const auto until = GetTickCount64() + 3000;
        while (GetTickCount64() < until) { poll_serial_stop(); poll_uart_operations(); flush_pending_lines(); if (ready()) return true; Sleep(1); }
        return ready();
    };
    serial_mgr_->connect(L"fake://operations",115200,8,0,0,nullptr);
    SerialSettings active; active.port = {L"COM4",L"Test",L"USB\\OPS",L"fake://operations"}; connection_.begin(active,false); connection_.connected(active.port);
    uart_statistics_.reset(); start_serial_thread(); show_uart_console();
    const bool console = uart_console_.verify(folder.wstring());
    check("button TX sends exactly once on a worker, no invalid-HEX/preset transmission", pump([&] { std::lock_guard<std::mutex> lock(ops->mutex); return ops->writes.size() == 1; }));
    { std::lock_guard<std::mutex> lock(ops->mutex); check("exact UTF-8 command suffix and worker thread", ops->writes.size() == 1 && ops->writes[0] == "help\r\n" && ops->worker != GetCurrentThreadId()); }
    check("TX does not fabricate RX messages or chart rows", message_page_.buffer().bytes().empty() || message_page_.buffer().bytes().find("help\r\n") == std::string::npos);
    UartConsole::Request control{UartConsole::Request::Action::Controls}; control.controls.flow = SerialFlow::RtsCts; control.controls.dtr = false;
    OnUartRequest(0,reinterpret_cast<LPARAM>(&control));
    check("confirmed controls execute and persist after completion", pump([&] { return !controls_pending_id_; }) && desired_controls_ == control.controls && connection_.active().controls == control.controls);
    std::wstring error; check("controls INI persists all fields together", load_uart_controls(settings_path_,error) == control.controls);
    UartConsole::Request breaker{UartConsole::Request::Action::Break}; OnUartRequest(0,reinterpret_cast<LPARAM>(&breaker));
    check("Break requires confirmation and executes once", pump([&] { std::lock_guard<std::mutex> lock(ops->mutex); return ops->breaks == 1; }) && notices == 2);
    uart_notice_ = [&](HWND, LPCWSTR, LPCWSTR, UINT) { uart_operations_.cancel(); uart_operations_.begin(); return IDOK; };
    OnUartRequest(0,reinterpret_cast<LPARAM>(&breaker));
    { std::lock_guard<std::mutex> lock(ops->mutex); check("connection-generation change during confirmation rejects stale hardware action", ops->breaks == 1 && uart_operations_.queued() == 0); }
    uart_notice_ = [&](HWND, LPCWSTR, LPCWSTR, UINT) { return IDCANCEL; };
    OnUartRequest(0,reinterpret_cast<LPARAM>(&breaker));
    { std::lock_guard<std::mutex> lock(ops->mutex); check("cancelled hardware confirmation sends nothing", ops->breaks == 1 && uart_operations_.queued() == 0); }
    { std::lock_guard<std::mutex> lock(ops->mutex); ops->partial = 2; }
    UartConsole::Request send{UartConsole::Request::Action::Send}; send.tx = {L"abcdef",false}; OnUartRequest(0,reinterpret_cast<LPARAM>(&send));
    check("partial write is counted, reported and never automatically retried", pump([&] { return uart_statistics_.snapshot(GetTickCount64()).tx_failures == 1; }));
    { std::lock_guard<std::mutex> lock(ops->mutex); check("only accepted prefix counted", ops->writes.size() == 2 && ops->writes[1] == "ab"); ops->partial = SIZE_MAX; ops->hold_write = true; ops->entered = false; }
    OnUartRequest(0,reinterpret_cast<LPARAM>(&send));
    check("blocked write fixture enters worker", pump([&] { std::lock_guard<std::mutex> lock(ops->mutex); return ops->entered; }));
    { std::lock_guard<std::mutex> lock(serial->mutex); serial->raw = "rx_during_tx:201\r\n"; }
    check("RX remains live while TX worker is blocked", pump([&] { return uart_statistics_.snapshot(GetTickCount64()).rx_bytes > 0; }));
    OnUartRequest(0,reinterpret_cast<LPARAM>(&send)); // Must be cancelled before execution.
    const auto start = GetTickCount64(); disconnect();
    check("disconnect returns without a UI-thread join of blocked I/O", disconnect_pending_ && GetTickCount64() - start < 100 && ::IsWindow(m_hWnd));
    { std::lock_guard<std::mutex> lock(ops->mutex); ops->released = true; } ops->wake.notify_all();
    check("cancellation completes and returns to idle", pump([&] { return !disconnect_pending_; }) && connection_.mode() == SerialConnectionState::Mode::Idle);
    { std::lock_guard<std::mutex> lock(ops->mutex); check("queued command cancelled, not replayed", ops->writes.size() == 3); }
    const auto stats = uart_statistics_.snapshot(GetTickCount64());
    check("traffic totals keep actual accepted bytes and RX pin observations", stats.tx_bytes == 14 && stats.rx_bytes > 0 && stats.line.modem_valid);
    serial_mgr_->connect(L"fake://operations",115200,8,0,0,nullptr); connection_.begin(active,false); connection_.connected(active.port);
    ops->error_with_tail = true; start_serial_thread();
    check("read completion bytes survive a simultaneous receive error and asynchronous stop", pump([&] {
        return connection_.retrying() && message_page_.buffer().bytes().find("tail-on-error") != std::string::npos;
    }));
    disconnect(); pump([&] { return !disconnect_pending_; });
    connection_.cancel(); serial_mgr_ = std::move(original); uart_notice_ = original_notice; desired_controls_ = original_controls; settings_path_ = original_path; demo_ = demo;
    if (uart_console_.GetSafeHwnd()) uart_console_.DestroyWindow(); update_connection_ui();
    return pass && console;
}
