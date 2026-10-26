#include <vector>
#include "ui_localized_controls.h"
#include <memory>
namespace ui {
namespace {
bool named(HWND h,const wchar_t* name){wchar_t type[80]{};GetClassNameW(h,type,80);return _wcsicmp(type,name)==0;}
LRESULT CALLBACK caption_proc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR){
    if(m==WM_SETTEXT&&l){const auto value=display_caption(reinterpret_cast<LPCWSTR>(l));return DefSubclassProc(h,m,w,reinterpret_cast<LPARAM>(value.c_str()));}
    if((m==TCM_INSERTITEMW||m==TCM_SETITEMW)&&l){auto item=*reinterpret_cast<TCITEMW*>(l);if(item.mask&TCIF_TEXT)item.pszText=const_cast<wchar_t*>(caption(item.pszText));return DefSubclassProc(h,m,w,reinterpret_cast<LPARAM>(&item));}
    if(m==WM_NCDESTROY)RemoveWindowSubclass(h,caption_proc,id);return DefSubclassProc(h,m,w,l);
}
struct Choice {std::vector<std::wstring> original;};
constexpr UINT_PTR choice_id=0x53434C;
LRESULT CALLBACK choice_proc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR id,DWORD_PTR data){auto& state=*reinterpret_cast<Choice*>(data);
    if(m==CB_ADDSTRING&&l){const auto source=reinterpret_cast<LPCWSTR>(l);const auto index=DefSubclassProc(h,m,w,reinterpret_cast<LPARAM>(caption(source)));if(index>=0){if(static_cast<size_t>(index)>state.original.size())return index;state.original.insert(state.original.begin()+index,source);}return index;}
    if(m==CB_RESETCONTENT)state.original.clear();
    if(m==CB_DELETESTRING&&w<state.original.size())state.original.erase(state.original.begin()+w);
    if(m==CB_FINDSTRINGEXACT&&l){const auto value=reinterpret_cast<LPCWSTR>(l);for(size_t i=0;i<state.original.size();++i)if(state.original[i]==value)return static_cast<LRESULT>(i);}
    if(m==WM_SETTEXT&&l){const auto value=reinterpret_cast<LPCWSTR>(l);for(size_t i=0;i<state.original.size();++i)if(state.original[i]==value){DefSubclassProc(h,CB_SETCURSEL,i,0);return TRUE;}}
    if(m==WM_NCDESTROY){RemoveWindowSubclass(h,choice_proc,id);delete &state;}
    return DefSubclassProc(h,m,w,l);
}
void label(HWND h){if(named(h,L"Static")||named(h,L"Button")||named(h,WC_TABCONTROLW)){
    wchar_t text[32769]{};GetWindowTextW(h,text,32769);const auto translated=display_caption(text);if(translated!=text)SetWindowTextW(h,translated.c_str());SetWindowSubclass(h,caption_proc,1,0);}}
}
HWND create_window_ex(DWORD ex,LPCWSTR cls,LPCWSTR text,DWORD style,int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,LPVOID data){
    const auto h=::CreateWindowExW(ex,cls,text,style,x,y,width,height,parent,menu,instance,data);if(h)label(h);return h;
}
HWND create_window(LPCWSTR cls,LPCWSTR text,DWORD style,int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,LPVOID data){return create_window_ex(0,cls,text,style,x,y,width,height,parent,menu,instance,data);}
void localize_dialog(HWND h){wchar_t title[512]{};GetWindowTextW(h,title,512);SetWindowTextW(h,caption(title));SetWindowSubclass(h,caption_proc,1,0);EnumChildWindows(h,[](HWND child,LPARAM)->BOOL{label(child);return TRUE;},0);localize_menu(GetMenu(h));}
void localize_menu(HMENU menu){if(!IsMenu(menu))return;for(int i=0;i<GetMenuItemCount(menu);++i){wchar_t text[512]{};MENUITEMINFOW item{sizeof(item)};item.fMask=MIIM_STRING|MIIM_SUBMENU;item.dwTypeData=text;item.cch=512;if(GetMenuItemInfoW(menu,i,TRUE,&item)){const auto translated=tr(text);if(translated!=text){MENUITEMINFOW changed{sizeof(changed)};changed.fMask=MIIM_STRING;changed.dwTypeData=const_cast<wchar_t*>(translated.c_str());SetMenuItemInfoW(menu,i,TRUE,&changed);}localize_menu(item.hSubMenu);}}}
void choices(HWND h){DWORD_PTR unused=0;if(GetWindowSubclass(h,choice_proc,choice_id,&unused))return;auto state=std::make_unique<Choice>();if(SetWindowSubclass(h,choice_proc,choice_id,reinterpret_cast<DWORD_PTR>(state.get())))state.release();}
std::wstring control_value(HWND h){DWORD_PTR data=0;if(GetWindowSubclass(h,choice_proc,choice_id,&data)){const auto selected=SendMessageW(h,CB_GETCURSEL,0,0);const auto& values=reinterpret_cast<Choice*>(data)->original;if(selected>=0&&static_cast<size_t>(selected)<values.size())return values[static_cast<size_t>(selected)];}std::wstring out(static_cast<size_t>(GetWindowTextLengthW(h))+1,0);GetWindowTextW(h,out.data(),static_cast<int>(out.size()));out.pop_back();return out;}
int message_box(HWND owner,LPCWSTR text,LPCWSTR title,UINT flags){const auto body=tr(text?text:L""),caption_text=tr(title?title:L"");return ::MessageBoxW(owner,body.c_str(),caption_text.c_str(),flags);}
}
