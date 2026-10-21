#include "mfc_main_dialog.h"

class CChartApp : public CWinApp {
public:
    BOOL InitInstance() override {
        Gdiplus::GdiplusStartupInput gdiplus_input;
        Gdiplus::GdiplusStartup(&gdiplus_token_, &gdiplus_input, nullptr);

        CWinApp::InitInstance();
        CMainDialog dlg;
        m_pMainWnd = &dlg;
        dlg.DoModal();
        smoke_exit_code_ = dlg.smoke_exit_code();
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
    ULONG_PTR gdiplus_token_ = 0;
    int smoke_exit_code_ = 0;
};

CChartApp theApp;
