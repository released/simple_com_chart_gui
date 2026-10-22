#include "data_page.h"
#include <commctrl.h>
#include <cstdio>

void DataPage::rebuild_columns() {
    const auto states = field_panel_.get_checkbox_state_map();
    visible_columns_.clear();
    const auto& keys = capture_->keys();
    for (size_t i = 0; i < keys.size(); ++i) {
        const auto found = states.find(keys[i]);
        if (found != states.end() && found->second) visible_columns_.push_back(i);
    }
    SendMessageW(table_, WM_SETREDRAW, FALSE, 0);
    while (ListView_DeleteColumn(table_, 0)) {}
    column_widths_.clear();
    HDC dc = GetDC(table_); const auto old = SelectObject(dc, font_);
    const auto insert = [&](const std::wstring& title) {
        SIZE size{}; GetTextExtentPoint32W(dc, title.c_str(), static_cast<int>(title.size()), &size);
        LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        col.fmt = LVCFMT_LEFT; col.cx = std::max(48L, size.cx + 24); col.pszText = const_cast<wchar_t*>(title.c_str());
        ListView_InsertColumn(table_, static_cast<int>(column_widths_.size()), &col);
        column_widths_.push_back(col.cx);
    };
    insert(L"Row"); insert(L"Time (s)");
    for (const auto column : visible_columns_) {
        const auto& key = keys[column];
        const int n = MultiByteToWideChar(CP_UTF8, 0, key.data(), static_cast<int>(key.size()), nullptr, 0);
        std::wstring title(n, 0);
        MultiByteToWideChar(CP_UTF8, 0, key.data(), static_cast<int>(key.size()), title.data(), n); insert(title);
    }
    SelectObject(dc, old); ReleaseDC(table_, dc);
    fit_columns();
    // Reset obsolete horizontal scrolling without disturbing the inspected row.
    ListView_Scroll(table_, -GetScrollPos(table_, SB_HORZ), 0);
    SendMessageW(table_, WM_SETREDRAW, TRUE, 0); InvalidateRect(table_, nullptr, TRUE);
}

void DataPage::fit_columns() {
    if (!IsWindow(table_) || column_widths_.empty()) return;
    const auto& rows = capture_->rows();
    // Sample at most 64 recent rows and ~2048 cells, never all retained data.
    const size_t count = std::min(rows.size(), std::min<size_t>(64, std::max<size_t>(1, 2048 / column_widths_.size())));
    HDC dc = GetDC(table_); const auto old = SelectObject(dc, font_);
    for (size_t column = 0; column < column_widths_.size(); ++column) {
        int width = column_widths_[column];
        for (size_t r = rows.size() - count; r < rows.size(); ++r) {
            const auto& row = *rows[r];
            wchar_t text[80] = {};
            if (column == 0) swprintf_s(text, L"%llu", static_cast<unsigned long long>(row.sequence));
            else if (column == 1) swprintf_s(text, L"%.3f", row.seconds);
            else {
                double value;
                if (CaptureStore::value(row, visible_columns_[column - 2], value)) swprintf_s(text, L"%.10g", value);
            }
            SIZE size{}; GetTextExtentPoint32W(dc, text, static_cast<int>(wcslen(text)), &size);
            width = std::max(width, static_cast<int>(size.cx) + 24);
        }
        column_widths_[column] = width;
        if (ListView_GetColumnWidth(table_, static_cast<int>(column)) != width)
            ListView_SetColumnWidth(table_, static_cast<int>(column), width);
    }
    SelectObject(dc, old); ReleaseDC(table_, dc);
}
