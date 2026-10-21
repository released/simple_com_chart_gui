#include "data_page.h"
#include "ui_metrics.h"
#include <commctrl.h>
#include <cstdio>

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
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 500, 500, parent, reinterpret_cast<HMENU>(602), wc.hInstance, this);
    if (!hwnd_) return false;
    follow_ = CreateWindowW(L"BUTTON", L"Auto-scroll to newest", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0, 0, 0, 0, hwnd_, reinterpret_cast<HMENU>(1), nullptr, nullptr);
    info_ = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, nullptr, nullptr);
    table_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
        0, 0, 1, 1, hwnd_, reinterpret_cast<HMENU>(2), nullptr, nullptr);
    ListView_SetExtendedListViewStyle(table_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    ListView_SetUnicodeFormat(table_, TRUE);
    for (int i = 0; i < 4; ++i) {
        selectors_[i] = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            0, 0, 1, 200, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(10 + i)), nullptr, nullptr);
        time_selectors_[i] = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            0, 0, 1, 200, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(20 + i)), nullptr, nullptr);
        for (const auto* value : {L"5 s", L"10 s", L"30 s", L"60 s", L"120 s"})
            SendMessageW(time_selectors_[i], CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value));
        SendMessageW(time_selectors_[i], CB_SETCURSEL, 2, 0);
        plots_[i].create(hwnd_, 0, 0, 1, 1, 30 + i);
        plots_[i].set_model(&models_[i]); plots_[i].set_compact(true);
        plots_[i].set_overlay_enabled(false); plots_[i].set_time_window(seconds_[i]);
        models_[i].set_max_channels(1); models_[i].set_time_window(seconds_[i]);
    }
    for (HWND child = GetWindow(hwnd_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    set_follow(true); reset(); return true;
}

void DataPage::reset() {
    if (!IsWindow(table_)) return;
    ListView_SetItemCountEx(table_, 0, LVSICF_NOSCROLL);
    while (ListView_DeleteColumn(table_, 0)) {}
    columns_ = 0; first_sequence_ = 0; now_ = 0;
    for (size_t i = 0; i < 4; ++i) {
        selected_[i].clear(); assigned_[i] = false; models_[i].reset(); plots_[i].reset_visual();
        SendMessageW(selectors_[i], CB_RESETCONTENT, 0, 0);
        SendMessageW(selectors_[i], CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(none)"));
        SendMessageW(selectors_[i], CB_SETCURSEL, 0, 0);
    }
    sync_columns(); refresh(0);
}

void DataPage::sync_columns() {
    const auto& keys = capture_->keys();
    if (columns_ == 0) {
        LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH; col.cx = 80;
        col.pszText = const_cast<wchar_t*>(L"Row"); ListView_InsertColumn(table_, 0, &col);
        col.cx = 110; col.pszText = const_cast<wchar_t*>(L"Time (s)"); ListView_InsertColumn(table_, 1, &col);
        columns_ = 2;
    }
    while (columns_ < keys.size() + 2) {
        const auto name = wide(keys[columns_ - 2]);
        LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH; col.cx = std::max(115, static_cast<int>(name.size()) * 9 + 24);
        col.pszText = const_cast<wchar_t*>(name.c_str()); ListView_InsertColumn(table_, static_cast<int>(columns_), &col);
        for (auto selector : selectors_) SendMessageW(selector, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        ++columns_;
    }
    for (size_t i = 0; i < 4; ++i) {
        if (!assigned_[i] && keys.size() > i)
            select_field(i, keys[i]);
    }
}

void DataPage::append(const std::unordered_map<std::string, double>& values, double timestamp) {
    now_ = timestamp;
    for (size_t i = 0; i < 4; ++i) {
        const auto it = values.find(selected_[i]);
        if (it != values.end()) models_[i].update_from_kv({*it}, timestamp);
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
    ListView_SetItemCountEx(table_, static_cast<int>(rows.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    first_sequence_ = rows.empty() ? 0 : rows.front()->sequence;
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
        std::to_wstring(capture_->keys().size()) + L" | Old rows discarded: " + std::to_wstring(capture_->dropped()) +
        L" | 32 MiB max; Save As = retained rows";
    SetWindowTextW(info_, info.c_str());
    for (size_t i = 0; i < 4; ++i) plots_[i].update_from_model(timestamp);
}

void DataPage::set_follow(bool enabled) {
    SendMessageW(follow_, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    if (enabled && ListView_GetItemCount(table_) > 0) ListView_EnsureVisible(table_, ListView_GetItemCount(table_) - 1, FALSE);
}
bool DataPage::follows() const { return SendMessageW(follow_, BM_GETCHECK, 0, 0) == BST_CHECKED; }

void DataPage::select_field(size_t chart, const std::string& key) {
    if (chart >= 4) return;
    const auto& keys = capture_->keys();
    const auto found = std::find(keys.begin(), keys.end(), key);
    selected_[chart] = found == keys.end() ? "" : key;
    assigned_[chart] = true;
    SendMessageW(selectors_[chart], CB_SETCURSEL, found == keys.end() ? 0 : 1 + (found - keys.begin()), 0);
    rebuild_chart(chart);
}

void DataPage::rebuild_chart(size_t chart) {
    models_[chart].reset(); plots_[chart].reset_visual();
    const auto& keys = capture_->keys();
    const auto found = std::find(keys.begin(), keys.end(), selected_[chart]);
    if (found != keys.end()) {
        const auto column = static_cast<size_t>(found - keys.begin());
        const auto& rows = capture_->rows();
        // Rebase retained relative timestamps to the current monotonic clock.
        const double origin = rows.empty() ? 0 : now_ - rows.back()->seconds;
        for (const auto& row : rows) {
            double value;
            if (CaptureStore::value(*row, column, value)) models_[chart].update_from_kv({{*found, value}}, origin + row->seconds);
        }
        models_[chart].prune(now_);
    }
    plots_[chart].update_from_model(now_);
}

void DataPage::layout(int w, int h) {
    if (!IsWindow(table_)) return;
    const int row = std::max(26, control_font_height(follow_) + 10), gap = 8;
    const int check_w = control_text_width(follow_, GetSystemMetrics(SM_CXMENUCHECK) + 20);
    MoveWindow(follow_, 0, 0, check_w, row, TRUE);
    MoveWindow(info_, check_w + gap, 4, std::max(1, w - check_w - gap), row, TRUE);
    const int table_h = std::max(100, (h - row - 2 * gap) / 2);
    MoveWindow(table_, 0, row + gap, w, table_h, TRUE);
    const int top = row + gap + table_h + gap;
    const int tile_w = (w - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i) {
        const int x = i * (tile_w + gap);
        MoveWindow(selectors_[i], x, top, std::max(50, tile_w - 86), 300, TRUE);
        MoveWindow(time_selectors_[i], x + tile_w - 78, top, 78, 200, TRUE);
        MoveWindow(plots_[i].hwnd(), x, top + row + 4, tile_w, std::max(50, h - top - row - 4), TRUE);
    }
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
    if (msg == WM_COMMAND) {
        const auto id = LOWORD(wp);
        if (id == 1) { set_follow(follows()); return 0; }
        if (id >= 10 && id <= 13 && HIWORD(wp) == CBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageW(selectors_[id - 10], CB_GETCURSEL, 0, 0));
            selected_[id - 10] = index > 0 ? capture_->keys()[index - 1] : "";
            assigned_[id - 10] = true;
            rebuild_chart(id - 10); return 0;
        }
        if (id >= 20 && id <= 23 && HIWORD(wp) == CBN_SELCHANGE) {
            const size_t chart = id - 20;
            const int index = static_cast<int>(SendMessageW(time_selectors_[chart], CB_GETCURSEL, 0, 0));
            const double choices[] = {5, 10, 30, 60, 120};
            if (index >= 0 && index < 5) {
                seconds_[chart] = choices[index]; models_[chart].set_time_window(seconds_[chart]);
                plots_[chart].set_time_window(seconds_[chart]); rebuild_chart(chart);
            }
            return 0;
        }
    }
    if (msg == WM_NOTIFY && reinterpret_cast<NMHDR*>(lp)->hwndFrom == table_) {
        auto header = reinterpret_cast<NMHDR*>(lp);
        if (header->code == LVN_GETDISPINFOW) {
            auto info = reinterpret_cast<NMLVDISPINFOW*>(lp);
            if ((info->item.mask & LVIF_TEXT) && info->item.iItem >= 0 &&
                static_cast<size_t>(info->item.iItem) < capture_->rows().size()) {
                const auto& row = *capture_->rows()[info->item.iItem];
                wchar_t buffer[80] = {};
                if (info->item.iSubItem == 0) swprintf_s(buffer, L"%llu", static_cast<unsigned long long>(row.sequence));
                else if (info->item.iSubItem == 1) swprintf_s(buffer, L"%.3f", row.seconds);
                else {
                    double value;
                    if (CaptureStore::value(row, static_cast<size_t>(info->item.iSubItem - 2), value)) swprintf_s(buffer, L"%.10g", value);
                }
                if (info->item.pszText && info->item.cchTextMax > 0)
                    wcsncpy_s(info->item.pszText, info->item.cchTextMax, buffer, _TRUNCATE);
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}
