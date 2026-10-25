#pragma once
#include <afxwin.h>
#include <array>
#include <vector>
#include "plot_view.h"
#include "capture_store.h"
class AdvancedTools : public CDialog {
public:
    static constexpr UINT kRequest = WM_APP + 8;
    enum Page { Replay, Profiles, Analysis, Automation, Extraction, Channels, FileTx, PageCount };
    struct Request { int page = 0, action = 0; std::vector<std::wstring> fields; };
    bool show(CWnd* owner, HFONT font, int page);
    void notice(int page, const std::wstring& value);
    void state(int page, const std::wstring& value);
    std::wstring field(int page, size_t index) const;
    void set_field(int page, size_t index, const std::wstring& value);
    void set_choices(int page,size_t index,const std::vector<std::wstring>& values);
    HWND field_hwnd(int page, size_t index) const;
    HWND action_hwnd(int page, size_t index) const;
    void show_analysis(const CaptureSnapshot& data, size_t column, double a, double b, double end, double window, bool reference, double threshold);
    void clear_analysis() { analysis_plot_.clear_analysis(); }
    const PlotView& analysis_plot() const { return analysis_plot_; }
    const CaptureSnapshot& analysis_snapshot() const {return analysis_data_;}
    void update_analysis_cursors(double a,double b,bool reference,double threshold) {analysis_plot_.set_analysis(a,b,reference,threshold);}
    bool verify_layout(const std::wstring& folder);
    bool verify_small_layout(const std::wstring& folder);
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM, LPARAM) override;
    BOOL OnNotify(WPARAM, LPARAM, LRESULT*) override;
    BOOL PreTranslateMessage(MSG*) override;
    void OnOK() override {}
    void OnCancel() override;
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnPlotCursor(WPARAM, LPARAM);
    afx_msg void OnVScroll(UINT, UINT, CScrollBar*);
    afx_msg BOOL OnMouseWheel(UINT, short, CPoint);
    afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
private:
    struct Field { std::wstring name, initial; std::vector<std::wstring> choices; bool multiline = false; HWND label = nullptr, control = nullptr; };
    void layout();
    void select(int page);
    HFONT font_ = nullptr;
    HWND tabs_ = nullptr, help_ = nullptr, status_ = nullptr;
    HWND content_=nullptr;
    int page_ = 0;
    bool selecting_ = false;
    bool searching_ = false;
    int scroll_ = 0, scroll_limit_ = 0;
    HFONT owned_font_ = nullptr;
    std::array<std::vector<Field>,PageCount> fields_;
    std::array<std::vector<HWND>,PageCount> actions_;
    std::array<std::wstring,PageCount> logs_, states_;
    ChannelModel analysis_model_;
    PlotView analysis_plot_;
    CaptureSnapshot analysis_data_;
};
