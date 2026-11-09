#pragma once
#include <windows.h>
#include "message_buffer.h"
#include "uart_recorder.h"
#include "monitor_features.h"
#include <set>

class MessagePage {
public:
    static constexpr UINT kFontChanged = WM_APP + 4;
    static constexpr UINT kKeyboardTx=WM_APP+12,kKeyboardArm=WM_APP+13;
    static constexpr UINT kLocateSource=WM_APP+11;
    static constexpr UINT kLogRequest=WM_APP+31;
    static constexpr UINT kFeatureRequest=WM_APP+34;
    enum FeatureAction { EditKeywords=1,InspectBytes=2 };
    const std::wstring& keyword_definition() const {return keyword_definition_;}
    void set_keywords(std::wstring,const std::vector<KeywordRule>&);
    int minimum_height(int width) const;
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
    static constexpr int kDefaultFontPoints = 11, kMinFontPoints = 8, kMaxFontPoints = 32;
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
#ifdef CHART_RUNTIME_CHECKS
    bool verify_default_controls(const std::wstring& folder);
    bool verify_readability_controls(const std::wstring& folder);
    bool verify_repaint_controls(const std::wstring& folder);
#endif
    bool translate_key(const MSG& message);
    void reset();
    void refresh();
    void set_source_end(uint64_t value) {source_end_=value;}
    bool navigate_source(uint64_t offset,size_t count);
    void locate_cursor_source();
    void set_offline_source(bool offline, bool synthetic) { offline_ = offline; synthetic_ = synthetic; rebuild_ = dirty_ = true; }
private:
#ifdef CHART_RUNTIME_CHECKS
    friend class CMainDialog;
#endif
    enum { kMode = 1, kFollow, kClear, kFontDown, kFontUp, kOptions, kSearchToggle,
        kSearch, kRegex, kCase, kPrevious, kNext, kHighlight, kMark, kNextMark, kClearMarks,
        kClockBase = 100, kNewlineBase = 110, kWrap = 120, kSgr, kPause, kCodePageBase = 130 };
    enum {kTerminal=140,kKeyboard,kEcho,kLocate,kHideNul};
    enum {kKeywords=150,kInspect,kSmartScroll,kLatest};
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT, WPARAM, LPARAM);
    void layout(int w, int h);
    bool follows() const;
    void follow_tail();
    void update_inspection_status();
    void user_scroll_begin(UINT,WPARAM);
    void user_scroll_end(UINT,WPARAM);
    void stream_keyword_colors(const FormattedMessage&,size_t begin);
    void update_font_controls();
    void create_options(HFONT font);
    void create_log_controls();
    void update_log_info();
    void show_options();
    bool options_command(int id);
    void update_search();
    void decorate(const FormattedMessage& formatted);
    void choose_view();
    MessageDisplayOptions display_options() const;
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
    bool wrap_ = true, hide_nul_ = true, paused_ = false, search_visible_ = false, highlight_matches_ = false;
    uint64_t nul_received_ = 0;
    std::wstring rendered_text_;
    std::vector<MessageMatch> matches_;
    std::set<int> bookmarks_;
    uint64_t bookmark_offset_ = 0;
    std::wstring search_status_;
    bool offline_ = false, synthetic_ = false;
    uint64_t source_end_=UINT64_MAX;
    uint64_t navigation_offset_=UINT64_MAX;
    size_t navigation_count_=0;
    bool terminal_=true,keyboard_armed_=false,keyboard_available_=false,keyboard_connected_=false,local_echo_=true;
    HWND latest_=nullptr,inspection_info_=nullptr;
    std::wstring inspection_text_;
    bool smart_scroll_=false,smart_suspended_=false,program_scroll_=false,keyword_limited_=false;
    int scroll_position_before_=0;
    uint64_t received_total_=0,seen_total_=0;
    std::wstring keyword_definition_,keyword_error_,preference_path_;
    std::vector<KeywordRule> keyword_rules_;
    uint64_t keyboard_generation_=0;
    wchar_t keyboard_high_surrogate_=0;
    bool keyboard_input(UINT,WPARAM,LPARAM);
};
