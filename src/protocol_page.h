#pragma once
#include <afxwin.h>
#include <functional>
#include <vector>
#include <string>

class ProtocolPage : public CWnd {
public:
    enum class Mode { Modbus, Lin };
    using Rows=std::vector<std::vector<std::wstring>>;
    bool create(CWnd*,Mode,HFONT,UINT,std::function<void(unsigned)>);
    void set_font(HFONT);
    void set_rows(Rows,bool preserve_selection=true);
    void status(const std::wstring&);
    void summary(const std::wstring&);
    void state(bool busy,bool running=false,bool has_rows=false);
    void reveal_focus();
    size_t target()const;
    bool allow_unknown()const;
    int selected()const;
    HWND control(unsigned id)const{return ::GetDlgItem(m_hWnd,static_cast<int>(id));}
    const Rows& rows()const{return rows_;}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnCommand(WPARAM,LPARAM)override;
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*)override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg BOOL OnEraseBkgnd(CDC*);
    afx_msg void OnVScroll(UINT,UINT,CScrollBar*);
private:
    void layout();
    Mode mode_=Mode::Modbus;
    HFONT font_=nullptr;
    HWND list_=nullptr,help_=nullptr,status_=nullptr,target_=nullptr,unknown_=nullptr;
    std::vector<HWND> toolbar_;
    Rows rows_;
    std::function<void(unsigned)> action_;
    int scroll_=0,content_height_=0;
    bool laying_out_=false;
    std::wstring summary_,notice_;
    void update_status();
};
