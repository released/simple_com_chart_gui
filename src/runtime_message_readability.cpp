#include "message_page.h"
#include "runtime_capture.h"
#include "ui_language.h"
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {
std::wstring window_text(HWND window) {
    std::wstring value(static_cast<size_t>(GetWindowTextLengthW(window))+1,L'\0');
    GetWindowTextW(window,value.data(),static_cast<int>(value.size()));value.pop_back();return value;
}
LRESULT CALLBACK locate_probe(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data) {
    if(message==MessagePage::kLocateSource){*reinterpret_cast<uint64_t*>(data)=*reinterpret_cast<const uint64_t*>(l);return 0;}
    return DefSubclassProc(window,message,w,l);
}
}

bool MessagePage::verify_readability_controls(const std::wstring& folder) {
    std::ofstream report(std::filesystem::path(folder)/L"message-readability-result.txt");bool pass=true;
    const auto check=[&](const char* label,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<label<<'\n';report.flush();pass=pass&&ok;};
    const auto raw=std::filesystem::path(folder)/L"missing-readability.ini";
    load_options(raw.wstring());reset();SendMessageW(follow_,BM_SETCHECK,BST_CHECKED,0);
    check("missing INI defaults wrap and NUL suppression on without creating a file",wrap_&&hide_nul_&&!std::filesystem::exists(raw));
    const std::string long_line(3000,'X');append(long_line);refresh();
    LRESULT narrow_lines=0,wide_lines=0;bool bounded=true;
    RECT before{};GetWindowRect(hwnd_,&before);MapWindowPoints(nullptr,GetParent(hwnd_),reinterpret_cast<POINT*>(&before),2);
    for(const int width:{360,640,1100}){
        MoveWindow(hwnd_,before.left,before.top,width,900,TRUE);UpdateWindow(view_);
        const auto lines=SendMessageW(view_,EM_GETLINECOUNT,0,0);
        if(width==360)narrow_lines=lines;if(width==1100)wide_lines=lines;
        SCROLLBARINFO scroll{};scroll.cbSize=sizeof(scroll);
        const bool horizontal=GetScrollBarInfo(view_,OBJID_HSCROLL,&scroll)&&!(scroll.rgstate[0]&(STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN));
        bounded=bounded&&view_==rich_view_&&lines>1&&!horizontal&&export_text()==long_line;
    }
    check("long unbroken line wraps at every tested width without horizontal scrolling or inserted export newlines",bounded&&narrow_lines>wide_lines);
    options_command(kWrap);const auto off_wide=SendMessageW(view_,EM_GETLINECOUNT,0,0);
    MoveWindow(hwnd_,before.left,before.top,360,900,TRUE);const auto off_narrow=SendMessageW(view_,EM_GETLINECOUNT,0,0);
    SCROLLINFO horizontal{};horizontal.cbSize=sizeof(horizontal);horizontal.fMask=SIF_RANGE|SIF_PAGE;
    const bool can_scroll=GetScrollInfo(view_,SB_HORZ,&horizontal)&&horizontal.nMax-horizontal.nMin+1>static_cast<int>(horizontal.nPage);
    report<<"wrap_off: wide_lines="<<off_wide<<" narrow_lines="<<off_narrow<<" horizontal="<<can_scroll<<'\n';
    check("explicit wrap off preserves horizontal scrolling without viewport reflow",!wrap_&&view_==plain_view_&&off_wide==off_narrow&&can_scroll&&export_text()==long_line);
    MoveWindow(hwnd_,before.left,before.top,1100,900,TRUE);
    options_command(kWrap);reset();append("READY\r\n");refresh();const auto ready=window_text(view_);
    for(unsigned i=0;i<3;++i){append(std::string(4096,'\0'));refresh();}
    check("split NUL flood leaves text/export unchanged and raw bytes retained",window_text(view_)==ready&&export_text()=="READY\r\n"&&nul_received_==12288&&buffer_.bytes().size()==12295);
    const auto status=window_text(info_);
    check("NUL counter and possible-noise hint are visible outside message text",status.find(L"12288")!=status.npos&&status.find(ui::tr(L"NUL hidden (possible noise; inspect HEX)"))!=status.npos);
    options_command(kHideNul);check("user can reveal NUL without receiving again",window_text(view_).find(L"\\x00")!=std::wstring::npos&&nul_received_==12288);
    options_command(kHideNul);check("re-enabling suppression restores readable text",window_text(view_)==ready);
    terminal_=false;configure_display({});reset();append(std::string("\0A\0B",4));set_source_end(104);refresh();
    check("text Save As snapshot follows suppression without discarding raw bytes",export_text()=="AB"&&buffer_.bytes()==std::string("\0A\0B",4));
    check("source navigation preserves byte offsets after hidden NUL",navigate_source(101,1));
    DWORD first=0,last=0;SendMessageW(view_,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));
    check("visible source byte highlights its exact rendered character",first==0&&last==1);
    check("hidden-only byte range reports no visible mapping",!navigate_source(100,1));
    reset();std::string binary;for(unsigned i=0;i<64;++i)binary+=static_cast<char>(i);append(binary);set_source_end(1064);
    MessageDisplayOptions hex;hex.encoding=MessageEncoding::Hex;configure_display(hex);
    MoveWindow(hwnd_,before.left,before.top,360,900,TRUE);refresh();
    check("wrapped HEX still exports all bytes and offsets",export_text()==message_utf8(buffer_.hex()));
    check("HEX source navigation works across physical wrapped rows",navigate_source(1032,1));
    SendMessageW(view_,EM_GETSEL,reinterpret_cast<WPARAM>(&first),reinterpret_cast<LPARAM>(&last));
    check("HEX target is the original third data row",rendered_text_.substr(first,8)==L"00000020");
    uint64_t located=UINT64_MAX;const auto parent=GetParent(hwnd_);
    if(SetWindowSubclass(parent,locate_probe,0x4e554c,reinterpret_cast<DWORD_PTR>(&located))){
        SendMessageW(view_,EM_SETSEL,first+45,first+45);locate_cursor_source();
        RemoveWindowSubclass(parent,locate_probe,0x4e554c);
    }
    check("HEX caret navigation counts data rows rather than soft wraps",located==1032);
    configure_display({});reset();append(std::string(MessageBuffer::kCapacity+17,'\0'));refresh();
    check("all-NUL retention stays bounded and does not create text",nul_received_==MessageBuffer::kCapacity+17&&buffer_.bytes().size()==MessageBuffer::kCapacity&&window_text(view_).empty());
    reset();MessageBuffer history;history.append(std::string("\0A\0",3));append_history(history);refresh();
    check("restored history counts NUL and renders visible bytes only",nul_received_==2&&export_text()=="A");
    reset();check("Clear resets NUL count and notice",nul_received_==0&&window_text(info_).find(ui::tr(L"NUL (0x00) received:"))==std::wstring::npos);
    const auto choices=std::filesystem::path(folder)/L"readability-settings.ini";
    wrap_=hide_nul_=false;check("explicit off preferences save",save_options(choices.wstring()));
    wrap_=hide_nul_=true;load_options(choices.wstring());check("explicit saved zero overrides both defaults",!wrap_&&!hide_nul_);
    wrap_=hide_nul_=true;check("enabled preferences save",save_options(choices.wstring()));
    wrap_=hide_nul_=false;load_options(choices.wstring());check("enabled preferences restore",wrap_&&hide_nul_);
    RECT parent_client{};GetClientRect(parent,&parent_client);SendMessageW(parent,WM_SIZE,SIZE_RESTORED,MAKELPARAM(parent_client.right,parent_client.bottom));
    append("Ready: received NUL bytes; raw recording retained.\r\n"+std::string(400,'X')+std::string(2000,'\0'));refresh();
    check("readability screenshot",save_runtime_frame(parent,std::filesystem::path(folder)/L"message-readability.png"));
    MoveWindow(hwnd_,before.left,before.top,before.right-before.left,before.bottom-before.top,TRUE);
    wrap_=hide_nul_=terminal_=false;configure_display({});reset();SendMessageW(follow_,BM_SETCHECK,BST_CHECKED,0);
    return pass;
}
