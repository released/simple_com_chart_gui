#include "json_tree_dialog.h"
#include "resource.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include "dpi_support.h"
#include "discovery_geometry.h"
#include "uart_files.h"
#include <commctrl.h>
#include <algorithm>
BEGIN_MESSAGE_MAP(JsonTreeDialog,CDialog)
 ON_WM_SIZE()
 ON_WM_DESTROY()
 ON_WM_GETMINMAXINFO()
 ON_MESSAGE(WM_DPICHANGED,&JsonTreeDialog::OnDpiChanged)
END_MESSAGE_MAP()
JsonTreeDialog::JsonTreeDialog(CWnd* owner,std::vector<JsonNumericNode> nodes):CDialog(IDD_UART_TOOL_FORM,owner),nodes_(std::move(nodes)){}
BOOL JsonTreeDialog::OnInitDialog(){
    CDialog::OnInitDialog();ModifyStyle(0,WS_CLIPCHILDREN);SetWindowTextW(ui::caption(L"JSON numeric paths"));
    font_=make_ui_font(m_hWnd,std::wstring(AfxGetApp()->m_lpCmdLine).find(L"--font125")!=std::wstring::npos);
    tree_=::CreateWindowExW(WS_EX_CLIENTEDGE,WC_TREEVIEWW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_CHECKBOXES,0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    info_=ui::create_window(L"STATIC",L"Select numeric leaves only. Stage creates value1/value2 aliases; review names and Apply separately. No UART or file changes.",WS_CHILD|WS_VISIBLE|SS_NOPREFIX,0,0,1,1,m_hWnd,nullptr,nullptr,nullptr);
    apply_=ui::create_window(L"BUTTON",L"Stage paths",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(IDOK),nullptr,nullptr);
    cancel_=ui::create_window(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,m_hWnd,reinterpret_cast<HMENU>(IDCANCEL),nullptr,nullptr);
    set_child_fonts(m_hWnd,font_);
    for(const auto& node:nodes_){auto label=uart_wide(node.pointer.empty()?"(root)":node.pointer)+L" | "+ui::tr(uart_wide(node.type));if(node.numeric)label+=L" = "+std::to_wstring(node.value);
        TVINSERTSTRUCTW insert{};insert.hParent=node.parent==SIZE_MAX?TVI_ROOT:items_.at(node.parent);insert.hInsertAfter=TVI_LAST;insert.item.mask=TVIF_TEXT;insert.item.pszText=label.data();items_.push_back(TreeView_InsertItem(tree_,&insert));
        if(!node.numeric)TreeView_SetItemState(tree_,items_.back(),0,TVIS_STATEIMAGEMASK);
    }
    if(!items_.empty())TreeView_Expand(tree_,items_[0],TVE_EXPAND);
    const auto work=discovery_work_area(GetOwner()?GetOwner()->GetSafeHwnd():m_hWnd);const auto r=fit_discovery_bounds({work.left+50,work.top+50,work.left+950,work.top+700},work);SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER);layout();::SetFocus(tree_);return FALSE;
}
void JsonTreeDialog::OnOK(){
    try{std::vector<size_t> selected;for(size_t i=0;i<items_.size();++i)if(TreeView_GetCheckState(tree_,items_[i])&&nodes_[i].numeric)selected.push_back(i);selection_=json_tree_selection(nodes_,selected);CDialog::OnOK();}
    catch(const std::exception& e){::SetWindowTextW(info_,ui::caption(uart_wide(e.what()).c_str()));}
}
void JsonTreeDialog::layout(){if(!tree_)return;CRect r;GetClientRect(&r);const int row=std::max(32,static_cast<int>(GetDpiForWindow(m_hWnd))*32/96);const int bottom=r.bottom-row*3-20;
    ::MoveWindow(tree_,10,10,std::max(1,r.Width()-20),std::max(1,bottom-10),TRUE);::MoveWindow(info_,10,bottom+4,std::max(1,r.Width()-20),row*2,TRUE);
    const int a=control_text_width(apply_,32),b=control_text_width(cancel_,32);::MoveWindow(apply_,10,r.bottom-row-8,a,row,TRUE);::MoveWindow(cancel_,20+a,r.bottom-row-8,b,row,TRUE);
}
void JsonTreeDialog::OnSize(UINT t,int w,int h){CDialog::OnSize(t,w,h);if(t!=SIZE_MINIMIZED)layout();}
void JsonTreeDialog::OnGetMinMaxInfo(MINMAXINFO* info){CDialog::OnGetMinMaxInfo(info);const auto work=discovery_work_area(m_hWnd);info->ptMinTrackSize.x=std::min<LONG>(600,work.right-work.left);info->ptMinTrackSize.y=std::min<LONG>(420,work.bottom-work.top);}
LRESULT JsonTreeDialog::OnDpiChanged(WPARAM,LPARAM p){const auto next=make_ui_font(m_hWnd);if(next){const auto old=font_;font_=next;set_child_fonts(m_hWnd,font_);if(old)::DeleteObject(old);}if(p){const auto r=fit_discovery_bounds(*reinterpret_cast<RECT*>(p),discovery_work_area(m_hWnd));SetWindowPos(nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);}layout();return 0;}
void JsonTreeDialog::OnDestroy(){if(tree_){const auto images=TreeView_GetImageList(tree_,TVSIL_STATE);TreeView_SetImageList(tree_,nullptr,TVSIL_STATE);if(images)ImageList_Destroy(images);}for(auto child=::GetWindow(m_hWnd,GW_CHILD);child;){const auto next=::GetWindow(child,GW_HWNDNEXT);::DestroyWindow(child);child=next;}CDialog::OnDestroy();if(font_)::DeleteObject(font_);font_=nullptr;tree_=nullptr;}
