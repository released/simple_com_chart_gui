#pragma once
#include <afxwin.h>
#include <functional>
#include <string>
#include <vector>

class UartFormViewport : public CWnd {
protected:LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
};
class UartToolForm : public CDialog {
public:
    struct Field {std::wstring label,value;std::vector<std::wstring> choices;int rows=1;};
    using Preview=std::function<std::wstring(const std::vector<std::wstring>&)>;
    using Validate=std::function<bool(const std::vector<std::wstring>&,std::wstring&)>;
    UartToolForm(std::wstring title,std::wstring help,std::vector<Field> fields,Preview preview={},Validate validate={},CWnd* owner=nullptr);
    std::vector<std::wstring> values()const;
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    void OnOK()override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnVScroll(UINT,UINT,CScrollBar*);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void layout();
    void scroll(int);
    std::wstring title_,help_;
    std::vector<Field> fields_;
    std::vector<HWND> labels_,inputs_;
    Preview preview_;
    Validate validate_;
    UartFormViewport viewport_;
    HWND help_label_=nullptr,result_=nullptr,preview_button_=nullptr,apply_=nullptr,cancel_=nullptr;
    HFONT font_=nullptr;
    int offset_=0,content_=0,row_=28;
    bool ready_=false;
};
