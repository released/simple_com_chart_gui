#include "channel_panel.h"

#include <commctrl.h>
#include <cstdio>
#include <algorithm>
#include "dpi_support.h"
#include "ui_metrics.h"

void ChannelPanel::set_ui_font(HFONT font) {
    set_child_fonts(hwnd_,font); RECT client{}; GetClientRect(hwnd_,&client); layout(client.right,client.bottom);
}

namespace {
constexpr int kValueWidth = 105;
constexpr int kPadding = 6;

std::wstring to_wstring(const std::string& s) {
    if (s.empty()) {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    out.resize(len - 1);
    return out;
}

std::wstring to_wstring(double value) {
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"%.10g", value);
    return buf;
}
} // namespace

bool ChannelPanel::create(HWND parent, int x, int y, int w, int h, int id) {
    parent_ = parent;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ChannelPanel::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ChannelPanelWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"", WS_CHILD | WS_VISIBLE,
                            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance, this);
    return hwnd_ != nullptr;
}

HWND ChannelPanel::hwnd() const {
    return hwnd_;
}

void ChannelPanel::reset() {
    keys_.clear();
    all_checks_.clear();
    wide_keys_.clear();
    sort_dirty_ = false;
    index_map_.clear();
    color_map_.clear();
    if (list_) {
        ListView_DeleteAllItems(list_);
    }
    update_count(0);
}

void ChannelPanel::update_count(int count) {
    if (!label_count_) {
        return;
    }
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"Detected: %d", count);
    SetWindowTextW(label_count_, buf);
    if (sort_dirty_) sort_channels();
    fit_columns();
}

void ChannelPanel::ensure_channel(const std::string& key, bool enabled, COLORREF color) {
    if (!list_) {
        return;
    }

    auto it = index_map_.find(key);
    if (it != index_map_.end()) {
        color_map_[key] = color;
        return;
    }

    int index = static_cast<int>(keys_.size());
    keys_.push_back(key);
    wide_keys_.push_back(to_wstring(key));
    index_map_[key] = index;
    color_map_[key] = color;
    all_checks_[key]=enabled;
    const auto title=display_?to_wstring(display_->name(key)):wide_keys_.back();
    if(!search_text_.empty()) {auto query=search_text_,name=wide_keys_.back()+L" "+title;std::transform(query.begin(),query.end(),query.begin(),towlower);std::transform(name.begin(),name.end(),name.begin(),towlower);if(name.find(query)==name.npos) return;}

    suppress_notify_ = true;
    LVITEMW item = {};
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = ListView_GetItemCount(list_);
    item.lParam = index;
    item.pszText = const_cast<wchar_t*>(title.c_str());
    const int row = ListView_InsertItem(list_, &item);
    ListView_SetCheckState(list_, row, enabled ? TRUE : FALSE);
    ListView_SetItemText(list_, row, 1, const_cast<wchar_t*>(L"--"));
    suppress_notify_ = false;
    sort_dirty_ = sort_active_;
}

void ChannelPanel::update_values(const std::unordered_map<std::string, double>& latest) {
    if (!list_) {
        return;
    }

    for (int row = 0; row < ListView_GetItemCount(list_); ++row) {
        const auto key = key_at_row(row);
        if (!key) continue;
        auto it = latest.find(*key);
        if (it == latest.end()) {
            ListView_SetItemText(list_, row, 1, const_cast<wchar_t*>(L"--"));
        } else {
            std::wstring val = display_?to_wstring(display_->format(*key,values_projected_?it->second:display_->project(*key,it->second))):to_wstring(it->second);
            ListView_SetItemText(list_, row, 1, const_cast<wchar_t*>(val.c_str()));
        }
    }
}

std::unordered_map<std::string, bool> ChannelPanel::get_checkbox_state_map() const {
    return all_checks_;
}

const std::string* ChannelPanel::key_at_row(int row) const {
    LVITEMW item{}; item.mask = LVIF_PARAM; item.iItem = row;
    if (!ListView_GetItem(list_, &item) || item.lParam < 0 || static_cast<size_t>(item.lParam) >= keys_.size()) return nullptr;
    return &keys_[static_cast<size_t>(item.lParam)];
}

