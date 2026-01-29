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
        return FALSE;
    }

    int ExitInstance() override {
        if (gdiplus_token_) {
            Gdiplus::GdiplusShutdown(gdiplus_token_);
            gdiplus_token_ = 0;
        }
        return CWinApp::ExitInstance();
    }

private:
    ULONG_PTR gdiplus_token_ = 0;
};

CChartApp theApp;
