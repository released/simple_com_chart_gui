#include "message_page.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>

namespace {
std::wstring caption(HWND control) {
    std::wstring value(static_cast<size_t>(GetWindowTextLengthW(control)) + 1, 0);
    GetWindowTextW(control, value.data(), static_cast<int>(value.size())); value.pop_back(); return value;
}
}

void MessagePage::create_options(HFONT font) {
    for (const auto* name : {L"UTF-8", L"ANSI"}) SendMessageW(mode_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
    const auto button = [&](const wchar_t* name, int id, bool check = false) {
        return ui::create_window(L"BUTTON", name, WS_CHILD | WS_TABSTOP | (check ? BS_AUTOCHECKBOX : BS_PUSHBUTTON),
            0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    };
    options_button_ = button(L"Options...", kOptions); search_toggle_ = button(L"Find...", kSearchToggle);
    ShowWindow(options_button_, SW_SHOW); ShowWindow(search_toggle_, SW_SHOW);
    search_ = ui::create_window_ex(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(kSearch), nullptr, nullptr);
    SendMessageW(search_, EM_SETLIMITTEXT, 256, 0);
    regex_ = button(L"Regex", kRegex, true); case_ = button(L"Case", kCase, true);
    previous_ = button(L"Prev", kPrevious); next_ = button(L"Next", kNext);
    highlight_ = button(L"Highlight", kHighlight, true); mark_ = button(L"Mark", kMark);
    next_mark_ = button(L"Next mark", kNextMark); clear_marks_ = button(L"Clear marks", kClearMarks);
    rich_library_ = LoadLibraryW(L"Msftedit.dll");
    if (rich_library_) rich_view_ = ui::create_window_ex(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"", WS_CHILD | WS_TABSTOP |
        WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        0, 0, 1, 1, hwnd_, nullptr, nullptr, nullptr);
    if (rich_view_) {
        SendMessageW(rich_view_, EM_EXLIMITTEXT, 0, 2 * 1024 * 1024);
        SendMessageW(rich_view_, EM_SETBKGNDCOLOR, 0, RGB(0,0,0));
        SendMessageW(rich_view_, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    }
    for (auto control : {plain_view_, rich_view_, search_})
        if (control) SetWindowSubclass(control, EditProc, 1, reinterpret_cast<DWORD_PTR>(this));
}

LRESULT CALLBACK MessagePage::EditProc(HWND control, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
    auto self = reinterpret_cast<MessagePage*>(data);
    if(control==self->view_ && self->keyboard_input(msg,wp,lp)) return 0;
    MSG message{}; message.hwnd = control; message.message = msg; message.wParam = wp; message.lParam = lp;
    if (self->translate_key(message)) return 0;
    return DefSubclassProc(control, msg, wp, lp);
}

bool MessagePage::translate_key(const MSG& message) {
    if (message.message != WM_KEYDOWN || !IsChild(hwnd_, message.hwnd) || !IsWindowVisible(hwnd_)) return false;
    const bool ctrl = GetKeyState(VK_CONTROL) < 0;
    if(message.hwnd==view_ && keyboard_input(message.message,message.wParam,message.lParam)) return true;
    if (ctrl && message.wParam == 'A' && (message.hwnd == search_ || message.hwnd == view_)) { SendMessageW(message.hwnd, EM_SETSEL, 0, -1); return true; }
    if (ctrl && message.wParam == 'F') { if (!search_visible_) options_command(kSearchToggle); SetFocus(search_); return true; }
    if (message.wParam == VK_F3 || (message.hwnd == search_ && message.wParam == VK_RETURN)) { find(GetKeyState(VK_SHIFT) < 0); return true; }
    if (message.wParam == VK_F2) { options_command(ctrl ? kMark : kNextMark); return true; }
    return false;
}

void MessagePage::choose_view() {
    const auto chosen = rich_view_ && (wrap_ || options_.ansi_sgr || highlight_matches_) ? rich_view_ : plain_view_;
    if (view_ == chosen) return;
    ShowWindow(view_, SW_HIDE); view_ = chosen;
    SendMessageW(view_, WM_SETFONT, reinterpret_cast<WPARAM>(mono_font_), FALSE);
    ShowWindow(view_, SW_SHOW); rebuild_ = true;
    RECT client{}; GetClientRect(hwnd_, &client); layout(client.right, client.bottom);
}

void MessagePage::configure_display(const MessageDisplayOptions& options) {
    options_ = options;
    SendMessageW(mode_, CB_SETCURSEL, static_cast<int>(options.encoding), 0);
    rebuild_ = dirty_ = true; refresh();
}

void MessagePage::load_options(const std::wstring& ini) {
    options_.encoding = static_cast<MessageEncoding>(std::clamp<int>(GetPrivateProfileIntW(L"Messages", L"Encoding", 0, ini.c_str()), 0, 3));
    options_.clock = static_cast<MessageClock>(std::clamp<int>(GetPrivateProfileIntW(L"Messages", L"Clock", 0, ini.c_str()), 0, 3));
    options_.newline = static_cast<MessageNewline>(std::clamp<int>(GetPrivateProfileIntW(L"Messages", L"Newline", 0, ini.c_str()), 0, 3));
    const auto cp = GetPrivateProfileIntW(L"Messages", L"CodePage", CP_ACP, ini.c_str());
    options_.code_page = cp == CP_ACP || (cp != CP_UTF8 && IsValidCodePage(cp)) ? cp : CP_ACP;
    options_.ansi_sgr = GetPrivateProfileIntW(L"Messages", L"AnsiSgr", 0, ini.c_str()) != 0;
    wrap_ = GetPrivateProfileIntW(L"Messages", L"Wrap", 0, ini.c_str()) != 0;
    terminal_=GetPrivateProfileIntW(L"Messages",L"LightTerminal",0,ini.c_str())!=0;local_echo_=GetPrivateProfileIntW(L"Messages",L"LocalEcho",0,ini.c_str())!=0;keyboard_armed_=false;
    configure_display(options_);
}

bool MessagePage::save_options(const std::wstring& ini) const {
    bool ok = true;
    const auto write = [&](const wchar_t* key, unsigned value) { ok = WritePrivateProfileStringW(L"Messages", key, std::to_wstring(value).c_str(), ini.c_str()) && ok; };
    write(L"Encoding", static_cast<unsigned>(SendMessageW(mode_, CB_GETCURSEL, 0, 0)));
    write(L"Clock", static_cast<unsigned>(options_.clock)); write(L"Newline", static_cast<unsigned>(options_.newline));
    write(L"CodePage", options_.code_page); write(L"AnsiSgr", options_.ansi_sgr); write(L"Wrap", wrap_);
    write(L"LightTerminal",terminal_);write(L"LocalEcho",local_echo_);
    return ok;
}

HMENU MessagePage::build_options_menu() const {
    HMENU menu = CreatePopupMenu(), clocks = CreatePopupMenu(), lines = CreatePopupMenu(), code_pages = CreatePopupMenu();
    const wchar_t* clock_names[] = {L"No timestamp", L"PC local time", L"Elapsed (PC receipt)", L"Delta between lines (PC receipt)"};
    const wchar_t* line_names[] = {L"Auto CR / LF / CRLF", L"LF", L"CR", L"CRLF"};
    for (int i = 0; i < 4; ++i) {
        AppendMenuW(clocks, MF_STRING | (static_cast<int>(options_.clock) == i ? MF_CHECKED : 0), kClockBase + i, clock_names[i]);
        AppendMenuW(lines, MF_STRING | (static_cast<int>(options_.newline) == i ? MF_CHECKED : 0), kNewlineBase + i, line_names[i]);
    }
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(clocks), L"Timestamp");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(lines), L"Line endings");
    const wchar_t* page_names[] = {L"Windows system ANSI", L"Big5 (950)", L"GBK (936)", L"Western (1252)"};
    const unsigned pages[] = {CP_ACP, 950, 936, 1252};
    for (unsigned i = 0; i < 4; ++i) AppendMenuW(code_pages, MF_STRING | (options_.code_page == pages[i] ? MF_CHECKED : 0), kCodePageBase + i, page_names[i]);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(code_pages), L"ANSI code page (when Display = ANSI)");
    AppendMenuW(menu, MF_STRING | (wrap_ ? MF_CHECKED : 0), kWrap, L"Word wrap");
    AppendMenuW(menu, MF_STRING | (options_.ansi_sgr ? MF_CHECKED : 0), kSgr, L"ANSI SGR colors (not full terminal emulation)");
    AppendMenuW(menu, MF_STRING | (paused_ ? MF_CHECKED : 0), kPause, L"Pause display (receiving / recording continue)");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING|(terminal_?MF_CHECKED:0),kTerminal,L"Light terminal display (CR / BS / erase line; no timestamps)");
    AppendMenuW(menu,MF_STRING|(keyboard_armed_?MF_CHECKED:0),kKeyboard,keyboard_armed_?L"Disarm keyboard TX":L"Enable keyboard TX...");
    AppendMenuW(menu,MF_STRING|(local_echo_?MF_CHECKED:0),kEcho,L"Local TX echo in status (never RX/log)");
    AppendMenuW(menu,MF_STRING,kLocate,L"Locate RX row at caret in table / chart");
    ui::localize_menu(menu);return menu;
}
void MessagePage::show_options() {
    const auto menu=build_options_menu();
    RECT rect{}; GetWindowRect(options_button_, &rect);
    const int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN, rect.left, rect.bottom, 0, hwnd_, nullptr);
    DestroyMenu(menu); if (chosen) options_command(chosen);
}

