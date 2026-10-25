#pragma once
#include <afxwin.h>
#include "command_panel.h"
#include "step_script.h"
#include "background_job.h"
#include <array>
class UartCommandWindow : public CDialog {
public:
    using Send=std::function<bool(size_t,std::string,std::wstring&)>;
    using Start=std::function<bool(size_t,std::vector<ScriptStep>,std::wstring&)>;
    bool show(CWnd*,const std::wstring& ini,Send,Start,std::function<void(size_t)> stop,std::function<uint64_t(size_t)> generation);
    void poll();
    void state(size_t,const std::wstring&);
    void reports(std::function<std::string(size_t)> value){report_=std::move(value);}
    void notice(const std::wstring&);
    bool busy()const{return files_.busy();}
    void shutdown();
    void resume(){closing_=false;}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    BOOL OnCommand(WPARAM,LPARAM)override;
    void OnOK()override{}
    void OnCancel()override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg void OnDestroy();
    afx_msg void OnHScroll(UINT,UINT,CScrollBar*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
private:
    friend class CMainDialog;
    enum {Mode=5000,Port,Group,List,Value,Minus,Plus,Slider,New,Edit,Remove,Up,Down,Import,Export,SendNow,Stop,SaveResults};
    void layout();
    void rebuild(int selected=-1);
    void selection();
    void edit(bool create);
    void save_default(int kind=-1);
    void file(bool importing,bool defaults=false,int kind=-1);
    void transmit();
    int selected()const;
    bool steps_mode()const;
    size_t port()const;
    std::vector<PanelCommand> commands_;
    std::vector<ScriptStep> steps_;
    std::vector<size_t> indexes_;
    std::array<std::wstring,2> states_;
    struct FileResult {bool loaded=false,script=false;std::vector<PanelCommand> commands;std::vector<ScriptStep> steps;};
    BackgroundJob<FileResult> files_;
    Send send_;Start start_;std::function<void(size_t)> stop_;
    std::function<uint64_t(size_t)> generation_;
    std::function<std::string(size_t)> report_;
    std::wstring ini_;
    HWND mode_=nullptr,port_=nullptr,group_=nullptr,list_=nullptr,value_=nullptr,slider_=nullptr,status_=nullptr,hint_=nullptr;
    std::vector<HWND> buttons_;
    HFONT font_=nullptr;
    bool ready_=false,loading_=false,default_second_=false,closing_=false;
};
