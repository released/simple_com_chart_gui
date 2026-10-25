#pragma once
#include <afxwin.h>
#include <functional>
#include <array>
#include "signal_analysis.h"
class InstrumentCanvas:public CWnd {
public:std::function<void(CDC&,CRect)> draw;
protected:LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
};
class UartInstruments:public CDialog {
public:
    using Keys=std::function<std::vector<std::string>()>;
    using Series=std::function<std::vector<std::pair<double,double>>(const std::string&,size_t)>;
    void sources(Keys keys,Series series){keys_=std::move(keys);series_=std::move(series);}
    bool show(CWnd*,const std::wstring& ini);
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    void OnOK()override{}void OnCancel()override{ShowWindow(SW_HIDE);}
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void layout();void update(bool fft=false);void draw(CDC&,CRect);void settings();void choices();void persist(size_t);
    std::string key(size_t)const;
    InstrumentCanvas canvas_;
    Keys keys_;Series series_;
    std::array<HWND,5> picks_{};
    std::array<double,4> values_{};
    std::array<bool,4> valid_{};
    std::vector<std::string> known_;
    std::wstring ini_,fft_status_=L"Select an FFT channel, then Analyze. Raw source clocks are validated unless a known rate is explicitly set.";
    Spectrum spectrum_;
    double gauge_low_=0,gauge_high_=100,bar_low_=0,bar_high_=100,led_=0.5,rate_=0;
    size_t points_=256;
    HWND settings_=nullptr,analyze_=nullptr,automatic_=nullptr,hint_=nullptr;
    HFONT font_=nullptr;
    bool ready_=false;
};
