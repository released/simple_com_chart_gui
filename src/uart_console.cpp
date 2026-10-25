#include "uart_console.h"
#include "dpi_support.h"
#include "resource.h"
#include "ui_metrics.h"
#include <algorithm>

namespace {
std::wstring caption(HWND control) {
    std::wstring value(static_cast<size_t>(::GetWindowTextLengthW(control)) + 1, 0);
    ::GetWindowTextW(control, value.data(), static_cast<int>(value.size())); value.pop_back(); return value;
}
int selection(HWND control) { return static_cast<int>(::SendMessageW(control, CB_GETCURSEL, 0, 0)); }
}
BEGIN_MESSAGE_MAP(UartConsole, CDialog)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_DPICHANGED, &UartConsole::OnDpiChanged)
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

bool UartConsole::show(CWnd* owner, const std::wstring& ini, HFONT font) {
    if (!GetSafeHwnd()) { ini_ = ini; font_ = font; if (!Create(IDD_UART_CONSOLE, owner)) return false; }
    ShowWindow(SW_SHOW); if (IsIconic()) ShowWindow(SW_RESTORE); SetForegroundWindow(); return true;
}
HWND UartConsole::label(const wchar_t* text) { return ::CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_NOPREFIX, 0,0,1,1,m_hWnd,nullptr,nullptr,nullptr); }
HWND UartConsole::button(const wchar_t* text, int id, bool checkbox) {
    return ::CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | (checkbox ? BS_AUTOCHECKBOX : BS_PUSHBUTTON), 0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);
}
HWND UartConsole::combo(int id, std::initializer_list<const wchar_t*> names, bool editable) {
    const auto result = ::CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | (editable ? CBS_DROPDOWN | CBS_AUTOHSCROLL : CBS_DROPDOWNLIST),
        0,0,100,200,m_hWnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);
    for (const auto name : names) ::SendMessageW(result, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
    if (names.size()) ::SendMessageW(result, CB_SETCURSEL, 0, 0); return result;
}
BOOL UartConsole::OnInitDialog() {
    owned_font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    if(owned_font_) font_=owned_font_;
    CDialog::OnInitDialog();
    flow_label_ = label(L"Flow control:"); flow_ = combo(kFlow, {L"None", L"RTS / CTS", L"XON / XOFF", L"DSR / DTR"});
    dtr_ = button(L"DTR on", kDtr, true); rts_ = button(L"RTS on", kRts, true);
    apply_ = button(L"Apply serial", kApply); break_ = button(L"Send Break...", kBreak); reset_ = button(L"Reset statistics", kReset);
    statistics_ = ::CreateWindowW(L"STATIC", L"RX: 0 | TX: 0", WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    mode_label_ = label(L"TX:"); mode_ = combo(kMode, {L"Text UTF-8", L"HEX"});
    ending_label_ = label(L"Append:"); ending_ = combo(kEnding, {L"None", L"CR", L"LF", L"CRLF"});
    rx_hint_ = label(L"RX stays in Messages. TX accepted by driver is not a device ACK.");
    input_ = combo(kInput, {}, true); ::SendMessageW(input_, CB_LIMITTEXT, 16384, 0); send_ = button(L"Send", kSend);
    preset_label_ = label(L"Preset:"); preset_ = combo(kPreset, {L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8"});
    save_preset_ = button(L"Save preset", kSavePreset); load_preset_ = button(L"Load preset", kLoadPreset); clear_preset_ = button(L"Clear preset", kClearPreset);
    clear_view_ = button(L"Clear TX view", kClearView); clear_history_ = button(L"Clear history", kClearHistory);
    transcript_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0,0,1,1,m_hWnd,nullptr,nullptr,nullptr); ::SendMessageW(transcript_, EM_SETLIMITTEXT, 65536, 0);
    if (!font_) font_ = reinterpret_cast<HFONT>(GetFont()->GetSafeHandle());
    for (auto child = ::GetWindow(m_hWnd, GW_CHILD); child; child = ::GetWindow(child, GW_HWNDNEXT)) ::SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    history_ = load_tx_history(ini_); presets_ = load_tx_presets(ini_); rebuild_history();
    std::wstring error; set_controls(load_uart_controls(ini_, error));
    ready_ = true; layout(); if (!error.empty()) notice(error);
    return TRUE;
}
void UartConsole::set_controls(const SerialControls& value) {
    if (!GetSafeHwnd()) return;
    ::SendMessageW(flow_, CB_SETCURSEL, static_cast<int>(value.flow), 0);
    ::SendMessageW(dtr_, BM_SETCHECK, value.dtr ? BST_CHECKED : BST_UNCHECKED, 0);
    ::SendMessageW(rts_, BM_SETCHECK, value.rts ? BST_CHECKED : BST_UNCHECKED, 0); update_handshake();
}
void UartConsole::update_handshake() {
    ::EnableWindow(dtr_, configurable_ && selection(flow_) != static_cast<int>(SerialFlow::DsrDtr));
    ::EnableWindow(rts_, configurable_ && selection(flow_) != static_cast<int>(SerialFlow::RtsCts));
}
TxEntry UartConsole::input() const { return {caption(input_), selection(mode_) == 1, static_cast<TxEnding>(selection(ending_))}; }
void UartConsole::set_input(const TxEntry& value) {
    ::SetWindowTextW(input_, value.text.c_str()); ::SendMessageW(mode_, CB_SETCURSEL, value.hex ? 1 : 0, 0); ::SendMessageW(ending_, CB_SETCURSEL, static_cast<int>(value.ending), 0);
}
void UartConsole::rebuild_history() {
    const auto draft = input(); ::SendMessageW(input_, CB_RESETCONTENT, 0, 0);
    for (const auto& item : history_) ::SendMessageW(input_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.text.c_str()));
    set_input(draft);
}
void UartConsole::remember(const TxEntry& value) {
    const auto encoded = encode_tx_entry(value);
    history_.erase(std::remove_if(history_.begin(), history_.end(), [&](const TxEntry& item) { return encode_tx_entry(item) == encoded; }), history_.end());
    history_.insert(history_.begin(), value); if (history_.size() > 32) history_.resize(32); rebuild_history();
    if (!save_tx_history(ini_, history_)) notice(L"Cannot save TX history: " + ini_);
}
void UartConsole::request(Request value) {
    const auto owner = GetOwner(); if (!owner || !::IsWindow(owner->GetSafeHwnd())) return;
    ::SendMessageW(owner->GetSafeHwnd(), kRequest, 0, reinterpret_cast<LPARAM>(&value));
}
BOOL UartConsole::OnCommand(WPARAM wp, LPARAM lp) {
    const int id = LOWORD(wp), notification = HIWORD(wp);
    if (id == kFlow && notification == CBN_SELCHANGE) { update_handshake(); return TRUE; }
    if (id == kInput && notification == CBN_SELCHANGE) {
        const int index = selection(input_); if (index >= 0 && static_cast<size_t>(index) < history_.size()) set_input(history_[index]); return TRUE;
    }
    if (notification != BN_CLICKED) return CDialog::OnCommand(wp, lp);
    if (id == kSend) { Request action{Request::Action::Send}; action.tx = input(); request(std::move(action)); return TRUE; }
    if (id == kApply) {
        Request action{Request::Action::Controls}; action.controls.flow = static_cast<SerialFlow>(selection(flow_));
        action.controls.dtr = ::SendMessageW(dtr_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        action.controls.rts = ::SendMessageW(rts_, BM_GETCHECK, 0, 0) == BST_CHECKED; request(std::move(action)); return TRUE;
    }
    if (id == kBreak || id == kReset) { request({id == kBreak ? Request::Action::Break : Request::Action::ResetStatistics}); return TRUE; }
    if (id == kClearView) { ::SetWindowTextW(transcript_, L""); return TRUE; }
    if (id == kClearHistory) { history_.clear(); rebuild_history(); if (!save_tx_history(ini_, history_)) notice(L"Cannot clear saved history: " + ini_); return TRUE; }
    const int slot = selection(preset_);
    if (slot >= 0 && slot < 8 && (id == kSavePreset || id == kLoadPreset || id == kClearPreset)) {
        if (id == kLoadPreset) { std::string payload; std::wstring error; if (make_tx_payload(presets_[slot], payload, error)) set_input(presets_[slot]); else notice(L"This preset is empty."); }
        else {
            auto value = input(); std::string payload; std::wstring error;
            if (id == kSavePreset && !make_tx_payload(value, payload, error)) { notice(error); return TRUE; }
            if (!save_tx_preset(ini_, slot, id == kSavePreset ? &value : nullptr)) notice(L"Cannot save preset: " + ini_);
            else { presets_[slot] = id == kSavePreset ? value : TxEntry{}; notice(id == kSavePreset ? L"Preset saved; nothing sent." : L"Preset cleared."); }
        }
        return TRUE;
    }
    return CDialog::OnCommand(wp, lp);
}
BOOL UartConsole::PreTranslateMessage(MSG* message) {
    if (message && message->message == WM_KEYDOWN && ::IsChild(m_hWnd, message->hwnd) && GetKeyState(VK_CONTROL) < 0) {
        if (message->wParam == VK_RETURN) { SendMessageW(WM_COMMAND, MAKEWPARAM(kSend, BN_CLICKED), 0); return TRUE; }
        if (message->wParam == 'A') {
            wchar_t name[32]{}; ::GetClassNameW(message->hwnd, name, 32);
            if (_wcsicmp(name, L"EDIT") == 0) { ::SendMessageW(message->hwnd, EM_SETSEL, 0, -1); return TRUE; }
        }
    }
    return CDialog::PreTranslateMessage(message);
}
void UartConsole::notice(const std::wstring& text) {
    if (!GetSafeHwnd()) return;
    auto old = caption(transcript_); if (old.size() > 45000) old.erase(0, old.size() - 30000);
    SYSTEMTIME time{}; GetLocalTime(&time); wchar_t prefix[32]{}; swprintf_s(prefix, L"%02u:%02u:%02u.%03u ", time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
    old += prefix; old += text; old += L"\r\n"; ::SetWindowTextW(transcript_, old.c_str());
    const int end = ::GetWindowTextLengthW(transcript_); ::SendMessageW(transcript_, EM_SETSEL, end, end); ::SendMessageW(transcript_, EM_SCROLLCARET, 0, 0);
}
void UartConsole::result(const SerialOperationResult& value) {
    const auto name = value.operation.kind == SerialOperation::Kind::Write ? L"TX" : value.operation.kind == SerialOperation::Kind::Controls ? L"Serial controls" : L"Break";
    auto text = std::wstring(name) + L" [" + value.operation.target + L"] #" + std::to_wstring(value.operation.id) + (value.cancelled ? L" CANCELLED" : value.success ? L" OK" : L" FAILED");
    if (value.operation.kind == SerialOperation::Kind::Write) text += L" | driver accepted " + std::to_wstring(value.accepted) + L" / " + std::to_wstring(value.operation.bytes.size()) + L" | " + tx_preview(value.operation.bytes);
    if (!value.error.empty()) text += L" | " + value.error; notice(text);
}
void UartConsole::update(bool connected, bool configurable, bool applying, const UartStatistics& stats, size_t queued, uint64_t now) {
    if (!GetSafeHwnd()) return;
    configurable_ = configurable && !applying;
    ::EnableWindow(send_, connected); ::EnableWindow(break_, connected); ::EnableWindow(flow_, configurable_); ::EnableWindow(apply_, configurable_); update_handshake();
    const auto pin = [&](unsigned flag) { return !connected || !stats.line.modem_valid ? L"n/a" : (stats.line.modem & flag) ? L"1" : L"0"; };
    const auto text = L"RX: " + std::to_wstring(stats.rx_bytes) + L" bytes | Last 1 s: " + std::to_wstring(static_cast<uint64_t>(stats.rx_per_second)) +
        L" B/s | TX (driver): " + std::to_wstring(stats.tx_bytes) + L" bytes / " + std::to_wstring(static_cast<uint64_t>(stats.tx_per_second)) + L" B/s | TX failures: " + std::to_wstring(stats.tx_failures) +
        L"\r\nDriver error observations - Frame: " + std::to_wstring(stats.frame) + L" | Parity: " + std::to_wstring(stats.parity) + L" | Overrun: " + std::to_wstring(stats.overrun) + L" | RX buffer: " + std::to_wstring(stats.rx_overflow) + L" | Break: " + std::to_wstring(stats.breaks) +
        L"\r\nPins (driver) - CTS: " + pin(MS_CTS_ON) + L" | DSR: " + pin(MS_DSR_ON) + L" | DCD: " + pin(MS_RLSD_ON) + L" | RI: " + pin(MS_RING_ON) +
        L" | Last RX: " + (stats.last_rx_ms ? std::to_wstring(now - std::min(now, stats.last_rx_ms)) + L" ms ago" : L"none") + L" | Queue: " + std::to_wstring(queued);
    if (caption(statistics_) != text) ::SetWindowTextW(statistics_, text.c_str());
}
void UartConsole::OnSize(UINT kind, int w, int h) { CDialog::OnSize(kind, w, h); if (ready_) layout(); }
void UartConsole::OnGetMinMaxInfo(MINMAXINFO* info) { CDialog::OnGetMinMaxInfo(info); info->ptMinTrackSize = {800,560}; }
LRESULT UartConsole::OnDpiChanged(WPARAM,LPARAM value) {
    const auto font=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    if(font) { const auto old=owned_font_; owned_font_=font; font_=font; set_child_fonts(m_hWnd,font); if(old) DeleteObject(old); }
    if(value) { const auto& r=*reinterpret_cast<const RECT*>(value); SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE); }
    layout(); return 0;
}
void UartConsole::OnDestroy() { CDialog::OnDestroy(); if(owned_font_) { DeleteObject(owned_font_); owned_font_=nullptr; font_=nullptr; } }
void UartConsole::layout() {
    if (!ready_) return; CRect client; GetClientRect(&client);
    const int margin = 12, gap = 8, width = client.Width() - 2 * margin;
    RECT selection_bounds{}; ::GetWindowRect(flow_, &selection_bounds);
    const int closed_height = std::max(20, static_cast<int>(selection_bounds.bottom - selection_bounds.top));
    const int row = std::max({28, closed_height, control_font_height(flow_label_) + 12}); int x = margin, y = margin;
    const auto place = [&](HWND control, int size, bool dropdown = false) {
        if (x > margin && x + size > client.right - margin) { x = margin; y += row + gap; }
        int height = closed_height;
        if (dropdown) { RECT r{}; ::GetWindowRect(control, &r); height = static_cast<int>(r.bottom - r.top); }
        ::MoveWindow(control, x, y + (row - height) / 2, size, dropdown ? 240 : closed_height, TRUE); x += size + gap;
    };
    const auto next = [&] { x = margin; y += row + gap; };
    place(flow_label_, control_text_width(flow_label_)); place(flow_, 150, true);
    for (auto control : {dtr_, rts_, apply_, break_, reset_}) place(control, control_text_width(control, control == dtr_ || control == rts_ ? GetSystemMetrics(SM_CXMENUCHECK) + 24 : 24));
    next(); const int statistics_height = std::max(control_font_height(statistics_) * 4 + 8, control_wrapped_text_height(statistics_, width));
    ::MoveWindow(statistics_, margin, y, width, statistics_height, TRUE); y += statistics_height + gap;
    place(mode_label_, control_text_width(mode_label_)); place(mode_, 150, true); place(ending_label_, control_text_width(ending_label_)); place(ending_, 100, true); next();
    ::MoveWindow(rx_hint_, margin, y, width, row, TRUE); next();
    const int send_width = control_text_width(send_, 40); place(input_, std::max(200, width - send_width - gap), true); place(send_, send_width); next();
    place(preset_label_, control_text_width(preset_label_)); place(preset_, 65, true);
    for (auto control : {save_preset_, load_preset_, clear_preset_, clear_view_, clear_history_}) place(control, control_text_width(control, 24));
    next(); ::MoveWindow(transcript_, margin, y, width, std::max<int>(40, client.bottom - margin - y), TRUE);
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}