int CALLBACK ChannelPanel::compare_items(LPARAM left, LPARAM right, LPARAM context) {
    const auto self = reinterpret_cast<const ChannelPanel*>(context);
    const auto& a = self->wide_keys_[static_cast<size_t>(left)];
    const auto& b = self->wide_keys_[static_cast<size_t>(right)];
    int order = CompareStringEx(LOCALE_NAME_INVARIANT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS,
        a.c_str(), -1, b.c_str(), -1, nullptr, nullptr, 0);
    // Deterministic tie-break for case or numeric spellings such as P2/P02.
    if (!order || order == CSTR_EQUAL) order = CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, FALSE);
    const int result = order == CSTR_LESS_THAN ? -1 : order == CSTR_GREATER_THAN ? 1 : 0;
    return self->sort_ascending_ ? result : -result;
}

void ChannelPanel::sort_channels() {
    if (!sort_active_ || !IsWindow(list_)) return;
    const auto checked = get_checkbox_state_map();
    const bool previous = suppress_notify_;
    suppress_notify_ = true;
    SendMessageW(list_, WM_SETREDRAW, FALSE, 0);
    ListView_SortItems(list_, compare_items, reinterpret_cast<LPARAM>(this));
    for (int row = 0; row < ListView_GetItemCount(list_); ++row) {
        const auto key = key_at_row(row);
        if (key) ListView_SetCheckState(list_, row, checked.at(*key) ? TRUE : FALSE);
    }
    suppress_notify_ = previous;
    sort_dirty_ = false;
    HDITEMW header{}; header.mask = HDI_FORMAT;
    const auto control = ListView_GetHeader(list_);
    if (Header_GetItem(control, 0, &header)) {
        header.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        header.fmt |= sort_ascending_ ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(control, 0, &header);
    }
    LVCOLUMNW column{}; column.mask = LVCF_TEXT;
    column.pszText = const_cast<wchar_t*>(sort_ascending_ ? L"Channel A-Z" : L"Channel Z-A");
    ListView_SetColumn(list_, 0, &column);
    SendMessageW(list_, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list_, nullptr, FALSE);
}

