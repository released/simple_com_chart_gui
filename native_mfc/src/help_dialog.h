#pragma once

#include <windows.h>

class HelpDialog {
public:
    void show(HWND parent);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    HWND edit_ = nullptr;
    HWND btn_close_ = nullptr;
    HFONT font_ = nullptr;
};
