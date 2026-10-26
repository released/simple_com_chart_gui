#pragma once
#include "ui_language.h"
#include <commctrl.h>
namespace ui {
HWND create_window_ex(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
HWND create_window(LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
void localize_dialog(HWND);
void localize_menu(HMENU);
void choices(HWND);
std::wstring control_value(HWND);
int message_box(HWND,LPCWSTR,LPCWSTR,UINT);
}
