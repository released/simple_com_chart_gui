#include "session_fixture.h"
#include "ui_language.h"
#include "ui_localized_controls.h"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const char* name){if(!b)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
template<class F> bool rejects(F f){try{f();}catch(...){return true;}return false;}
std::wstring shown(HWND h){std::wstring s(static_cast<size_t>(GetWindowTextLengthW(h))+1,0);GetWindowTextW(h,s.data(),static_cast<int>(s.size()));s.pop_back();return s;}
}
int main(){try{
    const auto catalog_root=std::filesystem::path(__FILE__).parent_path().parent_path()/L"src"/L"i18n";
    SessionFixture f;const auto en=uart_read_file(catalog_root/L"lang_en.ini",2*1024*1024,f.cancel),tw=uart_read_file(catalog_root/L"lang_tw.ini",2*1024*1024,f.cancel);
    const auto a=ui::parse_catalog(en,"en"),b=ui::parse_catalog(tw,"tw");check(a.texts.size()==b.texts.size()&&b.texts.size()>=499,"paired native catalogs have matching bounded entry counts");
    check(std::all_of(a.texts.begin(),a.texts.end(),[&](const auto& row){return b.texts.count(row.first)==1;}),"every catalog ID exists in both languages");
    check(ui::translate(a,L"Connect")==L"Connect"&&ui::translate(b,L"Connect")==L"連線","catalog translates an actual serial control caption");
    check(ui::translate(b,L"Unregistered source string")==L"Unregistered source string","missing entry keeps English source");
    check(rejects([&]{ui::parse_catalog(tw,"en");})&&rejects([&]{ui::parse_catalog(tw+"\n"+ui::text_id(L"Connect")+"=duplicate\n","tw");}),"wrong language and duplicate IDs fail closed");
    check(rejects([]{ui::parse_catalog("[Language]\nVersion=1\nCode=tw\n[Strings]\nbad=x\n","tw");})&&rejects([]{ui::parse_catalog(std::string(2*1024*1024+1,'X'),"tw");}),"malformed IDs and oversized catalogs rejected");
    check(rejects([]{ui::parse_catalog(std::string("\xFF",1),"tw");}),"invalid UTF-8 rejected before display");
    const auto ini=f.file(L"app.ini");f.write(L"app.ini",std::string("\xFF\xFE",2));check(ui::saved_language(ini)=="tw","missing language preference defaults to Traditional Chinese");
    WritePrivateProfileStringW(L"Private",L"Keep",L"preserve",ini.c_str());check(ui::save_language(ini,"en")&&ui::saved_language(ini)=="en"&&!ui::save_language(ini,"../tw"),"explicit en/tw preference only; path-like input rejected");
    wchar_t keep[32]{};GetPrivateProfileStringW(L"Private",L"Keep",L"",keep,32,ini.c_str());check(std::wstring(keep)==L"preserve","saving language preserves unrelated INI values");
    SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_READONLY);const bool blocked=!ui::save_language(ini,"tw");SetFileAttributesW(ini.c_str(),FILE_ATTRIBUTE_NORMAL);check(blocked,"read-only INI reports save failure");
    f.write(L"lang_en.ini",en);f.write(L"lang_tw.ini",tw);check(ui::save_language(ini,"tw"),"Traditional Chinese selection saves");ui::initialize_language(f.root/L"app.exe");check(ui::active_language()=="tw"&&ui::tr(L"Connect")==L"連線","next launch loads saved catalog from the EXE folder");
    INITCOMMONCONTROLSEX common{sizeof(common),ICC_STANDARD_CLASSES|ICC_TAB_CLASSES};InitCommonControlsEx(&common);
    const auto parent=CreateWindowExW(0,L"STATIC",L"Language fixture",WS_POPUP,0,0,300,200,nullptr,nullptr,nullptr,nullptr);check(parent!=nullptr,"owned invisible native fixture creates");
    const auto label=ui::create_window(L"BUTTON",L"Connect",WS_CHILD,0,0,100,24,parent,nullptr,nullptr,nullptr);check(shown(label)==L"連線","native caption uses Traditional Chinese");SetWindowTextW(label,L"Disconnect");check(shown(label)==L"中斷連線","later caption updates use the same catalog");
    const auto edit=ui::create_window(L"EDIT",L"Connect",WS_CHILD,0,30,100,24,parent,nullptr,nullptr,nullptr);check(shown(edit)==L"Connect","UART/user input text is never translated");
    const auto combo=ui::create_window(L"COMBOBOX",L"",WS_CHILD|CBS_DROPDOWNLIST,0,60,180,180,parent,nullptr,nullptr,nullptr);ui::choices(combo);SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Port A"));SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Port B"));SendMessageW(combo,CB_SETCURSEL,1,0);check(shown(combo)==L"連接埠 B"&&ui::control_value(combo)==L"Port B","translated dropdown retains exact canonical backend value");
    SendMessageW(combo,CB_RESETCONTENT,0,0);SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"Disabled"));SetWindowTextW(combo,L"Disabled");check(ui::control_value(combo)==L"Disabled","reset/set selection preserve canonical choice mapping");DestroyWindow(parent);
    check(ui::save_language(ini,"en")&&ui::active_language()=="tw","saving next-launch language does not mutate the live catalog");ui::initialize_language(f.root/L"app.exe");check(ui::active_language()=="en"&&ui::tr(L"Connect")==L"Connect","restart selects saved English");
    ui::save_language(ini,"tw");uart_write_file(f.root/L"lang_tw.ini","invalid",true,f.cancel);ui::initialize_language(f.root/L"app.exe");check(ui::active_language()=="en"&&!ui::language_notice().empty(),"invalid selected language falls back to English with a notice");
    f.cleanup();return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
