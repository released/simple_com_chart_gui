#include "input_framing.h"
#include "message_format.h"
#include <windows.h>
#include <stdexcept>
TextFraming saved_text_framing(const std::wstring& ini,unsigned source){
    if(source>2)throw std::runtime_error("Invalid text-framing source.");
    const wchar_t* keys[]={L"A",L"B",L"Replay"};wchar_t b[128]{};
    if(GetPrivateProfileStringW(L"InputFraming",keys[source],L"",b,128,ini.c_str())>=127)throw std::runtime_error("Text framing settings truncated.");
    return decode_text_framing(message_utf8(b));
}
