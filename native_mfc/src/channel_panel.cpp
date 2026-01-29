#include "channel_panel.h"

#include <commctrl.h>
#include <cstdio>

namespace {
constexpr int kValueWidth = 70;
constexpr int kPadding = 6;

std::wstring to_wstring(const std::string& s) {
    if (s.empty()) {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) {
        return L"";
    }
    std::wstring out(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}

std::wstring to_wstring(int value) {
    wchar_t buf[64] = {};
    _snwprintf_s(buf, 64, _TRUNCATE, L"%d", value);
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
    index_map_[key] = index;
    color_map_[key] = color;

    suppress_notify_ = true;
    LVITEMW item = {};
    item.mask = LVIF_TEXT;
    item.iItem = index;
    std::wstring wkey = to_wstring(key);
    item.pszText = const_cast<wchar_t*>(wkey.c_str());
    ListView_InsertItem(list_, &item);
    ListView_SetCheckState(list_, index, enabled ? TRUE : FALSE);
    ListView_SetItemText(list_, index, 1, const_cast<wchar_t*>(L"--"));
    suppress_notify_ = false;
}

void ChannelPanel::update_values(const std::unordered_map<std::string, int>& latest) {
    if (!list_) {
        return;
    }

    for (size_t i = 0; i < keys_.size(); ++i) {
        const auto& key = keys_[i];
        auto it = latest.find(key);
        if (it == latest.end()) {
            ListView_SetItemText(list_, static_cast<int>(i), 1, const_cast<wchar_t*>(L"--"));
        } else {
            std::wstring val = to_wstring(it->second);
            ListView_SetItemText(list_, static_cast<int>(i), 1, const_cast<wchar_t*>(val.c_str()));
        }
    }
}

std::unordered_map<std::string, bool> ChannelPanel::get_checkbox_state_map() const {
    std::unordered_map<std::string, bool> result;
    if (!list_) {
        return result;
    }
    for (size_t i = 0; i < keys_.size(); ++i) {
        bool checked = ListView_GetCheckState(list_, static_cast<int>(i)) != FALSE;
        result[keys_[i]] = checked;
    }
    return result;
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
                    auto it = color_map_.find(keys_[index]);
                    if (it != color_map_.end()) {
                        cd->clrText = it->second;
                    }
                }
                return CDRF_DODEFAULT;
            }
        } else if (hdr->hwndFrom == list_ && hdr->code == LVN_ITEMCHANGED) {
            auto* nmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);
            if ((nmlv->uChanged & LVIF_STATE) != 0) {
                UINT state_change = (nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK;
                if (state_change != 0 && !suppress_notify_) {
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
    MoveWindow(label_count_, kPadding, kPadding, w - 2 * kPadding, 20, TRUE);
    MoveWindow(btn_all_, kPadding, kPadding + 24, 60, 22, TRUE);
    MoveWindow(btn_none_, kPadding + 70, kPadding + 24, 60, 22, TRUE);

    int list_y = kPadding + 52;
    int list_h = h - list_y - kPadding;
    int list_w = w - 2 * kPadding;
    MoveWindow(list_, kPadding, list_y, list_w, list_h, TRUE);

    int value_w = kValueWidth;
    int channel_w = list_w - value_w - 6;
    if (channel_w < 80) {
        channel_w = 80;
    }
    ListView_SetColumnWidth(list_, 0, channel_w);
    ListView_SetColumnWidth(list_, 1, value_w);
}

void ChannelPanel::on_all_none(bool all_checked) {
    if (!list_) {
        return;
    }
    suppress_notify_ = true;
    for (size_t i = 0; i < keys_.size(); ++i) {
        ListView_SetCheckState(list_, static_cast<int>(i), all_checked ? TRUE : FALSE);
    }
    suppress_notify_ = false;
    send_channel_changed();
}

void ChannelPanel::send_channel_changed() {
    if (parent_) {
        SendMessageW(parent_, WM_APP + 1, 0, 0);
    }
}
