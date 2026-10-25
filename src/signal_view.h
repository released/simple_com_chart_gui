#pragma once
#include "uart_instruments.h"
#include "run_analysis.h"
#include "background_job.h"
#include <set>
class SignalView:public CDialog {
public:
    using Snapshot=std::function<CaptureSnapshot()>;using Options=std::function<RunOptions()>;using Project=std::function<double(const std::string&,double)>;
    void sources(Snapshot snapshot,Options options,Project project){snapshot_=std::move(snapshot);options_=std::move(options);project_=std::move(project);}
    bool show(CWnd*,const std::wstring& ini);
    void poll();void shutdown(){closing_=true;loader_.cancel();saver_.cancel();}
    void resume(){closing_=false;}
    bool busy()const{return loader_.busy()||saver_.busy();}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;BOOL OnCommand(WPARAM,LPARAM)override;
    void OnOK()override{}void OnCancel()override;
    afx_msg void OnTimer(UINT_PTR);afx_msg void OnSize(UINT,int,int);afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    void layout();void refresh(bool choices=true);void settings();void load(size_t);bool begin_load(size_t,const std::filesystem::path&);void save();void draw(CDC&,CRect);
    std::string key(HWND)const;void persist_xy();
    Snapshot snapshot_;Options options_;Project project_;
    std::wstring ini_,status_text_;
    std::array<std::wstring,2> xy_choices_;
    std::set<std::string> known_;
    std::array<RecordedRun,2> runs_;
    struct Loaded {size_t slot=0;RecordedRun run;};
    BackgroundJob<Loaded> loader_;BackgroundJob<int> saver_;
    RunComparison comparison_;
    std::array<std::vector<std::pair<double,double>>,2> points_;
    InstrumentCanvas canvas_;HFONT font_=nullptr;
    HWND mode_=nullptr,x_=nullptr,y_=nullptr,hint_=nullptr,status_=nullptr,connect_=nullptr;
    std::vector<HWND> buttons_;
    double shift_=0,max_age_=0.05,tolerance_=0;
    int mode_value_=-1;bool ready_=false,closing_=false;
};
