#pragma once
#include <afxwin.h>
#include <commctrl.h>
#include "json_numeric.h"
class JsonTreeDialog:public CDialog {
public:
    JsonTreeDialog(CWnd* owner,std::vector<JsonNumericNode> nodes);
    const std::string& selection()const{return selection_;}
protected:
    BOOL OnInitDialog()override;
    void OnOK()override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()
private:
    friend class CMainDialog;
    void layout();
    std::vector<JsonNumericNode> nodes_;
    std::vector<HTREEITEM> items_;
    std::string selection_;
    HWND tree_=nullptr,info_=nullptr,apply_=nullptr,cancel_=nullptr;
    HFONT font_=nullptr;
};
