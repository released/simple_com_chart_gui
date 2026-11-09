#include "message_page.h"
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include "session_fixture.h"

namespace {
bool passed=true;
void check(bool ok,const char* name){std::cout<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';passed=passed&&ok;}
void pump(){MSG m{};unsigned count=0;while(count++<64&&PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}
struct ViewFixture {
    MessagePage page;HWND host=nullptr;
    ViewFixture(){
        INITCOMMONCONTROLSEX cc{sizeof(cc),ICC_STANDARD_CLASSES};InitCommonControlsEx(&cc);
        WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SccMessageViewTest";
        RegisterClassW(&wc);host=CreateWindowExW(WS_EX_TOOLWINDOW,wc.lpszClassName,L"Owned Messages verification",WS_OVERLAPPEDWINDOW,0,0,820,700,nullptr,nullptr,wc.hInstance,nullptr);
        if(!host||!page.create(host,reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))))throw std::runtime_error("Cannot create Messages fixture");
        ShowWindow(host,SW_SHOWNOACTIVATE);ShowWindow(page.hwnd(),SW_SHOW);resize(820,700);page.reset();
    }
    ~ViewFixture(){if(host)DestroyWindow(host);}
    void resize(int width,int height){MoveWindow(host,0,0,width,height,TRUE);RECT r{};GetClientRect(host,&r);MoveWindow(page.hwnd(),0,0,r.right,r.bottom,TRUE);UpdateWindow(host);pump();}
};
bool bottom(HWND view){
    UpdateWindow(view);SCROLLINFO s{};s.cbSize=sizeof(s);s.fMask=SIF_RANGE|SIF_PAGE|SIF_POS;
    if(!GetScrollInfo(view,SB_VERT,&s))return false;
    const int last=std::max(s.nMin,s.nMax-static_cast<int>(s.nPage)+1);
    const bool ok=s.nPos>=last-1;
    if(!ok)std::cout<<"scroll: pos="<<s.nPos<<" required="<<last<<" first="<<SendMessageW(view,EM_GETFIRSTVISIBLELINE,0,0)<<'\n';
    return ok;
}
std::wstring shown(HWND view){std::wstring s(static_cast<size_t>(GetWindowTextLengthW(view))+1,L'\0');GetWindowTextW(view,s.data(),static_cast<int>(s.size()));s.pop_back();s.erase(std::remove(s.begin(),s.end(),L'\r'),s.end());return s;}
int64_t clock_origin(){
    SYSTEMTIME local{};GetLocalTime(&local);local.wHour=12;local.wMinute=34;local.wSecond=56;local.wMilliseconds=789;
    SYSTEMTIME utc{};FILETIME ft{};
    if(!TzSpecificLocalTimeToSystemTime(nullptr,&local,&utc)||!SystemTimeToFileTime(&utc,&ft))throw std::runtime_error("Cannot create PC local clock fixture");
    ULARGE_INTEGER ticks{};ticks.LowPart=ft.dwLowDateTime;ticks.HighPart=ft.dwHighDateTime;
    return static_cast<int64_t>(ticks.QuadPart/10000)-11644473600000LL;
}
void choose_clock(MessagePage& page,MessageClock clock){
    const auto menu=page.build_options_menu();const auto submenu=GetSubMenu(menu,0);
    const auto id=GetMenuItemID(submenu,static_cast<int>(clock));DestroyMenu(menu);
    SendMessageW(page.hwnd(),WM_COMMAND,MAKEWPARAM(id,0),0);
}
class FontDesktop {
public:
    FontDesktop():original_(GetThreadDesktop(GetCurrentThreadId())){
        const auto name=L"SccFontDefault_"+std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(GetTickCount64());
        owned_=CreateDesktopW(name.c_str(),nullptr,nullptr,0,DESKTOP_CREATEWINDOW|DESKTOP_CREATEMENU|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS,nullptr);
        if(!owned_)throw std::runtime_error("Cannot create private font desktop");
        if(!SetThreadDesktop(owned_)){CloseDesktop(owned_);owned_=nullptr;throw std::runtime_error("Cannot select private font desktop");}
    }
    ~FontDesktop(){close();}
    void close(){if(!owned_)return;check(SetThreadDesktop(original_)!=FALSE,"font fixture restores original thread desktop without input switching");const bool ok=CloseDesktop(owned_)!=FALSE;check(ok,"private font desktop closes after its windows");if(ok)owned_=nullptr;}
private:HDESK original_=nullptr,owned_=nullptr;
};
int native_points(HWND view){
    wchar_t type[64]{};GetClassNameW(view,type,64);
    if(_wcsicmp(type,MSFTEDIT_CLASS)==0){CHARFORMAT2W format{};format.cbSize=sizeof(format);SendMessageW(view,EM_GETCHARFORMAT,SCF_DEFAULT,reinterpret_cast<LPARAM>(&format));return (format.dwMask&CFM_SIZE)?(format.yHeight+10)/20:0;}
    LOGFONTW font{};const auto handle=reinterpret_cast<HFONT>(SendMessageW(view,WM_GETFONT,0,0));
    return GetObjectW(handle,sizeof(font),&font)==sizeof(font)?MulDiv(std::abs(font.lfHeight),72,GetDpiForWindow(view)):0;
}
void font_defaults(ViewFixture& f,SessionFixture& files){
    check(MessagePage::kDefaultFontPoints==11&&f.page.font_points()==11,"new MessagePage defaults to11 points");
    check(native_points(f.page.view())==11,"actual native message font is11 points");
    bool caption=false;for(auto child=GetWindow(f.page.hwnd(),GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))caption=caption||shown(child)==L"11 pt";
    check(caption,"font size readout initially shows11 pt");
    const auto ini=files.file(L"font-default.ini");
    // Exercise the same Win32 preference expression used by CMainDialog::load_settings.
    const auto load=[&]{f.page.set_font_points(static_cast<int>(GetPrivateProfileIntW(L"Messages",L"FontSizePt",MessagePage::kDefaultFontPoints,ini.c_str())));};
    load();check(f.page.font_points()==11&&native_points(f.page.view())==11,"absent INI font uses11 point fallback");
    WritePrivateProfileStringW(L"Messages",L"Wrap",L"1",ini.c_str());
    load();check(f.page.font_points()==11,"missing individual font key also uses11 points");
    f.page.append("font-default:11\r\nretained raw\r\n");f.page.refresh();const auto raw=f.page.buffer().bytes(),exported=f.page.export_text();
    WritePrivateProfileStringW(L"Messages",L"FontSizePt",L"10",ini.c_str());
    load();check(f.page.font_points()==10&&native_points(f.page.view())==10,"explicit saved10 point preference remains10");
    WritePrivateProfileStringW(L"Messages",L"FontSizePt",L"24",ini.c_str());
    load();check(f.page.font_points()==24,"other saved user sizes remain authoritative");
    WritePrivateProfileStringW(L"Messages",L"FontSizePt",nullptr,ini.c_str());load();
    check(f.page.font_points()==11,"removing saved key restores11 point factory default");
    check(f.page.buffer().bytes()==raw&&f.page.export_text()==exported,"default/preference changes preserve raw bytes and export text");
    SendMessageW(f.page.mode_control(),CB_SETCURSEL,1,0);SendMessageW(f.page.hwnd(),WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),0);
    check(f.page.hex_mode()&&native_points(f.page.view())==11,"HEX uses the same11 point native message font");
    SendMessageW(f.page.mode_control(),CB_SETCURSEL,0,0);SendMessageW(f.page.hwnd(),WM_COMMAND,MAKEWPARAM(1,CBN_SELCHANGE),0);
    check(!f.page.hex_mode()&&native_points(f.page.view())==11,"ASCII restores the same11 point native font");
}
void clocks(ViewFixture& f,SessionFixture& files){
    const auto origin=clock_origin();MessageBuffer history;
    history.append("first\r\r\n",0,origin);history.append("second\r\n",0,origin+1500);
    f.page.reset();f.page.append_history(history);
    MessageDisplayOptions o;choose_clock(f.page,MessageClock::Local);
    check(f.page.export_text()=="[12:34:56.789] first\r\n[12:34:58.289] second\r\n","light terminal shows PC receipt timestamps without extra CR CR LF rows");
    check(shown(f.page.view())==L"[12:34:56.789] first\n[12:34:58.289] second\n","actual message window shows PC local timestamps");
    choose_clock(f.page,MessageClock::Elapsed);
    check(f.page.export_text()=="[0.000s] first\r\n[1.500s] second\r\n","terminal elapsed clock uses capture origin and original receipts");
    choose_clock(f.page,MessageClock::Delta);
    check(f.page.export_text()=="[dt +0.000s] first\r\n[dt +1.500s] second\r\n","terminal delta uses receipt times of successive displayed rows");
    const auto ini=files.file(L"messages.ini");check(f.page.save_options(ini.wstring()),"clock choice saves with terminal enabled");
    f.page.configure_display({});f.page.load_options(ini.wstring());
    check(f.page.options().clock==MessageClock::Delta&&f.page.export_text().find("[dt +1.500s]")!=std::string::npos,"saved terminal clock restores visibly");
    check(f.page.buffer().bytes()==history.bytes(),"clock/display choices preserve exact raw bytes");
    f.page.reset();MessageBuffer progress;progress.append("progress: 10%",0,origin);progress.append("\rprogress: 20%\x1b[K\r\nnext",0,origin+2000);f.page.append_history(progress);
    o.clock=MessageClock::Elapsed;f.page.configure_display(o);
    check(f.page.export_text()=="[0.000s] progress: 20%\r\n[2.000s] next","CR overwrite and erase cannot remove timestamp prefixes");
    f.page.reset();MessageBuffer zeros;zeros.append(std::string(4,'\0'),0,origin);f.page.append_history(zeros);f.page.refresh();
    check(f.page.export_text().empty(),"hidden NUL creates no phantom terminal clock");
    zeros.reset();zeros.append("visible",0,origin+1500);f.page.append_history(zeros);f.page.refresh();
    check(f.page.export_text()=="[1.500s] visible","first visible byte after NUL retains its own receipt timestamp");
    f.page.set_offline_source(true,true);f.page.refresh();check(f.page.export_text()=="visible","synthetic offline time remains explicitly unstamped");f.page.set_offline_source(false,false);
    o.clock=MessageClock::None;f.page.configure_display(o);
}
void following(ViewFixture& f){
    f.page.reset();SendMessageW(f.page.follow_control(),BM_SETCHECK,BST_CHECKED,0);SetFocus(f.page.clear_control());
    std::string seed;for(unsigned i=0;i<400;++i)seed+="row "+std::to_string(i)+": "+std::string(160,'X')+"\r\n";
    f.page.append(seed);f.page.refresh();check(bottom(f.page.view()),"checked follow shows actual bottom with focus on another control");
    bool exact=true;for(unsigned i=0;i<8;++i){SendMessageW(f.page.view(),WM_VSCROLL,i%2?SB_BOTTOM:SB_TOP,0);Sleep(50);f.page.append("new row: "+std::to_string(i)+"\r\n");f.page.refresh();exact=bottom(f.page.view())&&exact;}
    check(exact,"new RX returns to bottom after manual top or bottom scrolling");check(GetFocus()==f.page.clear_control(),"following never steals keyboard focus");
    f.resize(430,650);check(bottom(f.page.view()),"narrow reflow follows the actual new bottom");
    f.page.append(std::string(16,'\0'));f.page.refresh();check(bottom(f.page.view()),"NUL notice/status relayout preserves follow-to-bottom");
    f.page.set_font_points(19);check(bottom(f.page.view()),"font reflow follows the actual new bottom");
    SendMessageW(f.page.follow_control(),BM_CLICK,0,0);SendMessageW(f.page.view(),WM_VSCROLL,SB_TOP,0);const auto first=SendMessageW(f.page.view(),EM_GETFIRSTVISIBLELINE,0,0);
    f.page.append("inspecting retained history\r\n");f.page.refresh();check(SendMessageW(f.page.view(),EM_GETFIRSTVISIBLELINE,0,0)==first,"follow off preserves inspected rows during RX");
    SendMessageW(f.page.follow_control(),BM_CLICK,0,0);check(bottom(f.page.view()),"re-enabling follow reaches bottom immediately without another RX");
    f.resize(1000,720);check(bottom(f.page.view()),"wider reflow stays at bottom");
}

