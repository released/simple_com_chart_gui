#pragma once
#include <afxwin.h>
#include "workflow_waterfall.h"
class WaterfallView:public CWnd {
public:
    bool create(CWnd*);
    void clear(){history_.clear();Invalidate(FALSE);}
    void append(const WaterfallFrame&,size_t,double low,double high);
    const WaterfallHistory& history()const{return history_;}
protected:LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
private:WaterfallHistory history_;size_t depth_=128;double low_=-100,high_=0;
};
