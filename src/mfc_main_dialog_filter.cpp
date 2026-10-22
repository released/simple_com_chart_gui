#include "mfc_main_dialog.h"

#include <cmath>
#include <filesystem>

namespace {
std::wstring text(HWND control) {
    std::wstring value(static_cast<size_t>(::GetWindowTextLengthW(control)) + 1, L'\0');
    ::GetWindowTextW(control, value.data(), static_cast<int>(value.size()));
    value.resize(value.size() - 1);
    return value;
}

std::string utf8(const std::wstring& value) {
    const auto length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring wide(const std::string& value) {
    const auto n = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), n); return result;
}
}

void CMainDialog::build_filter_ui() {
    const auto label = [this](const wchar_t* caption) {
        return CreateWindowW(L"STATIC", caption, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);
    };
    const auto edit = [this](const wchar_t* value, int id) {
        return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", value,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0,
            m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    };
    label_include_ = label(L"Include");
    edit_include_ = edit(L"", 501);
    label_exclude_ = label(L"Exclude");
    edit_exclude_ = edit(L"", 502);
    ::SendMessageW(edit_include_, EM_SETLIMITTEXT, 256, 0);
    ::SendMessageW(edit_exclude_, EM_SETLIMITTEXT, 256, 0);
    combo_filter_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        0, 0, 90, 150, m_hWnd, reinterpret_cast<HMENU>(503), nullptr, nullptr);
    ::SendMessageW(combo_filter_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Text"));
    ::SendMessageW(combo_filter_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Regex"));
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    label_limit_ = label(L"Max CH");
    edit_limit_ = edit(L"256", 504);
    ::SendMessageW(edit_limit_, EM_SETLIMITTEXT, 4, 0);
    btn_apply_ = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(505), nullptr, nullptr);
    label_filter_ = label(L"Filter: all lines. Apply starts a new capture; empty Include accepts all, empty Exclude drops none.");
    ::SetWindowLongPtrW(label_filter_, GWL_STYLE, ::GetWindowLongPtrW(label_filter_, GWL_STYLE) | SS_NOPREFIX);
}

void CMainDialog::set_filter_status(const std::wstring& message) {
    ::SetWindowTextW(label_filter_, message.c_str());
    if (ui_ready_) {
        CRect client; GetClientRect(&client); layout_ui(client.Width(), client.Height());
        RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
}

bool CMainDialog::apply_filter(bool save) {
    const auto limit_text = text(edit_limit_);
    const int limit = _wtoi(limit_text.c_str());
    if (limit < 1 || limit > 1024 || limit_text.find_first_not_of(L"0123456789") != std::wstring::npos) {
        set_filter_status(L"Max CH must be 1..1024. Previous filter remains active.");
        return false;
    }
    std::string error;
    const bool regex = ::SendMessageW(combo_filter_, CB_GETCURSEL, 0, 0) == 1;
    if (!line_filter_.configure(utf8(text(edit_include_)), utf8(text(edit_exclude_)), regex, error)) {
        const auto message = std::wstring(error.begin(), error.end()) + L" Previous filter remains active.";
        set_filter_status(message);
        log_line(std::wstring(error.begin(), error.end()));
        return false;
    }
    model_.set_max_channels(limit);
    active_include_ = text(edit_include_);
    active_exclude_ = text(edit_exclude_);
    active_regex_ = regex;
    model_.reset();
    capture_.reset(static_cast<size_t>(limit));
    data_page_.reset();
    channel_panel_.reset();
    plot_view_.set_frozen(false);
    snapshot_ = false;
    ::SendMessageW(btn_snapshot_, BM_SETCHECK, BST_UNCHECKED, 0);
    ::SetWindowTextW(btn_snapshot_, L"Snapshot");
    plot_view_.reset_visual();
    plot_view_.update_from_model(0);
    set_right_status(L"Samples: 0 | CH: 0");
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_.lines.clear();
    }
    set_filter_status(std::wstring(L"Filter applied (") + (regex ? L"Regex" : L"Text") + L", case-sensitive). New capture started.");
    if (save) save_settings();
    return true;
}

