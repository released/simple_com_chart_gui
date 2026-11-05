#include "message_page.h"
#include "runtime_capture.h"
#include "light_terminal.h"
#include <commctrl.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace {
struct RepaintProbe {
    HWND window;
    unsigned redraw_off=0, replace_all=0, erases=0, paints=0, whole_paints=0;
    explicit RepaintProbe(HWND value):window(value) {
        if(!SetWindowSubclass(window,procedure,0x524650,reinterpret_cast<DWORD_PTR>(this)))
            throw std::runtime_error("Cannot observe Messages repaint.");
    }
    ~RepaintProbe(){if(IsWindow(window))RemoveWindowSubclass(window,procedure,0x524650);}
    static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data) {
        auto& probe=*reinterpret_cast<RepaintProbe*>(data);
        if(message==WM_SETREDRAW&&!w)++probe.redraw_off;
        if(message==WM_SETTEXT)++probe.replace_all;
        if(message==WM_ERASEBKGND)++probe.erases;
        if(message==WM_PAINT){
            ++probe.paints;RECT update{},client{};GetClientRect(window,&client);
            if(GetUpdateRect(window,&update,FALSE)&&EqualRect(&update,&client))++probe.whole_paints;
        }
        return DefSubclassProc(window,message,w,l);
    }
};
void repaint_pump(){
    MSG message{};unsigned count=0;
    while(count++<32&&PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){
        TranslateMessage(&message);DispatchMessageW(&message);
    }
}
}