LRESULT CALLBACK ChannelPanel::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ChannelPanel* self = reinterpret_cast<ChannelPanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<ChannelPanel*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        return self->handle_message(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT ChannelPanel::handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        label_count_ = CreateWindowW(L"STATIC", L"Detected: 0", WS_CHILD | WS_VISIBLE,
                                     kPadding, kPadding, 120, 20, hwnd, nullptr, nullptr, nullptr);
        btn_all_ = CreateWindowW(L"BUTTON", L"All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 kPadding, kPadding + 24, 60, 22, hwnd, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        btn_none_ = CreateWindowW(L"BUTTON", L"None", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  kPadding + 70, kPadding + 24, 60, 22, hwnd, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        search_=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,hwnd,reinterpret_cast<HMENU>(4),nullptr,nullptr);
        SendMessageW(search_,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(L"Search channels..."));SendMessageW(search_,EM_SETLIMITTEXT,128,0);

        list_ = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                              kPadding, kPadding + 52, 200, 200, hwnd, reinterpret_cast<HMENU>(3), nullptr, nullptr);
        ListView_SetExtendedListViewStyle(list_, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = const_cast<wchar_t*>(L"Channel");
        col.cx = 160;
        ListView_InsertColumn(list_, 0, &col);

        col.pszText = const_cast<wchar_t*>(L"Value");
        col.cx = kValueWidth;
        ListView_InsertColumn(list_, 1, &col);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        layout(w, h);
        return 0;
    }
    case WM_COMMAND: {
        if(reinterpret_cast<HWND>(lParam)==search_ && HIWORD(wParam)==EN_CHANGE) {all_checks_=get_checkbox_state_map();wchar_t value[129]{};GetWindowTextW(search_,value,129);search_text_=value;rebuild_visible();return 0;}
        if (reinterpret_cast<HWND>(lParam) == btn_all_) {
            on_all_none(true);
            return 0;
        }
        if (reinterpret_cast<HWND>(lParam) == btn_none_) {
            on_all_none(false);
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr->hwndFrom == list_ && hdr->code == NM_CUSTOMDRAW) {
            auto* cd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) {
                return CDRF_NOTIFYITEMDRAW;
            }
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                int index = static_cast<int>(cd->nmcd.dwItemSpec);
                if (index >= 0 && index < static_cast<int>(keys_.size())) {
                    const auto key = key_at_row(index);
                    auto it = key ? color_map_.find(*key) : color_map_.end();
                    if (it != color_map_.end()) {
                        cd->clrText = it->second;
                    }
                }
                return CDRF_DODEFAULT;
            }
        } else if (hdr->hwndFrom == list_ && hdr->code == LVN_COLUMNCLICK) {
            const auto click = reinterpret_cast<NMLISTVIEW*>(lParam);
            if (click->iSubItem == 0) {
                sort_ascending_ = !sort_active_ || !sort_ascending_;
                sort_active_ = true;
                sort_channels();
            }
            return 0;
        } else if (hdr->hwndFrom == list_ && hdr->code == LVN_ITEMCHANGED) {
            auto* nmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
            if ((nmlv->uChanged & LVIF_STATE) != 0) {
                UINT state_change = (nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK;
                if (state_change != 0 && !suppress_notify_) {
                    if(const auto key=key_at_row(nmlv->iItem)) all_checks_[*key]=ListView_GetCheckState(list_,nmlv->iItem)!=FALSE;
                    send_channel_changed();
                }
            }
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ChannelPanel::layout(int w, int h) {
    if (!label_count_) {
        return;
    }
    const int row=std::max(24,control_font_height(label_count_)+8);
    MoveWindow(label_count_, kPadding, kPadding, w - 2 * kPadding, row, TRUE);
    const int all_width=std::max(60,control_text_width(btn_all_,24)),none_width=std::max(60,control_text_width(btn_none_,24));
    MoveWindow(btn_all_,kPadding,kPadding+row,all_width,row,TRUE);
    MoveWindow(btn_none_,kPadding+all_width+8,kPadding+row,none_width,row,TRUE);
    MoveWindow(search_,kPadding,kPadding+row*2+4,w-2*kPadding,row,TRUE);

    int list_y = kPadding + row*3+8;
    int list_h = h - list_y - kPadding;
    int list_w = w - 2 * kPadding;
    MoveWindow(list_, kPadding, list_y, list_w, list_h, TRUE);

    fit_columns();
}

void ChannelPanel::fit_columns() {
    if (!IsWindow(list_)) return;
    RECT client{}; GetClientRect(list_, &client);
    const int channel_w = std::max(80, static_cast<int>(client.right) - kValueWidth - 2);
    if (ListView_GetColumnWidth(list_, 0) != channel_w) ListView_SetColumnWidth(list_, 0, channel_w);
    if (ListView_GetColumnWidth(list_, 1) != kValueWidth) ListView_SetColumnWidth(list_, 1, kValueWidth);
}

void ChannelPanel::on_all_none(bool all_checked) {
    if (!list_) {
        return;
    }
    suppress_notify_ = true;
    for (int row = 0; row < ListView_GetItemCount(list_); ++row) {
        ListView_SetCheckState(list_, row, all_checked ? TRUE : FALSE);
        if(const auto key=key_at_row(row)) all_checks_[*key]=all_checked;
    }
    suppress_notify_ = false;
    send_channel_changed();
}

void ChannelPanel::set_display_preferences(const DisplayPreferences* value,bool projected) {all_checks_=get_checkbox_state_map();display_=value;values_projected_=projected;rebuild_visible();}
void ChannelPanel::rebuild_visible() {
    if(!IsWindow(list_)) return;suppress_notify_=true;SendMessageW(list_,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(list_);
    auto query=search_text_;std::transform(query.begin(),query.end(),query.begin(),towlower);
    for(size_t i=0;i<keys_.size();++i) {
        const auto title=display_?to_wstring(display_->name(keys_[i])):wide_keys_[i];
        auto searchable=wide_keys_[i]+L" "+title;std::transform(searchable.begin(),searchable.end(),searchable.begin(),towlower);
        if(!query.empty() && searchable.find(query)==searchable.npos) continue;
        LVITEMW item{};item.mask=LVIF_TEXT|LVIF_PARAM;item.iItem=ListView_GetItemCount(list_);item.lParam=static_cast<LPARAM>(i);item.pszText=const_cast<wchar_t*>(title.c_str());
        const int row=ListView_InsertItem(list_,&item);ListView_SetCheckState(list_,row,all_checks_[keys_[i]]?TRUE:FALSE);
    }
    suppress_notify_=false;if(sort_active_) sort_channels();SendMessageW(list_,WM_SETREDRAW,TRUE,0);InvalidateRect(list_,nullptr,TRUE);
}

void ChannelPanel::send_channel_changed() {
    if (parent_) {
        SendMessageW(parent_, WM_APP + 1, 0, 0);
    }
}
