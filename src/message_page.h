#pragma once
#include <windows.h>
#include "message_buffer.h"

class MessagePage {
public:
    static constexpr UINT kFontChanged = WM_APP + 4;
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
    const MessageBuffer& buffer() const { return buffer_; }
    bool hex_mode() const;
    std::string export_text() const;
    void append(std::string_view bytes, uint64_t skipped = 0);
    void reset();
    void refresh();
private:
    enum { kMode = 1, kFollow, kClear, kFontDown, kFontUp };
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handle(UINT, WPARAM, LPARAM);
    void layout(int w, int h);
    bool follows() const;
    void update_font_controls();
    HWND hwnd_ = nullptr, label_ = nullptr, mode_ = nullptr, follow_ = nullptr, clear_ = nullptr;
    HWND info_ = nullptr, view_ = nullptr;
    HWND font_label_ = nullptr, font_down_ = nullptr, font_size_ = nullptr, font_up_ = nullptr;
    HFONT mono_font_ = nullptr;
    LOGFONTW mono_lf_{};
    int font_points_ = kDefaultFontPoints;
    MessageBuffer buffer_;
    uint64_t rendered_offset_ = 0;
    size_t rendered_size_ = 0;
    bool rebuild_ = true, dirty_ = true;
};