bool MessagePage::verify_repaint_controls(const std::wstring& folder) {
    std::ofstream report(std::filesystem::path(folder)/L"message-repaint-result.txt");report<<std::unitbuf;
    bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto text=[&]{
        std::wstring value(static_cast<size_t>(GetWindowTextLengthW(view_))+1,L'\0');
        GetWindowTextW(view_,value.data(),static_cast<int>(value.size()));value.pop_back();
        if(view_==rich_view_)value.erase(std::remove(value.begin(),value.end(),L'\n'),value.end());
        return value;
    };
    const auto exact=[&]{
        auto expected=(terminal_?format_light_terminal(buffer_.bytes(),options_):format_message(buffer_.bytes(),buffer_.offset(),buffer_.receipts(),options_,buffer_.origin_ms())).text;
        if(view_==rich_view_)expected.erase(std::remove(expected.begin(),expected.end(),L'\n'),expected.end());
        return text()==expected;
    };
    try{
        for(unsigned variant=0;variant<5;++variant){
            wrap_=variant==3;terminal_=paused_=highlight_matches_=false;
            terminal_=variant==4;
            MessageDisplayOptions display;
            display.encoding=variant==1?MessageEncoding::Hex:(variant==2||variant==3)?MessageEncoding::Utf8:MessageEncoding::Ascii;
            display.clock=variant==2?MessageClock::Local:MessageClock::None;
            display.ansi_sgr=variant==3;
            configure_display(display);reset();SendMessageW(follow_,BM_SETCHECK,BST_CHECKED,0);
            std::string seed;for(unsigned line=0;line<2000;++line)seed+="filled row "+std::to_string(line)+": abcdefghijklmnopqrstuvwxyz\r\n";
            append(seed);refresh();UpdateWindow(view_);SetFocus(view_);
            check("fixture fills visible log rows and starts following",SendMessageW(view_,EM_GETFIRSTVISIBLELINE,0,0)>100&&exact());
            RECT before{};GetWindowRect(view_,&before);
            unsigned whole_dirty=0,queued_focus_changes=0;bool content=true,focus=true,geometry=true;
            {
                RepaintProbe probe(view_);
                for(unsigned line=0;line<24;++line){
                    const unsigned intervals[]{50,62,75};Sleep(intervals[line%3]);
                    const auto focus_before=GetFocus();
                    append(variant==3?"\x1b[31mnext colored row\x1b[0m\r\n":variant==4?"\rprogress: 123\b4\x1b[K\r\n":"next row: 123.456\r\n");refresh();
                    focus=focus&&GetFocus()==focus_before;
                    RECT update{},client{},after{};GetClientRect(view_,&client);GetWindowRect(view_,&after);
                    if(GetUpdateRect(view_,&update,FALSE)&&EqualRect(&update,&client))++whole_dirty;
                    UpdateWindow(view_);const auto before_pump=GetFocus();repaint_pump();
                    if(GetFocus()!=before_pump)++queued_focus_changes;
                    content=content&&exact();
                    geometry=geometry&&EqualRect(&before,&after);
                }
                report<<"variant="<<variant<<" appended=24 redraw_off="<<probe.redraw_off<<" whole_text_replacements="<<probe.replace_all
                    <<" erases="<<probe.erases<<" paints="<<probe.paints<<" whole_paints="<<probe.whole_paints<<" whole_dirty="<<whole_dirty
                    <<" content="<<content<<" focus="<<focus<<" geometry="<<geometry<<" queued_focus_changes="<<queued_focus_changes<<'\n';
                check("steady receive does not replace the complete text",probe.replace_all==0);
                if(variant!=3)check("plain monitor append keeps native redraw/visibility enabled",probe.redraw_off==0);
                if(variant==0)check("filled ASCII monitor avoids repeated full-client invalidation",whole_dirty==0);
            }
            check("paced append preserves exact formatted bytes, focus and geometry",content&&focus&&geometry);
            check("filled monitor keeps following the newest rows",SendMessageW(view_,EM_GETFIRSTVISIBLELINE,0,0)>100);
            if(variant==0||variant==3){
                const auto name=variant==0?L"message-repaint-ascii.png":L"message-repaint-rich.png";
                check("filled log screenshot",save_runtime_frame(GetParent(hwnd_),std::filesystem::path(folder)/name));
            }
            SendMessageW(follow_,BM_SETCHECK,BST_UNCHECKED,0);
            SendMessageW(view_,EM_SETSEL,0,0);SendMessageW(view_,EM_SCROLLCARET,0,0);SendMessageW(view_,EM_LINESCROLL,0,12);
            const auto top=SendMessageW(view_,EM_GETFIRSTVISIBLELINE,0,0);
            for(unsigned line=0;line<8;++line){append("follow disabled\r\n");refresh();}
            check("inspected scroll position survives receive",SendMessageW(view_,EM_GETFIRSTVISIBLELINE,0,0)==top&&exact());
            reset();append(std::string(MessageBuffer::kCapacity-64,'x'));refresh();
            append(std::string(128,'y'));refresh();UpdateWindow(view_);
            check("retention eviction remains exact and bounded",buffer_.offset()>0&&buffer_.bytes().size()<=MessageBuffer::kCapacity&&exact());
            check("evicted inspected history clamps to the first retained line",SendMessageW(view_,EM_GETFIRSTVISIBLELINE,0,0)==0);
        }
        wrap_=terminal_=false;MessageDisplayOptions unicode;unicode.encoding=MessageEncoding::Utf8;
        configure_display(unicode);reset();append("\xF0\x9F");refresh();
        check("incremental UTF-8 holds a split supplementary character",text().empty());
        append("\x98\x80\r");refresh();append("\n");refresh();
        check("incremental UTF-8 and split CRLF match complete formatting",exact());
        terminal_=true;configure_display(unicode);reset();append("\xF0\x9F\x98\x80");refresh();
        append("\r\xF0\x9F\x98\x81");refresh();
        check("terminal tail replacement preserves a complete surrogate pair",exact()&&text().size()==2);
    }catch(const std::exception& error){check(error.what(),false);}
    wrap_=terminal_=paused_=highlight_matches_=false;configure_display({});reset();SendMessageW(follow_,BM_SETCHECK,BST_CHECKED,0);
    return pass;
}
