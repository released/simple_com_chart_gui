#pragma once
#include <afxwin.h>
#include "capture_store.h"
#include "lab_metrics.h"
class DigitalView : public CWnd {
public:
    bool create(CWnd*);
    void update(CaptureSnapshot,std::vector<DigitalLane>,double window,double hold);
protected:
    LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
    DECLARE_MESSAGE_MAP()
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
private:
    CaptureSnapshot data_;
    std::vector<DigitalLane> lanes_;
    double window_=30,hold_=2;
    HFONT font_=nullptr;
};