void CMainDialog::load_settings() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (settings_path_.empty()) {
        settings_path_ = exe;
        settings_path_ = settings_path_.substr(0, settings_path_.find_last_of(L"\\/") + 1) + L"SimpleComChart.ini";
    }
    const auto read = [this](const wchar_t* key, const wchar_t* fallback) {
        wchar_t value[1024] = {};
        GetPrivateProfileStringW(L"Capture", key, fallback, value, 1024, settings_path_.c_str());
        return std::wstring(value);
    };
    ::SetWindowTextW(edit_include_, read(L"Include", L"").c_str());
    ::SetWindowTextW(edit_exclude_, read(L"Exclude", L"").c_str());
    ::SetWindowTextW(edit_limit_, read(L"MaxChannels", L"256").c_str());
    ::SendMessageW(combo_filter_, CB_SETCURSEL, GetPrivateProfileIntW(L"Capture", L"Regex", 0, settings_path_.c_str()) ? 1 : 0, 0);
    apply_filter(false);
    std::array<std::string, 4> fields;
    for (size_t i = 0; i < fields.size(); ++i) {
        wchar_t value[256] = {};
        const auto key = L"Chart" + std::to_wstring(i + 1);
        GetPrivateProfileStringW(L"FourCharts", key.c_str(), L"", value, 256, settings_path_.c_str());
        std::wstring name = value;
        if (name.rfind(L"key:", 0) == 0) name.erase(0, 4);
        fields[i] = utf8(name);
        if (fields[i].size() > 160 || std::any_of(fields[i].begin(), fields[i].end(),
            [](unsigned char ch) { return ch < 32; })) fields[i].clear();
    }
    data_page_.set_selected_fields(fields);
    message_page_.set_font_points(static_cast<int>(GetPrivateProfileIntW(L"Messages", L"FontSizePt",
        MessagePage::kDefaultFontPoints, settings_path_.c_str())));
    load_serial_settings();
}

void CMainDialog::save_settings() {
    if (settings_path_.empty()) return;
    // Save only a valid applied configuration, never uncommitted edits.
    if (!ui_ready_) return;
    if (GetFileAttributesW(settings_path_.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // A UTF-16 BOM lets Windows profile APIs preserve arbitrary Unicode filters.
        std::ofstream ini(std::filesystem::path(settings_path_), std::ios::binary);
        ini.put(static_cast<char>(0xff));
        ini.put(static_cast<char>(0xfe));
    }
    const auto write = [this](const wchar_t* key, const std::wstring& value) {
        return WritePrivateProfileStringW(L"Capture", key, value.c_str(), settings_path_.c_str()) != FALSE;
    };
    bool ok = write(L"Include", active_include_) && write(L"Exclude", active_exclude_) &&
        write(L"Regex", active_regex_ ? L"1" : L"0") &&
        write(L"MaxChannels", std::to_wstring(model_.get_max_channels()));
    for (size_t i = 0; i < 4; ++i) {
        const auto key = L"Chart" + std::to_wstring(i + 1);
        const auto& selected = data_page_.selected_field(i);
        // Prefix prevents profile APIs from stripping quotes around a field name.
        const auto value = selected.empty() ? L"" : L"key:" + wide(selected);
        ok = (WritePrivateProfileStringW(L"FourCharts", key.c_str(), value.c_str(), settings_path_.c_str()) != FALSE) && ok;
    }
    ok = (WritePrivateProfileStringW(L"Messages", L"FontSizePt", std::to_wstring(message_page_.font_points()).c_str(),
        settings_path_.c_str()) != FALSE) && ok;
    ok = save_serial_settings() && ok;
    if (!ok) show_status_message(L"Cannot save INI beside EXE. Use a writable folder.", 5000);
}

LRESULT CMainDialog::OnChartSelectionChanged(WPARAM, LPARAM) {
    save_settings(); return 0;
}

LRESULT CMainDialog::OnMessageFontChanged(WPARAM, LPARAM) {
    save_settings(); return 0;
}

void CMainDialog::feed_demo() {
    if (++demo_tick_ % 4 != 0) return;
    const double phase = static_cast<double>(demo_tick_) / 12.0;
    const auto line = std::string("fan,duty:920,target:2340,pri_ntc:76,fg_hz:") +
        std::to_string(69.1 + 3 * std::sin(phase)) + ",fg_rpm:" + std::to_string(2074 + 80 * std::sin(phase)) +
        ",offset:" + std::to_string(-2.5 + std::sin(phase)) + ",err:11%,cap:105/76/29,tick:612/609/2453,cmd:0x3202";
    std::lock_guard<std::mutex> lock(pending_mutex_);
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    const double now = static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
    pending_.lines.push_back({line, now});
    const std::string ac = "ac_adc_mv:1169,calibrated:1169,ac_v:116,ocp_mv:377,ntc_mv:2439,ntc_c:76,cnt:554";
    pending_.lines.push_back({ac, now});
    pending_.raw.append(line + "\r\n" + ac + "\r\n");
}
