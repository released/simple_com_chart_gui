#pragma once
#include "uart_tool_form.h"
#include "digital_view.h"
#include "calculus_view.h"
#include "waterfall_view.h"
#include <array>
class UartLabWindow : public CDialog {
public:
    enum Page {Diagnostics,Recording,Packet,Quality,Parser,Schema,Digital,Sessions,Devices,Input,Checksums,Calculus,Transactions,Views,Images,Notes,Reports,Waterfall,PageCount};
    static constexpr int FieldId=10000,ActionId=20000;
    struct Request {int page=0,action=0;std::vector<std::wstring> fields;};
    static constexpr UINT kRequest=WM_APP+28;
    bool show(CWnd*);
    void select(int);
    void notice(int,const std::wstring&);
    std::vector<std::wstring> values(int)const;
    void field(int,size_t,const std::wstring&);
    void file_busy(bool);
    bool active(int p)const{return GetSafeHwnd()&&IsWindowVisible()&&page_==p;}
    void digital(CaptureSnapshot d,std::vector<DigitalLane> l,double w,double h){const auto count=l.size();digital_.update(std::move(d),std::move(l),w,h);if(count!=digital_lane_count_){digital_lane_count_=count;layout();}}
    void calculus(std::shared_ptr<const CalculusResult> result){calculus_.update(std::move(result));}
    void spectrum(const WaterfallFrame& f,size_t n,double low,double high){waterfall_.append(f,n,low,high);}
    void clear_waterfall(){waterfall_.clear();}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    BOOL PreTranslateMessage(MSG*)override;
    void OnOK()override{}
    void OnCancel()override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnVScroll(UINT,UINT,CScrollBar*);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    struct Field {std::wstring name,initial;std::vector<std::wstring> choices;int rows=1;HWND label=nullptr,input=nullptr;};
    void build_pages();
    void layout();
    void scroll_to(int);
    UartFormViewport viewport_;
    DigitalView digital_;
    CalculusView calculus_;
    WaterfallView waterfall_;
    HWND page_selector_=nullptr,help_=nullptr,result_=nullptr;
    HFONT font_=nullptr;
    std::array<std::vector<Field>,PageCount> fields_;
    std::array<std::vector<HWND>,PageCount> actions_;
    std::array<std::wstring,PageCount> reports_;
    int page_=0,scroll_=0,height_=0,row_=28;
    size_t digital_lane_count_=0;
    bool ready_=false;
};