bool MessagePage::options_command(int id) {
    if(id==kLocate) {locate_cursor_source();return true;}
    if(id==kKeyboard) {if(keyboard_armed_) keyboard_armed_=false;else SendMessageW(GetParent(hwnd_),kKeyboardArm,0,0);return true;}
    if(id==kTerminal||id==kEcho) {if(id==kTerminal) {terminal_=!terminal_;rebuild_=dirty_=true;refresh();}else local_echo_=!local_echo_;SendMessageW(GetParent(hwnd_),kFontChanged,0,0);return true;}
    if (id < kOptions || (id > kClearMarks && id < kClockBase) || (id > kPause && (id < kCodePageBase || id >= kCodePageBase + 4))) return false;
    if (id == kSearch) return false; // EN_CHANGE is intentionally not a per-keystroke regex evaluation.
    if (id == kOptions) { show_options(); return true; }
    if (id == kSearchToggle) {
        search_visible_ = !search_visible_;
        for (auto control : {search_, regex_, case_, previous_, next_, highlight_, mark_, next_mark_, clear_marks_}) ShowWindow(control, search_visible_ ? SW_SHOW : SW_HIDE);
        RECT rect{}; GetClientRect(hwnd_, &rect); layout(rect.right, rect.bottom);
        if (search_visible_) SetFocus(search_);
        return true;
    }
    if (id == kPrevious || id == kNext) { find(id == kPrevious); return true; }
    if (id == kMark) {
        DWORD start = 0; SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), 0);
        const int line = static_cast<int>(SendMessageW(view_, EM_LINEFROMCHAR, start, 0));
        if (!bookmarks_.erase(line)) bookmarks_.insert(line);
        search_status_ = ui::tr(L"Marked line") + L" " + std::to_wstring(line + 1);
        SetWindowTextW(info_, (search_status_ + L" | " + ui::tr(L"Marks:") + L" " + std::to_wstring(bookmarks_.size())).c_str()); return true;
    }
    if (id == kNextMark) {
        DWORD start = 0; SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), 0);
        const int line = static_cast<int>(SendMessageW(view_, EM_LINEFROMCHAR, start, 0));
        if (!bookmarks_.empty()) {
            auto it = bookmarks_.upper_bound(line); if (it == bookmarks_.end()) it = bookmarks_.begin();
            const auto at = SendMessageW(view_, EM_LINEINDEX, *it, 0);
            SendMessageW(follow_, BM_SETCHECK, BST_UNCHECKED, 0); SendMessageW(view_, EM_SETSEL, at, at); SendMessageW(view_, EM_SCROLLCARET, 0, 0);
        }
        return true;
    }
    if (id == kClearMarks) { bookmarks_.clear(); SetWindowTextW(info_, ui::caption(L"Bookmarks cleared")); return true; }
    if (id >= kClockBase && id < kClockBase + 4) options_.clock = static_cast<MessageClock>(id - kClockBase);
    else if (id >= kNewlineBase && id < kNewlineBase + 4) options_.newline = static_cast<MessageNewline>(id - kNewlineBase);
    else if (id == kWrap) wrap_ = !wrap_;
    else if (id == kSgr) options_.ansi_sgr = !options_.ansi_sgr;
    else if (id == kPause) paused_ = !paused_;
    else if (id == kHighlight) highlight_matches_ = SendMessageW(highlight_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    else if (id >= kCodePageBase && id < kCodePageBase + 4) { const unsigned pages[] = {CP_ACP, 950, 936, 1252}; options_.code_page = pages[id - kCodePageBase]; }
    if (!paused_) { bookmarks_.clear(); rebuild_ = dirty_ = true; refresh(); }
    if (id == kPause) SetWindowTextW(options_button_, paused_ ? L"Paused..." : L"Options...");
    SendMessageW(GetParent(hwnd_), kFontChanged, 0, 0); return true;
}

