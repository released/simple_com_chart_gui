#pragma once
#include <afxwin.h>
#include <string>
#include <vector>
#include <functional>
class WorkflowGrid : public CDialog {
public:
    using Rows=std::vector<std::vector<std::wstring>>;
    using Validator=std::function<void(const Rows&)>;
    WorkflowGrid(std::wstring title,std::vector<std::wstring> columns,Rows rows,Validator validate={},CWnd* owner=nullptr);
    const Rows& rows()const{return rows_;}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    void OnOK()override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void layout();void refresh();bool edit(bool insert);void selected();
    std::wstring title_;std::vector<std::wstring> columns_;Rows rows_;Validator validate_;
    HWND list_=nullptr,status_=nullptr,details_=nullptr,ok_=nullptr,cancel_=nullptr;
    std::vector<HWND> inputs_,labels_,actions_;HFONT font_=nullptr;
    int selection_=-1;bool ready_=false,refreshing_=false;
};
