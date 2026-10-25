#include "session_instance.h"

namespace {
constexpr wchar_t kClass[] = L"SimpleComChart.SessionActivation.v1";
struct Target { std::wstring identity; HWND window = nullptr; };
BOOL CALLBACK find_target(HWND window, LPARAM value) {
    auto& target = *reinterpret_cast<Target*>(value);
    wchar_t name[128]{}, title[256]{};
    GetClassNameW(window,name,128);
    if (wcscmp(name,kClass)) return TRUE;
    GetWindowTextW(window,title,256);
    if (target.identity == title) { target.window = window; return FALSE; }
    return TRUE;
}
}
SessionInstance::~SessionInstance() {
    if (window_) DestroyWindow(window_);
    if (event_) CloseHandle(event_);
    if (owns_) ReleaseMutex(mutex_);
    if (mutex_) CloseHandle(mutex_);
}
SessionInstance::Result SessionInstance::acquire(const std::wstring& identity,std::wstring& error) {
    error.clear();
    const auto base=L"Local\\Released.SimpleComChart."+identity;
    mutex_=CreateMutexW(nullptr,FALSE,(base+L".Owner").c_str());
    if (!mutex_) { error=L"Cannot acquire the session instance guard: "+std::to_wstring(GetLastError()); return Result::Failed; }
    const auto wait=WaitForSingleObject(mutex_,0);
    owns_=wait==WAIT_OBJECT_0 || wait==WAIT_ABANDONED;
    if (!owns_ && wait!=WAIT_TIMEOUT) { error=L"Session guard wait failed."; return Result::Failed; }
    event_=CreateEventW(nullptr,TRUE,FALSE,(base+L".Ready").c_str());
    if (!event_) { error=L"Cannot create the instance readiness signal."; return Result::Failed; }
    activate_=RegisterWindowMessageW((base+L".Activate").c_str());
    if (!activate_) { error=L"Cannot register instance activation."; return Result::Failed; }
    if (!owns_) {
        // Never fall through to another production window, including readiness failure.
        if (WaitForSingleObject(event_,5000)!=WAIT_OBJECT_0) { error=L"The existing tool is starting or stopping. Please retry shortly."; return Result::Failed; }
        Target target{identity}; EnumWindows(find_target,reinterpret_cast<LPARAM>(&target));
        DWORD_PTR response=0;
        if (!target.window || !SendMessageTimeoutW(target.window,activate_,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,2000,&response) || !response) {
            error=L"The existing tool cannot be activated right now. No second instance was started."; return Result::Failed;
        }
        return Result::Activated;
    }
    ResetEvent(event_);
    WNDCLASSW type{}; type.lpfnWndProc=procedure; type.hInstance=GetModuleHandleW(nullptr); type.lpszClassName=kClass;
    if (!RegisterClassW(&type) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) { error=L"Cannot register instance activation window."; return Result::Failed; }
    window_=CreateWindowW(kClass,identity.c_str(),WS_POPUP,0,0,0,0,nullptr,nullptr,type.hInstance,this);
    if (!window_) { error=L"Cannot create instance activation window."; return Result::Failed; }
    return Result::Owner;
}
void SessionInstance::ready(HWND main) { main_=main; if (IsWindow(main_)) SetEvent(event_); }
LRESULT CALLBACK SessionInstance::procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    if (message==WM_NCCREATE) SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
    const auto self=reinterpret_cast<SessionInstance*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if (self && message==self->activate_) {
        if (!IsWindow(self->main_)) return 0;
        HWND target=GetLastActivePopup(self->main_);
        if (!IsWindowVisible(target)) target=self->main_;
        if (IsIconic(self->main_)) ShowWindow(self->main_,SW_RESTORE);
        if (IsIconic(target)) ShowWindow(target,SW_RESTORE);
        // Do not dismiss a modal dialog, change selection, or issue any operation.
        SetForegroundWindow(target);
        FLASHWINFO flash{sizeof(flash),target,FLASHW_TRAY,2,0}; FlashWindowEx(&flash);
        return 1;
    }
    return DefWindowProcW(window,message,w,l);
}
