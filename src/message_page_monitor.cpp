#include "message_page.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include <richedit.h>
#include <algorithm>
#include <cstring>

namespace {
struct ColorStream {std::string_view bytes;size_t position=0;};
DWORD CALLBACK read_color_stream(DWORD_PTR cookie,LPBYTE output,LONG count,LONG* written){
    auto& stream=*reinterpret_cast<ColorStream*>(cookie);const auto size=std::min(static_cast<size_t>(std::max<LONG>(0,count)),stream.bytes.size()-stream.position);
    if(size)std::memcpy(output,stream.bytes.data()+stream.position,size);stream.position+=size;*written=static_cast<LONG>(size);return 0;
}
}

void MessagePage::set_keywords(std::wstring definition,const std::vector<KeywordRule>& rules){keyword_definition_=std::move(definition);keyword_rules_=rules;keyword_error_.clear();keyword_limited_=false;rebuild_=dirty_=true;refresh();}
void MessagePage::update_inspection_status(){
    if(!IsWindow(inspection_info_))return;
    const bool visible=IsWindowVisible(hwnd_)&&!paused_&&follows();
    if(visible&&!dirty_)seen_total_=received_total_;
    const bool inspecting=paused_||!follows();
    auto text=ui::tr(paused_?L"Paused display; receiving continues":follows()?L"Following":L"Browsing history");
    // Pending paint while following is not an inspection backlog.
    text+=L" | "+ui::tr(L"New bytes:")+L" "+std::to_wstring(inspecting?received_total_-seen_total_:0);
    if(text!=inspection_text_){SetWindowTextW(inspection_info_,text.c_str());inspection_text_=std::move(text);}
    if((IsWindowEnabled(latest_)!=FALSE)!=inspecting)EnableWindow(latest_,inspecting);
}
void MessagePage::user_scroll_begin(UINT message,WPARAM w){
    if(!smart_scroll_||paused_)return;
    const bool up=message==WM_MOUSEWHEEL?GET_WHEEL_DELTA_WPARAM(w)>0:message==WM_KEYDOWN?w==VK_PRIOR||w==VK_HOME||w==VK_UP:LOWORD(w)==SB_LINEUP||LOWORD(w)==SB_PAGEUP||LOWORD(w)==SB_TOP||LOWORD(w)==SB_THUMBTRACK||LOWORD(w)==SB_THUMBPOSITION;
    SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};GetScrollInfo(view_,SB_VERT,&info);scroll_position_before_=info.nPos;
    if(up&&follows()&&info.nMax>=static_cast<int>(info.nPage)){SendMessageW(follow_,BM_SETCHECK,BST_UNCHECKED,0);smart_suspended_=true;}
}
void MessagePage::user_scroll_end(UINT message,WPARAM input){
    if(smart_suspended_&&!paused_){
        SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS};GetScrollInfo(view_,SB_VERT,&info);
        const int code=LOWORD(input);const bool explicit_bottom=message==WM_VSCROLL&&code==SB_BOTTOM;
        const bool descending=message==WM_MOUSEWHEEL?GET_WHEEL_DELTA_WPARAM(input)<0:message==WM_KEYDOWN?input==VK_NEXT||input==VK_END||input==VK_DOWN:explicit_bottom||code==SB_LINEDOWN||code==SB_PAGEDOWN||((code==SB_THUMBTRACK||code==SB_THUMBPOSITION)&&info.nPos>=scroll_position_before_);
        const int bottom=std::max(info.nMin,info.nMax-static_cast<int>(info.nPage)+1);bool at_bottom=explicit_bottom||info.nPos>=bottom;
        if(descending&&!at_bottom&&view_==rich_view_){GETTEXTLENGTHEX query{GTL_NUMCHARS|GTL_PRECISE,1200};const auto length=SendMessageW(view_,EM_GETTEXTLENGTHEX,reinterpret_cast<WPARAM>(&query),0);POINTL point{-1,-1};SendMessageW(view_,EM_POSFROMCHAR,reinterpret_cast<WPARAM>(&point),std::max<LRESULT>(0,length-1));RECT client{};GetClientRect(view_,&client);int body_height=1;const auto dc=GetDC(view_);if(dc){const auto previous=SelectObject(dc,mono_font_?mono_font_:GetStockObject(DEFAULT_GUI_FONT));TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);body_height=metrics.tmHeight;if(previous&&previous!=HGDI_ERROR)SelectObject(dc,previous);ReleaseDC(view_,dc);}at_bottom=point.y>=0&&point.y+body_height<=client.bottom;}
        if(descending&&at_bottom){SendMessageW(follow_,BM_SETCHECK,BST_CHECKED,0);smart_suspended_=false;seen_total_=received_total_;follow_tail();}
    }
    update_inspection_status();
}
void MessagePage::stream_keyword_colors(const FormattedMessage& formatted,size_t begin){
    if(view_!=rich_view_)return;
    if(rendered_text_.size()>2*1024*1024){SetWindowTextW(view_,rendered_text_.c_str());CHARFORMAT2W plain{};plain.cbSize=sizeof(plain);plain.dwMask=CFM_COLOR|CFM_BACKCOLOR;plain.crTextColor=RGB(0,255,0);plain.crBackColor=RGB(0,0,0);SendMessageW(view_,EM_SETCHARFORMAT,SCF_ALL,reinterpret_cast<LPARAM>(&plain));keyword_limited_=true;return;}
    begin=std::min(begin,rendered_text_.size());if(begin){const auto line=rendered_text_.find_last_of(L"\r\n",begin-1);begin=line==std::wstring::npos?0:line+1;}
    std::vector<RichColorSpan> colors;
    const auto add=[&](size_t first,size_t count,COLORREF fg,COLORREF bg){if(first>=rendered_text_.size())return;const auto end=first+std::min(count,rendered_text_.size()-first);if(end>begin)colors.push_back({std::max(first,begin)-begin,end-std::max(first,begin),fg,bg});};
    if(!formatted.colors.empty()){
        std::vector<size_t> positions(formatted.text.size()+1);size_t at=0;
        for(size_t i=0;i<formatted.text.size();++i){positions[i]=at;if(!(formatted.text[i]==L'\n'&&i&&formatted.text[i-1]==L'\r'))++at;}positions.back()=at;
        for(const auto& span:formatted.colors)if(span.start<positions.size()&&span.count<positions.size()-span.start)add(positions[span.start],positions[span.start+span.count]-positions[span.start],span.color,RGB(0,0,0));
    }
    bool limited=false;const auto spans=keyword_spans(rendered_text_,keyword_rules_,begin,limited);keyword_limited_=limited||(begin&&keyword_limited_);
    for(const auto& span:spans)add(span.start,span.count,span.color,RGB(0,0,0));
    if(highlight_matches_)for(const auto& match:matches_)add(match.start,match.length,RGB(0,0,0),RGB(230,210,70));
    auto rtf=monitor_colored_rtf(std::wstring_view(rendered_text_).substr(begin),colors,font_points_);
    // A full RTF document consumes its final paragraph marker; selection imports do not.
    if(!begin)rtf.insert(rtf.size()-1,"\\par ");
    ColorStream source{rtf};EDITSTREAM stream{};stream.dwCookie=reinterpret_cast<DWORD_PTR>(&source);stream.pfnCallback=read_color_stream;
    SendMessageW(view_,EM_SETSEL,begin,-1);SendMessageW(view_,EM_STREAMIN,SF_RTF|(begin?SFF_SELECTION:0),reinterpret_cast<LPARAM>(&stream));
    if(stream.dwError){SetWindowTextW(view_,rendered_text_.c_str());keyword_limited_=true;return;}
}
int MessagePage::minimum_height(int width)const{
    if(!IsWindow(mode_))return 200;RECT r{};GetWindowRect(mode_,&r);const int row=std::max({28,static_cast<int>(r.bottom-r.top),control_font_height(label_)+12});int x=0,lines=1;
    for(const auto h:{label_,mode_,follow_,latest_,clear_,font_label_,font_down_,font_size_,font_up_,options_button_,search_toggle_,save_log_,start_log_,stop_log_}){const int w=h==mode_?100:control_text_width(h,(h==follow_?GetSystemMetrics(SM_CXMENUCHECK)+24:24));if(x&&x+w>width){++lines;x=0;}x+=w+8;}
    return (lines+3+(search_visible_?3:0))*(row+8)+control_wrapped_text_height(info_,width)+control_wrapped_text_height(log_info_,width)+60;
}
