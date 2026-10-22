#pragma once
#include <windows.h>
#include <vector>

class HoverHelp {
public:
    bool create(HWND parent, HFONT font);
    void add(HWND control, const wchar_t* text);
    void layout();
    void destroy();
    HWND hwnd() const { return hwnd_; }
private:
    HWND hwnd_ = nullptr, parent_ = nullptr;
    std::vector<HWND> controls_;
};
