#pragma once
#include <afxwin.h>
#include "monitor_features.h"

class MonitorCenter : public CDialog {
public:
    static constexpr UINT kRequest=WM_APP+35;
    enum Action { Stage,Folder,Rename,Remove,ClearEvents,Flash,Add };
    struct Request {Action action=Stage;RecentResource resource;std::wstring name;bool flag=false;};
    MonitorCenter();
    bool show(CWnd*,bool events=false);
    void update(const std::vector<RecentResource>&,const MonitorEvents&,bool flash);
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    void OnOK()override {}
    void OnCancel()override {ShowWindow(SW_HIDE);}
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void layout();
    void rebuild();
    void selected();
    HWND mode_=nullptr,list_=nullptr,details_=nullptr,name_=nullptr,flash_=nullptr;
    std::vector<HWND> buttons_;
    std::vector<RecentResource> recent_;
    std::vector<MonitorEvent> events_;
    HFONT font_=nullptr;
    uint64_t event_revision_=UINT64_MAX;
    bool ready_=false;
};
