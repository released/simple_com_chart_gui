#pragma once

#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>

class ChannelPanel {
public:
    bool create(HWND parent, int x, int y, int w, int h, int id);
    HWND hwnd() const;

    void reset();
    void update_count(int count);
    void ensure_channel(const std::string& key, bool enabled, COLORREF color);
    void update_values(const std::unordered_map<std::string, int>& latest);
    std::unordered_map<std::string, bool> get_checkbox_state_map() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void layout(int w, int h);
    void on_all_none(bool all_checked);
    void send_channel_changed();

    HWND hwnd_ = nullptr;
    HWND parent_ = nullptr;
    HWND label_count_ = nullptr;
    HWND btn_all_ = nullptr;
    HWND btn_none_ = nullptr;
    HWND list_ = nullptr;

    std::vector<std::string> keys_;
    std::unordered_map<std::string, int> index_map_;
    std::unordered_map<std::string, COLORREF> color_map_;
    bool suppress_notify_ = false;
};
