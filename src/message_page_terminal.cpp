#include "message_page.h"
#include "uart_operations.h"
#include <commctrl.h>

void MessagePage::set_keyboard_available(bool available,uint64_t generation,bool connected) {
    const bool new_connection=connected&&(!keyboard_connected_||generation!=keyboard_generation_);
    if(!connected) keyboard_armed_=false;else if(new_connection) keyboard_armed_=true;
    if(!available||new_connection)keyboard_high_surrogate_=0;
    keyboard_available_=connected&&available;keyboard_generation_=generation;keyboard_connected_=connected;
}
void MessagePage::echo_tx(std::string_view bytes) {if(local_echo_) {const auto text=L"LOCAL TX ASCII (not RX): "+tx_ascii_preview(bytes);SetWindowTextW(info_,text.c_str());}}
bool MessagePage::keyboard_input(UINT message,WPARAM w,LPARAM) {
    if(!keyboard_armed_ || !keyboard_available_) return false;
    KeyboardTx request;
    if(message==WM_PASTE || (message==WM_KEYDOWN && w=='V' && GetKeyState(VK_CONTROL)<0)) {
        request.paste=true;
        if(!OpenClipboard(hwnd_)) return true;const auto data=GetClipboardData(CF_UNICODETEXT);
        if(data) {const auto size=GlobalSize(data);const auto value=static_cast<const wchar_t*>(GlobalLock(data));if(value){size_t n=0;while(n<size/sizeof(wchar_t) && n<4097 && value[n]) ++n;if(n<=4096 && n<size/sizeof(wchar_t)) request.bytes=message_utf8(std::wstring_view(value,n));GlobalUnlock(data);}}
        CloseClipboard();
        if(request.bytes.empty()||request.bytes.size()>4096) {SetWindowTextW(info_,L"Paste rejected: empty/invalid clipboard or more than 4096 UTF-8 bytes.");return true;}
    } else if(message==WM_KEYDOWN) {
        if(w=='C' && GetKeyState(VK_CONTROL)<0) request.bytes="\x03";
        else if(w==VK_RETURN) request.bytes="\r";else if(w==VK_BACK) request.bytes="\b";else if(w==VK_TAB) request.bytes="\t";else if(w==VK_ESCAPE) request.bytes="\x1b";else return false;
    } else if(message==WM_CHAR && w>=32 && w!=127) {
        // Supplementary characters arrive as UTF-16 pairs; do not send lone surrogates.
        const auto c=static_cast<wchar_t>(w);
        if(c>=0xd800 && c<=0xdbff) {keyboard_high_surrogate_=c;return true;}
        std::wstring text;if(c>=0xdc00&&c<=0xdfff) {if(!keyboard_high_surrogate_) return true;text+=keyboard_high_surrogate_;keyboard_high_surrogate_=0;}else keyboard_high_surrogate_=0;text+=c;request.bytes=message_utf8(text);
    } else return false;
    SendMessageW(GetParent(hwnd_),kKeyboardTx,keyboard_generation_,reinterpret_cast<LPARAM>(&request));return true;
}
