#include "data_page.h"
#include "ui_localized_controls.h"
#include "ui_metrics.h"
#include <commctrl.h>
#include <cstdio>
#include <cmath>
#include "dpi_support.h"
#include "chart_time_controls.h"

void DataPage::set_ui_font(HFONT font) {
    font_=font; set_child_fonts(hwnd_,font); field_panel_.set_ui_font(font);
    RECT client{}; GetClientRect(hwnd_,&client); layout(client.right,client.bottom);
}
void DataPage::set_display_preferences(const DisplayPreferences* value) {
    display_=value;field_panel_.set_display_preferences(value);for(auto& model:models_) model.set_display_preferences(value);
    if(IsWindow(table_)) {rebuild_columns();InvalidateRect(table_,nullptr,TRUE);for(auto& plot:plots_) plot.update_from_model(now_);}
}

namespace {
std::wstring wide(const std::string& value) {
    const int n = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), n); return result;
}
}

bool DataPage::create(HWND parent, CaptureStore* capture, HFONT font) {
    capture_ = capture; font_ = font;
    WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"TelemetryDataPage"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1); RegisterClassW(&wc);
    hwnd_ = ui::create_window_ex(WS_EX_CONTROLPARENT, wc.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 500, 500, parent, reinterpret_cast<HMENU>(602), wc.hInstance, this);
    if (!hwnd_) return false;
    follow_ = ui::create_window(L"BUTTON", L"Auto-scroll to newest", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(1), nullptr, nullptr);
    reset_ = ui::create_window(L"BUTTON", L"Reset to none", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(4), nullptr, nullptr);
    info_ = ui::create_window(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, nullptr, nullptr);
    table_ = ui::create_window_ex(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(2), nullptr, nullptr);
    ListView_SetExtendedListViewStyle(table_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    ListView_SetUnicodeFormat(table_, TRUE);
    field_panel_.create(hwnd_, 0, 0, 260, 200, 3);
    for (int i = 0; i < 4; ++i) {
        selectors_[i] = ui::create_window(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            0, 0, 1, 200, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(10 + i)), nullptr, nullptr);
        time_selectors_[i] = ui::create_window(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | WS_VSCROLL,
            0, 0, 1, 200, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(20 + i)), nullptr, nullptr);
        ::chart_time::populate(time_selectors_[i]);
        plots_[i].create(hwnd_, 0, 0, 1, 1, 30 + i);
        plots_[i].set_model(&models_[i]); plots_[i].set_compact(true);
        plots_[i].set_overlay_enabled(true); plots_[i].set_time_window(seconds_[i]);
        models_[i].set_max_channels(1); models_[i].set_time_window(seconds_[i]);
    }
    for (HWND child = GetWindow(hwnd_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    EnumChildWindows(field_panel_.hwnd(), [](HWND child, LPARAM font) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(font), TRUE); return TRUE;
    }, reinterpret_cast<LPARAM>(font_));
    set_follow(true); reset();
    RECT client{};GetClientRect(hwnd_,&client);layout(client.right,client.bottom);
    return true;
}

