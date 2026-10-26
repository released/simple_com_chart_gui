#pragma once
#include <afxwin.h>
#include "time_calculus.h"
class CalculusView:public CWnd {
public:bool create(CWnd*);void update(std::shared_ptr<const CalculusResult> r){data_=std::move(r);Invalidate(FALSE);}
protected:LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
private:void paint();HFONT font_=nullptr;std::shared_ptr<const CalculusResult> data_;
};
