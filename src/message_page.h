#pragma once
#include <windows.h>
#include "message_buffer.h"
#include "uart_recorder.h"
#include <set>

class MessagePage {
public:
    static constexpr UINT kFontChanged = WM_APP + 4;
    static constexpr UINT kKeyboardTx=WM_APP+12,kKeyboardArm=WM_APP+13;
    static constexpr UINT kLocateSource=WM_APP+11;
    static constexpr UINT kLogRequest=WM_APP+31;
    enum LogAction {SaveLog=17,StartLog,StopLog};
    HWND save_log_control() const {return save_log_;}
    HWND start_log_control() const {return start_log_;}
    HWND stop_log_control() const {return stop_log_;}
    HWND log_info_control() const {return log_info_;}
    HMENU build_options_menu() const;
    void set_exporting(bool busy);
    void stage_log_export(std::wstring filename,uint64_t bytes);
    void finish_log_export(bool success);
    void set_recording_status(const RecordingStatus&,bool can_start);
    struct KeyboardTx {std::string bytes;bool paste=false;};
    void set_keyboard_available(bool available,uint64_t generation,bool connected);
    void arm_keyboard(bool arm) {keyboard_armed_=arm && keyboard_available_;keyboard_high_surrogate_=0;}
    bool keyboard_armed() const {return keyboard_armed_;}
    bool local_echo() const {return local_echo_;}
    HWND status_control() const {return info_;}
    void echo_tx(std::string_view bytes);
    static constexpr int kDefaultFontPoints = 10, kMinFontPoints = 8, kMaxFontPoints = 32;
    bool create(HWND parent, HFONT font);
    HWND hwnd() const { return hwnd_; }
    HWND view() const { return view_; }
    HWND mode_control() const { return mode_; }
    HWND follow_control() const { return follow_; }
    HWND clear_control() const { return clear_; }
    HWND font_down_control() const { return font_down_; }
    HWND font_up_control() const { return font_up_; }
    int font_points() const { return font_points_; }
    bool set_font_points(int points);
    void set_ui_font(HFONT font);
    const MessageBuffer& buffer() const { return buffer_; }
    bool hex_mode() const;
    std::string export_text() const;
    void append(std::string_view bytes, uint64_t skipped = 0);
    void append_history(const MessageBuffer& bytes);
    void load_options(const std::wstring& ini);
    bool save_options(const std::wstring& ini) const;
    const MessageDisplayOptions& options() const { return options_; }
    bool wraps() const { return wrap_; }
    void configure_display(const MessageDisplayOptions& options);
    void find(bool previous = false);
    bool verify_receive_controls(const std::wstring& folder);
    bool translate_key(const MSG& message);
    void reset();
    void refresh();
    void set_source_end(uint64_t value) {source_end_=value;}
    bool navigate_source(uint64_t offset,size_t count);
    void locate_cursor_source();
    void set_offline_source(bool offline, bool synthetic) { offline_ = offline; synthetic_ = synthetic; rebuild_ = dirty_ = true; }
private:
    enum { kMode = 1, kFollow, kClear, kFontDown, kFontUp, kOptions, kSearchToggle,
        kSearch, kRegex, kCase, kPrevious, kNext, kHighlight, kMark, kNextMark, kClearMarks,
        kClockBase = 100, kNewlineBase = 110, kWrap = 120, kSgr, kPause, kCodePageBase = 130 };
    enum {kTerminal=140,kKeyboard,kEcho,kLocate};
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT, WPARAM, LPARAM);
    void layout(int w, int h);
    bool follows() const;
    void update_font_controls();
    void create_options(HFONT font);
    void create_log_controls();
    void update_log_info();
    void show_options();
    bool options_command(int id);
    void update_search();
    void decorate(const FormattedMessage& formatted);
    void choose_view();
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    HWND hwnd_ = nullptr, label_ = nullptr, mode_ = nullptr, follow_ = nullptr, clear_ = nullptr;
    HWND info_ = nullptr, view_ = nullptr;
    HWND font_label_ = nullptr, font_down_ = nullptr, font_size_ = nullptr, font_up_ = nullptr;
    HFONT mono_font_ = nullptr;
    LOGFONTW mono_lf_{};
    int font_points_ = kDefaultFontPoints;
    UINT font_dpi_ = 0;
    MessageBuffer buffer_;
    uint64_t rendered_offset_ = 0;
    size_t rendered_size_ = 0;
    bool rebuild_ = true, dirty_ = true;
    HWND plain_view_ = nullptr, rich_view_ = nullptr, options_button_ = nullptr, search_toggle_ = nullptr;
    HWND search_ = nullptr, regex_ = nullptr, case_ = nullptr, previous_ = nullptr, next_ = nullptr, highlight_ = nullptr;
    HWND mark_ = nullptr, next_mark_ = nullptr, clear_marks_ = nullptr;
    HWND save_log_=nullptr,start_log_=nullptr,stop_log_=nullptr,log_info_=nullptr;
    bool export_busy_=false;
    std::wstring saved_log_,pending_log_,recording_label_,recording_name_,log_info_text_;
    uint64_t saved_log_bytes_=0,pending_log_bytes_=0,recording_bytes_=0;
    int log_info_height_=0,info_height_=0;
    HMODULE rich_library_ = nullptr;
    MessageDisplayOptions options_;
    bool wrap_ = false, paused_ = false, search_visible_ = false, highlight_matches_ = false;
    std::wstring rendered_text_;
    std::vector<MessageMatch> matches_;
    std::set<int> bookmarks_;
    uint64_t bookmark_offset_ = 0;
    std::wstring search_status_;
    bool offline_ = false, synthetic_ = false;
    uint64_t source_end_=UINT64_MAX;
    uint64_t navigation_offset_=UINT64_MAX;
    size_t navigation_count_=0;
    bool terminal_=false,keyboard_armed_=false,keyboard_available_=false,keyboard_connected_=false,local_echo_=false;
    uint64_t keyboard_generation_=0;
    wchar_t keyboard_high_surrogate_=0;
    bool keyboard_input(UINT,WPARAM,LPARAM);
};