void MessagePage::update_search() {
    const auto pattern = caption(search_);
    std::wstring error;
    matches_ = find_message_matches(rendered_text_, pattern, SendMessageW(regex_, BM_GETCHECK, 0, 0) == BST_CHECKED,
        SendMessageW(case_, BM_GETCHECK, 0, 0) == BST_CHECKED, error);
    search_status_ = error.empty() ? (pattern.empty() ? ui::tr(L"PC receipt time") : std::to_wstring(matches_.size()) + L" " + ui::tr(L"matches (max 4096)")) : ui::tr(error);
}

void MessagePage::find(bool previous) {
    if (highlight_matches_ && !paused_) { dirty_ = rebuild_ = true; refresh(); }
    update_search();
    if (matches_.empty()) { SetWindowTextW(info_, search_status_.c_str()); return; }
    DWORD start = 0, end = 0; SendMessageW(view_, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    auto match = matches_.front();
    if (previous) {
        match = matches_.back(); for (auto it = matches_.rbegin(); it != matches_.rend(); ++it) if (it->start < start) { match = *it; break; }
    } else { for (const auto& candidate : matches_) if (candidate.start >= end) { match = candidate; break; } }
    SendMessageW(follow_, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessageW(view_, EM_SETSEL, match.start, match.start + match.length); SendMessageW(view_, EM_SCROLLCARET, 0, 0);
    SetWindowTextW(info_, search_status_.c_str());
}

void MessagePage::decorate(const FormattedMessage& formatted) {
    if (view_ != rich_view_) return;
    CHARFORMAT2W cf{}; cf.cbSize = sizeof(cf); cf.dwMask = CFM_COLOR | CFM_BACKCOLOR;
    cf.crTextColor = RGB(0,255,0); cf.crBackColor = RGB(0,0,0);
    SendMessageW(view_, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
    // RichEdit stores CRLF as one character; convert span positions with a prefix map.
    std::vector<size_t> positions(formatted.text.size() + 1); size_t at = 0;
    for (size_t i = 0; i < formatted.text.size(); ++i) { positions[i] = at; if (!(formatted.text[i] == '\n' && i && formatted.text[i - 1] == '\r')) ++at; }
    positions.back() = at;
    for (const auto& span : formatted.colors) {
        SendMessageW(view_, EM_SETSEL, positions[span.start], positions[span.start + span.count]);
        cf.crTextColor = span.color; SendMessageW(view_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    }
    if (highlight_matches_) for (const auto& match : matches_) {
        SendMessageW(view_, EM_SETSEL, match.start, match.start + match.length);
        cf.crTextColor = RGB(0,0,0); cf.crBackColor = RGB(230,210,70);
        SendMessageW(view_, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    }
}
