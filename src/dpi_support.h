#pragma once
#include <windows.h>
#include <string>

inline HFONT make_ui_font(HWND window, bool enlarged = false) {
    const UINT dpi=GetDpiForWindow(window);
    LOGFONTW font{};
    if (!SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT,sizeof(font),&font,0,dpi?dpi:96)) {
        SystemParametersInfoW(SPI_GETICONTITLELOGFONT,sizeof(font),&font,0);
        font.lfHeight=MulDiv(font.lfHeight,dpi?dpi:96,96);
    }
    if (std::wstring(GetCommandLineW()).find(L"--font150")!=std::wstring::npos) font.lfHeight=MulDiv(font.lfHeight,3,2);
    else if (enlarged) font.lfHeight=MulDiv(font.lfHeight,5,4);
    font.lfWeight=FW_NORMAL;
    return CreateFontIndirectW(&font);
}
inline void set_child_fonts(HWND owner,HFONT font) {
    for (HWND child=GetWindow(owner,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
}
