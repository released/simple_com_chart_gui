#include "mfc_main_dialog.h"
#include "session_instance.h"
#include <filesystem>
#include "ui_language.h"

class CChartApp : public CWinApp {
public:
    void ready(HWND main) { instance_.ready(main); }
    BOOL InitInstance() override {
        std::wstring identity=L"UARTChart.v1", error;
        const std::wstring command=m_lpCmdLine;
        // Demonstration mode is not a supported application entry point.
        if (command.find(L"--demo")!=std::wstring::npos) { smoke_exit_code_=2; return FALSE; }
#ifdef CHART_RUNTIME_CHECKS
        wchar_t executable[MAX_PATH]{}; GetModuleFileNameW(nullptr,executable,MAX_PATH);
        // Only noninteractive verification copies get a private namespace. No normal
        // launch flag bypasses the stable production identity.
        const std::wstring path=std::filesystem::path(executable).parent_path().wstring();
        if (command.find(L"--smoke-test")!=std::wstring::npos && path.find(L"\\build\\verify\\")!=std::wstring::npos)
            identity=L"Fixture."+std::to_wstring(std::hash<std::wstring>{}(path));
#else
        // Fail closed rather than silently running a normal session as a test.
        // scripts/verify_runtime.ps1 builds the separate verification executable.
        if (command.find(L"--smoke-test")!=std::wstring::npos) { smoke_exit_code_=2; return FALSE; }
#endif
        const auto result=instance_.acquire(identity,error);
        if (result!=SessionInstance::Result::Owner) {
            if (result==SessionInstance::Result::Failed) { MessageBoxW(nullptr,error.c_str(),L"Simple COM Chart",MB_OK|MB_ICONWARNING); smoke_exit_code_=2; }
            return FALSE;
        }
        Gdiplus::GdiplusStartupInput gdiplus_input;
        Gdiplus::GdiplusStartup(&gdiplus_token_, &gdiplus_input, nullptr);

        CWinApp::InitInstance();
        wchar_t language_executable[32768]{};
        GetModuleFileNameW(nullptr,language_executable,32768);
        ui::initialize_language(language_executable);
        CMainDialog dlg;
        m_pMainWnd = &dlg;
        dlg.DoModal();
#ifdef CHART_RUNTIME_CHECKS
        smoke_exit_code_ = dlg.smoke_exit_code();
#endif
        return FALSE;
    }

    int ExitInstance() override {
        if (gdiplus_token_) {
            Gdiplus::GdiplusShutdown(gdiplus_token_);
            gdiplus_token_ = 0;
        }
        CWinApp::ExitInstance();
        return smoke_exit_code_;
    }

private:
    SessionInstance instance_;
    ULONG_PTR gdiplus_token_ = 0;
    int smoke_exit_code_ = 0;
};

CChartApp theApp;
void publish_main_instance(HWND main) { theApp.ready(main); }
