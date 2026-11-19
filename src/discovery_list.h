#pragma once
#include <afxwin.h>
#include <vector>
#include <string>
class DiscoveryListDialog:public CDialog {
public:
    using Rows=std::vector<std::vector<std::wstring>>;
    DiscoveryListDialog(CWnd*,std::wstring,std::vector<std::wstring>,Rows,std::wstring summary,std::wstring action=L"Open selected",std::wstring alternate=L"");
    size_t selection()const{return selected_;}
    bool alternate()const{return alternate_;}
protected:
    BOOL OnInitDialog()override;BOOL OnCommand(WPARAM,LPARAM)override;BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    void OnOK()override;
    afx_msg void OnSize(UINT,int,int);afx_msg void OnDestroy();afx_msg void OnGetMinMaxInfo(MINMAXINFO*);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    friend class CMainDialog;
    void rebuild();void layout();
    std::wstring title_,summary_,action_,alternate_caption_;std::vector<std::wstring> columns_;Rows rows_;std::vector<size_t> visible_;
    size_t selected_=SIZE_MAX;bool alternate_=false,ready_=false;HFONT font_=nullptr;
    HWND search_label_=nullptr,search_=nullptr,list_=nullptr,details_=nullptr,info_=nullptr,accept_=nullptr,other_=nullptr,cancel_=nullptr;
};
