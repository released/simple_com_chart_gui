#include "mfc_main_dialog.h"
#include "dpi_support.h"

LRESULT CMainDialog::OnDpiChanged(WPARAM,LPARAM value) {
    if (!ui_ready_ || !value) return 0;
    const bool enlarged=std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos;
    const auto font=make_ui_font(m_hWnd,enlarged), buttons=make_ui_font(m_hWnd,enlarged);
    if (!font || !buttons) { if(font) DeleteObject(font); if(buttons) DeleteObject(buttons); return 0; }
    const auto previous=ui_font_, previous_buttons=btn_font_;
    ui_font_=font; btn_font_=buttons; set_child_fonts(m_hWnd,font);
    for (const auto button:{btn_connect_,btn_refresh_,btn_fit_,btn_snapshot_,btn_overlay_}) ::SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(buttons),TRUE);
    channel_panel_.set_ui_font(font); data_page_.set_ui_font(font); message_page_.set_ui_font(font);
    hover_help_.destroy(); build_hover_help(font);
    if (previous) DeleteObject(previous); if (previous_buttons) DeleteObject(previous_buttons);
    const auto& bounds=*reinterpret_cast<const RECT*>(value);
    SetWindowPos(nullptr,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOZORDER|SWP_NOACTIVATE);
    CRect client; GetClientRect(&client); layout_ui(client.Width(),client.Height());
    RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN); return 0;
}