struct ScrollProbe {
    HWND window;unsigned enabled_changes=0,text_changes=0,backward_steps=0,forward_steps=0,caret_scrolls=0,unsettled_redraws=0;
    bool rich=false;
    explicit ScrollProbe(HWND value):window(value){
        wchar_t name[64]{};GetClassNameW(window,name,64);rich=_wcsicmp(name,MSFTEDIT_CLASS)==0;
        if(!SetWindowSubclass(window,procedure,0x534352,reinterpret_cast<DWORD_PTR>(this)))throw std::runtime_error("Cannot observe native scroll state");
    }
    ~ScrollProbe(){if(IsWindow(window))RemoveWindowSubclass(window,procedure,0x534352);}
    int position()const{
        if(rich){POINT point{};SendMessageW(window,EM_GETSCROLLPOS,0,reinterpret_cast<LPARAM>(&point));return point.y;}
        SCROLLINFO info{sizeof(info),SIF_POS};GetScrollInfo(window,SB_VERT,&info);return info.nPos;
    }
    static LRESULT CALLBACK procedure(HWND h,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR data){
        auto& self=*reinterpret_cast<ScrollProbe*>(data);
        if(message==WM_ENABLE)++self.enabled_changes;
        if(message==WM_SETTEXT)++self.text_changes;
        if(message==EM_SCROLLCARET)++self.caret_scrolls;
        const bool observe=message==EM_REPLACESEL||message==EM_SCROLLCARET||message==WM_VSCROLL||message==EM_SETSEL;
        const int before=observe?self.position():0;
        const auto result=DefSubclassProc(h,message,w,l);
        if(message==WM_SETREDRAW&&w){SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};GetScrollInfo(h,SB_VERT,&info);if(info.nPos<std::max(info.nMin,info.nMax-static_cast<int>(info.nPage)+1)-1)++self.unsettled_redraws;}
        if(observe){const int after=self.position();if(after<before)++self.backward_steps;if(after>before)++self.forward_steps;}
        return result;
    }
};
void scroll_stability(ViewFixture& f){
    SetWindowLongPtrW(f.host,GWL_STYLE,GetWindowLongPtrW(f.host,GWL_STYLE)|WS_CLIPCHILDREN);
    const auto latest=GetDlgItem(f.page.hwnd(),153);
    check(latest!=nullptr,"native Return to latest button exists");
    for(unsigned variant=0;variant<3;++variant){
        if(variant==1)SendMessageW(f.page.hwnd(),WM_COMMAND,120,0);
        if(variant==2){SendMessageW(f.page.hwnd(),WM_COMMAND,120,0);MessageDisplayOptions options;options.ansi_sgr=true;f.page.configure_display(options);}
        f.resize(820,700);f.page.reset();SendMessageW(f.page.follow_control(),BM_SETCHECK,BST_CHECKED,0);
        std::string seed;for(unsigned i=0;i<300;++i)seed+="row "+std::to_string(i)+": "+std::string(60,'X')+"\r\n";
        f.page.append(seed);f.page.refresh();pump();SetFocus(variant==1?f.page.view():f.page.clear_control());
        check(bottom(f.page.view())&&!IsWindowEnabled(latest),"filled terminal starts at bottom with Return to latest disabled");
        HWND status=nullptr;
        for(auto child=GetWindow(f.page.hwnd(),GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))
            if(shown(child).find(L"New bytes:")!=std::wstring::npos)status=child;
        if(!status)throw std::runtime_error("Inspection status control missing");
        bool disabled=true,at_bottom=true,focus=true,exact=true;
        ScrollProbe button(latest),view(f.page.view()),inspection(status);
        for(unsigned i=0;i<18;++i){
            const auto prior_focus=GetFocus();const unsigned pacing[]{50,62,75};Sleep(pacing[i%3]);
            const std::string row=variant==2?"\x1b[31mnext colored row\x1b[0m\r\n":"next row: "+std::to_string(i)+"\r\n";
            if(i%2){MessageBuffer received;received.append(row);f.page.append_history(received);}else f.page.append(row);
            disabled=disabled&&!IsWindowEnabled(latest);pump();f.page.refresh();
            disabled=disabled&&!IsWindowEnabled(latest);at_bottom=at_bottom&&bottom(f.page.view());focus=focus&&GetFocus()==prior_focus;
            const auto exported=f.page.export_text();std::wstring expected(exported.begin(),exported.end());
            expected.erase(std::remove(expected.begin(),expected.end(),L'\r'),expected.end());
            exact=exact&&shown(f.page.view())==expected;
            pump();
        }
        std::cout<<"scroll_variant="<<variant<<" latest_enable_changes="<<button.enabled_changes<<" backward_steps="<<view.backward_steps<<" forward_steps="<<view.forward_steps<<" caret_scrolls="<<view.caret_scrolls<<" unsettled_redraws="<<view.unsettled_redraws<<'\n';
        check(disabled&&button.enabled_changes==0,"following RX never pulses Return to latest enabled before refresh");
        check(inspection.text_changes==0,"steady following never rewrites an unchanged inspection status");
        check(view.backward_steps==0,"steady terminal receive never scrolls the viewport backwards within an update");
        check(view.unsettled_redraws==0,"following redraw resumes only after final viewport positioning");
        check(at_bottom&&focus&&exact,"paced receive keeps bottom, native text and keyboard focus");
        const auto before_changes=button.text_changes;f.page.refresh();pump();
        check(button.text_changes==before_changes,"idle refresh leaves button caption unchanged");
    }
    SendMessageW(f.page.follow_control(),BM_CLICK,0,0);SendMessageW(f.page.view(),WM_VSCROLL,SB_TOP,0);
    const auto top=SendMessageW(f.page.view(),EM_GETFIRSTVISIBLELINE,0,0);
    f.page.append("browsed backlog\r\n");f.page.refresh();
    check(IsWindowEnabled(latest)&&SendMessageW(f.page.view(),EM_GETFIRSTVISIBLELINE,0,0)==top,"browsing keeps inspected position and Return to latest enabled");
    SendMessageW(latest,BM_CLICK,0,0);
    check(!IsWindowEnabled(latest)&&bottom(f.page.view()),"Return to latest resumes immediately and disables once caught up");
    SendMessageW(f.page.hwnd(),WM_COMMAND,122,0);const auto paused=shown(f.page.view());
    f.page.append("paused backlog\r\n");f.page.refresh();
    check(IsWindowEnabled(latest)&&shown(f.page.view())==paused,"paused receive retains display and enables Return to latest");
    SendMessageW(latest,BM_CLICK,0,0);
    check(!IsWindowEnabled(latest)&&bottom(f.page.view())&&shown(f.page.view()).find(L"paused backlog")!=std::wstring::npos,"Return to latest drains paused backlog without losing text");
    MessageDisplayOptions normal;f.page.configure_display(normal);f.page.reset();
    std::string block="retained row: "+std::string(240,'X')+"\r\n";block.resize(256,' ');
    std::string full;for(unsigned i=0;i<1024;++i)full+=block;
    f.page.append(full);f.page.refresh();pump();
    {ScrollProbe view(f.page.view());f.page.append("capacity rollover\r\n");f.page.refresh();pump();
     std::cout<<"retention_unsettled_redraws="<<view.unsettled_redraws<<'\n';
     check(view.unsettled_redraws==0&&bottom(f.page.view()),"retention rebuild reaches bottom before redraw is re-enabled");
     check(f.page.buffer().offset()>0&&f.page.buffer().bytes().size()<=MessageBuffer::kCapacity,"retention rollover remains bounded and preserves source bytes");}
}
}
void colored_stream(ViewFixture& f){
    f.resize(820,700);f.page.set_font_points(10);f.page.reset();
    const std::wstring definition=L"red,line,1,ERROR\ncyan,word,1,\u4e2d\u6587";std::vector<KeywordRule> rules;std::wstring error;parse_keyword_rules(definition,rules,error);f.page.set_keywords(definition,rules);
    MessageDisplayOptions options;options.encoding=MessageEncoding::Utf8;f.page.configure_display(options);
    f.page.append("unterminated");f.page.refresh();check(shown(f.page.view())==L"unterminated","full colored import never adds a source newline");f.page.reset();check(shown(f.page.view()).empty(),"empty full colored import stays empty");
    const std::wstring text=L"ERROR {\\rtf1} \u4e2d\u6587 \U0001F642\r\nnext\trow\r\n";f.page.append(message_utf8(text));f.page.refresh();
    if(shown(f.page.view())!=L"ERROR {\\rtf1} \u4e2d\u6587 \U0001F642\nnext\trow\n"){std::cout<<"colored_native_code_units=";for(const auto c:shown(f.page.view()))std::cout<<std::hex<<static_cast<unsigned>(c)<<',';std::cout<<std::dec<<'\n';}
    check(shown(f.page.view())==L"ERROR {\\rtf1} \u4e2d\u6587 \U0001F642\nnext\trow\n","colored stream preserves Unicode, raw RTF syntax, tabs and exact newlines");
    CHARFORMAT2W color{};color.cbSize=sizeof(color);SendMessageW(f.page.view(),EM_SETSEL,14,16);SendMessageW(f.page.view(),EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&color));
    check(color.crTextColor==RGB(80,230,230),"last overlapping keyword color wins after native stream");
    f.page.set_font_points(19);SendMessageW(f.page.view(),EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&color));const auto font_height=color.yHeight;
    f.page.append("ERROR tail\r\n");f.page.refresh();SendMessageW(f.page.view(),EM_SETSEL,0,1);SendMessageW(f.page.view(),EM_GETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&color));
    check((color.yHeight+10)/20==19&&(font_height+10)/20==19&&shown(f.page.view()).find(L"ERROR tail\n")!=std::wstring::npos,"colored tail append preserves selected point size and previous text");
    f.page.set_font_points(10);f.page.reset();parse_keyword_rules(L"cyan,word,1,a:1\nyellow,word,1,c:3",rules,error);f.page.set_keywords(L"cyan,word,1,a:1\nyellow,word,1,c:3",rules);options.clock=MessageClock::Local;f.page.configure_display(options);
    std::string line="a:1,b:2,c:3";line.resize(255,' ');line+='\n';std::string seed;for(int i=0;i<1024;++i)seed+=line;
    const auto began=std::chrono::steady_clock::now();f.page.append(seed);f.page.refresh();const auto filled=std::chrono::steady_clock::now();f.page.append(seed.substr(0,4096));f.page.refresh();const auto evicted=std::chrono::steady_clock::now();
    std::cout<<"colored_fill_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(filled-began).count()<<" colored_eviction_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(evicted-filled).count()<<'\n';
    check(f.page.buffer().bytes()==seed.substr(65536)+seed.substr(0,4096)&&f.page.buffer().offset()==65536&&bottom(f.page.view()),"full colored retention keeps exact quarter-evicted raw history and bottom follow");
    for(const unsigned undo_limit:{100U,0U}){
        f.page.reset();SendMessageW(f.page.view(),EM_SETUNDOLIMIT,undo_limit,0);f.page.append(seed);f.page.refresh();uint64_t maximum=0;const auto start=GetTickCount64();
        for(int i=0;i<40;++i){const auto frame=GetTickCount64();f.page.append(seed.substr(0,4096));f.page.refresh();maximum=std::max(maximum,GetTickCount64()-frame);}
        const auto undo=SendMessageW(f.page.view(),EM_CANUNDO,0,0);std::cout<<"colored_undo_limit="<<undo_limit<<" total_ms="<<GetTickCount64()-start<<" max_refresh_ms="<<maximum<<" can_undo="<<undo<<'\n';
        if(!undo_limit)check(!undo,"readonly colored display can disable synthetic undo history");
    }
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string(argv[1])=="--scroll-stability"){
        FontDesktop desktop;{ViewFixture view;scroll_stability(view);following(view);}desktop.close();return passed?0:1;
    }
    if(argc==2&&std::string(argv[1])=="--font-default"){
        FontDesktop desktop;{SessionFixture files;ViewFixture view;font_defaults(view,files);}desktop.close();return passed?0:1;
    }
    if(argc!=1)return 2;
    SessionFixture files;ViewFixture view;clocks(view,files);following(view);colored_stream(view);return passed?0:1;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