void DataPage::reset() {
    if (!IsWindow(table_)) return;
    ListView_SetItemCountEx(table_, 0, LVSICF_NOSCROLL);
    while (ListView_DeleteColumn(table_, 0)) {}
    known_fields_ = 0; first_sequence_ = 0; now_ = 0;
    visible_columns_.clear(); latest_values_.clear(); field_panel_.reset();
    accepted_fields_.clear();
    selector_keys_.clear();
    for (size_t i = 0; i < 4; ++i) {
        models_[i].reset(); plots_[i].reset_visual();
        SendMessageW(selectors_[i], CB_RESETCONTENT, 0, 0);
        SendMessageW(selectors_[i], CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(none)"));
        SendMessageW(selectors_[i], CB_SETCURSEL, 0, 0);
    }
    for (const auto& key : selected_) if (!key.empty()) ensure_selector_key(key);
    for (size_t i = 0; i < 4; ++i) select_field(i, selected_[i]);
    rebuild_columns(); sync_columns(); refresh(0);
}

void DataPage::sync_columns() {
    const auto& keys = capture_->keys();
    const bool changed = known_fields_ < keys.size();
    while (known_fields_ < keys.size()) {
        const auto& key = keys[known_fields_];
        field_panel_.ensure_channel(key, false, GetSysColor(COLOR_WINDOWTEXT));
        ensure_selector_key(key);
        ++known_fields_;
    }
    field_panel_.update_count(static_cast<int>(keys.size()));
    if (changed) rebuild_columns();
}

void DataPage::ensure_selector_key(const std::string& key) {
    if (std::find(selector_keys_.begin(), selector_keys_.end(), key) != selector_keys_.end()) return;
    selector_keys_.push_back(key);
    const auto name = wide(key);
    for (auto selector : selectors_) SendMessageW(selector, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
}

void DataPage::set_show_names(bool enabled) {
    for (auto& plot : plots_) plot.set_show_names(enabled);
}

void DataPage::append(const std::unordered_map<std::string, double>& values, double timestamp) {
    now_ = timestamp;
    const auto& keys = capture_->keys();
    while (accepted_fields_.size() < keys.size()) accepted_fields_.insert(keys[accepted_fields_.size()]);
    // Do not retain arbitrary rejected keys beyond the applied channel limit.
    for (const auto& item : values)
        if (accepted_fields_.count(item.first)) latest_values_[item.first] = item.second;
    for (size_t i = 0; i < 4; ++i) {
        const auto it = values.find(selected_[i]);
        if (it != values.end() && accepted_fields_.count(selected_[i])) models_[i].update_from_kv({*it}, timestamp);
        models_[i].prune(timestamp);
    }
}

void DataPage::refresh(double timestamp) {
    if (!IsWindow(table_)) return;
    if (timestamp < 0) timestamp = now_;
    const auto& rows = capture_->rows();
    const int top = ListView_GetTopIndex(table_);
    const uint64_t anchor = first_sequence_ ? first_sequence_ + top : 0;
    sync_columns();
    field_panel_.update_values(latest_values_);
    ListView_SetItemCountEx(table_, static_cast<int>(rows.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    first_sequence_ = rows.empty() ? 0 : rows.front()->sequence;
    fit_columns();
    if (!rows.empty()) {
        if (follows()) ListView_EnsureVisible(table_, static_cast<int>(rows.size() - 1), FALSE);
        else if (anchor && first_sequence_) {
            const int target = static_cast<int>(anchor > first_sequence_ ? std::min<uint64_t>(anchor - first_sequence_, rows.size() - 1) : 0);
            const int current = ListView_GetTopIndex(table_);
            RECT item{};
            if (ListView_GetItemRect(table_, current, &item, LVIR_BOUNDS))
                ListView_Scroll(table_, 0, (target - current) * (item.bottom - item.top));
        }
    }
    InvalidateRect(table_, nullptr, FALSE);
    const auto info = L"Rows: " + std::to_wstring(rows.size()) + L" / 10000 | Fields: " +
        std::to_wstring(visible_columns_.size()) + L" / " + std::to_wstring(capture_->keys().size()) +
        L" shown | Discarded: " + std::to_wstring(capture_->dropped()) + L" | Save As = all retained fields";
    SetWindowTextW(info_, info.c_str());
    RECT bounds{}; GetClientRect(hwnd_, &bounds);
    const int row = std::max(26, control_font_height(follow_) + 10);
    const int info_x = control_text_width(follow_, GetSystemMetrics(SM_CXMENUCHECK) + 20) + control_text_width(reset_, 24) + 16;
    if (header_height_ != std::max(row, control_wrapped_text_height(info_, bounds.right - info_x) + 4))
        layout(bounds.right, bounds.bottom);
    for (size_t i = 0; i < 4; ++i) plots_[i].update_from_model(timestamp);
}

void DataPage::set_follow(bool enabled) {
    SendMessageW(follow_, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    if (enabled && ListView_GetItemCount(table_) > 0) ListView_EnsureVisible(table_, ListView_GetItemCount(table_) - 1, FALSE);
}
bool DataPage::follows() const { return SendMessageW(follow_, BM_GETCHECK, 0, 0) == BST_CHECKED; }

void DataPage::select_field(size_t chart, const std::string& key) {
    if (chart >= 4) return;
    selected_[chart] = key;
    if (!key.empty()) ensure_selector_key(key);
    const auto found = std::find(selector_keys_.begin(), selector_keys_.end(), key);
    SendMessageW(selectors_[chart], CB_SETCURSEL, key.empty() ? 0 : 1 + (found - selector_keys_.begin()), 0);
    rebuild_chart(chart);
}

void DataPage::set_selected_fields(const std::array<std::string, 4>& fields) {
    for (size_t i = 0; i < 4; ++i) select_field(i, fields[i]);
}

std::shared_ptr<const CaptureRow> DataPage::selected_row()const {
    const auto index=ListView_GetNextItem(table_,-1,LVNI_SELECTED);
    if(index<0||static_cast<size_t>(index)>=capture_->rows().size())return {};
    return capture_->rows()[index];
}
void DataPage::set_chart_time(size_t chart,double seconds) {
    if(chart>=4||!std::isfinite(seconds)||seconds<::chart_time::minimum||seconds>86400)return;
    seconds_[chart]=seconds;models_[chart].set_time_window(seconds);plots_[chart].set_time_window(seconds);
    ::chart_time::show(time_selectors_[chart],seconds);rebuild_chart(chart);
}

void DataPage::apply_time_input(size_t chart, bool selection) {
    if (!IsWindow(plots_[3].hwnd())) return;
    double seconds=0;
    if (::chart_time::read(time_selectors_[chart],selection,seconds)) set_chart_time(chart,seconds);
    else {
        ::chart_time::show(time_selectors_[chart],seconds_[chart]);
        SendMessageW(GetParent(hwnd_),kTimeInputInvalid,0,0);
    }
}

bool DataPage::commit_time_input(HWND target) {
    for (size_t chart=0;chart<4;++chart) if (::chart_time::owns(time_selectors_[chart],target)) {
        apply_time_input(chart,false);return true;
    }
    return false;
}

void DataPage::rebuild_chart(size_t chart) {
    models_[chart].reset(); plots_[chart].reset_visual();
    const auto& keys = capture_->keys();
    const auto found = std::find(keys.begin(), keys.end(), selected_[chart]);
    if (found != keys.end()) {
        const auto column = static_cast<size_t>(found - keys.begin());
        const auto& rows = capture_->rows();
        for (const auto& row : rows) {
            double value;
            if (CaptureStore::value(*row, column, value)) models_[chart].update_from_kv({{*found, value}},row->source_seconds);
        }
        models_[chart].prune(now_);
    }
    plots_[chart].update_from_model(now_);
}

void DataPage::layout(int w, int h) {
    if (!IsWindow(table_)) return;
    const int row = std::max(26, control_font_height(follow_) + 10), gap = 8;
    const int check_w = control_text_width(follow_, GetSystemMetrics(SM_CXMENUCHECK) + 20);
    const int reset_w = control_text_width(reset_, 24);
    const int info_x = check_w + reset_w + 2 * gap;
    header_height_ = std::max(row, control_wrapped_text_height(info_, w - info_x) + 4);
    MoveWindow(follow_, 0, 0, check_w, row, TRUE);
    MoveWindow(reset_, check_w + gap, 0, reset_w, row, TRUE);
    MoveWindow(info_, info_x, 4, std::max(1, w - info_x), header_height_ - 4, TRUE);
    const int columns=w<1200?2:4,rows=4/columns;
    const int table_h = std::max(60, (h - header_height_ - 2 * gap) * (rows==2?2:3) / 6);
    const int panel_w = std::min(290, std::max(240, w / 4));
    MoveWindow(field_panel_.hwnd(), 0, header_height_ + gap, panel_w, table_h, TRUE);
    MoveWindow(table_, panel_w + gap, header_height_ + gap, std::max(1, w - panel_w - gap), table_h, TRUE);
    fit_columns();
    const int top = header_height_ + gap + table_h + gap;
    const int tile_w = (w - (columns-1)*gap) / columns;
    const int tile_h=std::max(20,(h-top-(rows-1)*gap)/rows);
    for (int i = 0; i < 4; ++i) {
        const int x = (i%columns)*(tile_w+gap), y=top+(i/columns)*(tile_h+gap);
        const int time_w=::chart_time::control_width(time_selectors_[i]);
        MoveWindow(selectors_[i],x,y,std::max(50,tile_w-time_w-gap),300,TRUE);
        MoveWindow(time_selectors_[i],x+tile_w-time_w,y,time_w,200,TRUE);
        MoveWindow(plots_[i].hwnd(),x,y+row+4,tile_w,std::max(1,tile_h-row-4),TRUE);
    }
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK DataPage::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto self = reinterpret_cast<DataPage*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        self = static_cast<DataPage*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        self->hwnd_ = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}
LRESULT DataPage::handle(UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_SIZE) { layout(LOWORD(lp), HIWORD(lp)); return 0; }
    if (msg == WM_APP + 1) { rebuild_columns(); refresh(-1); return 0; }
    if(msg==PlotView::kCursorMoved) {return SendMessageW(GetParent(hwnd_),msg,wp,lp);}
    if (msg == WM_COMMAND) {
        const auto id = LOWORD(wp);
        if (id == 1) { set_follow(follows()); return 0; }
        if (id == 4 && HIWORD(wp) == BN_CLICKED) {
            set_selected_fields({});
            SendMessageW(GetParent(hwnd_), kSelectionChanged, 0, 0); return 0;
        }
        if (id >= 10 && id <= 13 && HIWORD(wp) == CBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageW(selectors_[id - 10], CB_GETCURSEL, 0, 0));
            if (index < 0 || static_cast<size_t>(index) > selector_keys_.size()) return 0;
            select_field(id - 10, index > 0 ? selector_keys_[index - 1] : "");
            SendMessageW(GetParent(hwnd_), kSelectionChanged, 0, 0); return 0;
        }
        if (id >= 20 && id <= 23 && (HIWORD(wp) == CBN_SELCHANGE || HIWORD(wp) == CBN_KILLFOCUS)) {
            apply_time_input(id-20,HIWORD(wp)==CBN_SELCHANGE);
            return 0;
        }
    }
    if (msg == WM_NOTIFY && reinterpret_cast<NMHDR*>(lp)->hwndFrom == table_) {
        auto header = reinterpret_cast<NMHDR*>(lp);
        if(header->code==NM_DBLCLK) {const auto click=reinterpret_cast<NMITEMACTIVATE*>(lp);if(click->iItem>=0 && static_cast<size_t>(click->iItem)<capture_->rows().size()) {const auto row=capture_->rows()[click->iItem];SendMessageW(GetParent(hwnd_),kNavigate,0,reinterpret_cast<LPARAM>(row.get()));}return 0;}
        if (header->code == LVN_GETDISPINFOW) {
            auto info = reinterpret_cast<NMLVDISPINFOW*>(lp);
            if ((info->item.mask & LVIF_TEXT) && info->item.iItem >= 0 &&
                static_cast<size_t>(info->item.iItem) < capture_->rows().size()) {
                const auto& row = *capture_->rows()[info->item.iItem];
                wchar_t buffer[80] = {};
                if (info->item.iSubItem == 0) swprintf_s(buffer, L"%llu", static_cast<unsigned long long>(row.sequence));
                else if (info->item.iSubItem == 1) swprintf_s(buffer, L"%.3f", row.seconds);
                else if (info->item.iSubItem >= 2 && static_cast<size_t>(info->item.iSubItem - 2) < visible_columns_.size()) {
                    double value;
                    const auto column=visible_columns_[info->item.iSubItem-2];
                    if (CaptureStore::value(row,column,value)) {
                        if(display_) {const auto& key=capture_->keys()[column];const auto formatted=wide(display_->format(key,display_->project(key,value)));wcsncpy_s(buffer,formatted.c_str(),_TRUNCATE);}
                        else swprintf_s(buffer,L"%.10g",value);
                    }
                }
                if (info->item.pszText && info->item.cchTextMax > 0)
                    wcsncpy_s(info->item.pszText, info->item.cchTextMax, buffer, _TRUNCATE);
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}
bool DataPage::select_source_time(double seconds) {
    const auto& rows=capture_->rows();if(rows.empty() || seconds<rows.front()->source_seconds || seconds>rows.back()->source_seconds) return false;
    auto row=std::lower_bound(rows.begin(),rows.end(),seconds,[](const auto& item,double t){return item->source_seconds<t;});
    if(row!=rows.begin() && (row==rows.end() || seconds-(*(row-1))->source_seconds<=(*row)->source_seconds-seconds)) --row;
    return select_sequence((*row)->sequence);
}
bool DataPage::select_sequence(uint64_t sequence) {
    const auto& rows=capture_->rows();if(rows.empty()||sequence<rows.front()->sequence||sequence>rows.back()->sequence) return false;
    const auto found=std::lower_bound(rows.begin(),rows.end(),sequence,[](const auto& row,uint64_t n){return row->sequence<n;});if(found==rows.end()||(*found)->sequence!=sequence) return false;
    const int index=static_cast<int>(found-rows.begin());set_follow(false);ListView_SetItemState(table_,-1,0,LVIS_SELECTED|LVIS_FOCUSED);ListView_SetItemState(table_,index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);ListView_EnsureVisible(table_,index,FALSE);return true;
}
