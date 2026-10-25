#pragma once
#include <afxwin.h>
#include "binary_inspection.h"
#include "traffic_store.h"
class BinaryInspector:public CDialog {
public:BinaryInspector(CWnd* owner,std::vector<TrafficRecord> frames,BinaryFrameConfig config);
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;BOOL OnCommand(WPARAM,LPARAM)override;BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    void OnOK()override{OnCancel();}
    afx_msg void OnSize(UINT,int,int);afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void rebuild();void layout();
    std::vector<TrafficRecord> frames_;BinaryFrameConfig config_;std::vector<BitField> bits_;
    std::vector<InspectionRow> rows_;std::vector<int> positions_;
    HWND pick_=nullptr,list_=nullptr,hex_=nullptr,status_=nullptr,edit_=nullptr,close_=nullptr;
    HFONT font_=nullptr,mono_=nullptr;std::wstring definitions_;
};
