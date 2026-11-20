#pragma once
#include <windows.h>
#include <algorithm>

inline RECT discovery_work_area(HWND owner,const RECT* proposed=nullptr) {
    MONITORINFO monitor{sizeof(monitor)};
    const auto handle=proposed?MonitorFromRect(proposed,MONITOR_DEFAULTTONEAREST):MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST);
    if(GetMonitorInfoW(handle,&monitor))return monitor.rcWork;
    RECT work{};
    if(!SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0))work={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
    return work;
}
inline RECT fit_discovery_bounds(RECT desired,const RECT& work) {
    const LONG width=std::max<LONG>(1,work.right-work.left),height=std::max<LONG>(1,work.bottom-work.top);
    const LONG mx=std::min<LONG>(8,(width-1)/2),my=std::min<LONG>(8,(height-1)/2);
    const LONG w=std::clamp<LONG>(desired.right-desired.left,std::min<LONG>(800,width-2*mx),width-2*mx),h=std::clamp<LONG>(desired.bottom-desired.top,std::min<LONG>(620,height-2*my),height-2*my);
    const LONG x=std::clamp<LONG>(desired.left,work.left+mx,work.right-mx-w),y=std::clamp<LONG>(desired.top,work.top+my,work.bottom-my-h);
    return {x,y,x+w,y+h};
}
