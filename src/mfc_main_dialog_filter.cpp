#include "mfc_main_dialog.h"
#include "ui_localized_controls.h"

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
        return ui::create_window(L"STATIC", caption, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);
    };
    const auto edit = [this](const wchar_t* value, int id) {
        return ui::create_window_ex(WS_EX_CLIENTEDGE, L"EDIT", value,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0,
            m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    };
    label_include_ = label(L"Include");
    edit_include_ = edit(L"", 501);
    label_exclude_ = label(L"Exclude");
    edit_exclude_ = edit(L"", 502);
    ::SendMessageW(edit_include_, EM_SETLIMITTEXT, 256, 0);
    ::SendMessageW(edit_exclude_, EM_SETLIMITTEXT, 256, 0);
    combo_filter_ = ui::create_window(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        0, 0, 90, 150, m_hWnd, reinterpret_cast<HMENU>(503), nullptr, nullptr);
    ui::choices(combo_filter_);::SendMessageW(combo_filter_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Text"));
    ::SendMessageW(combo_filter_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Regex"));
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0);
    label_limit_ = label(L"Max CH");
    edit_limit_ = edit(L"256", 504);
    ::SendMessageW(edit_limit_, EM_SETLIMITTEXT, 4, 0);
    btn_apply_ = ui::create_window(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
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

void CMainDialog::reset_numeric_capture(bool clear_messages) {
    model_.reset();
    extracted_clock_=false;extracted_origin_=extracted_base_=extracted_now_=0;
    parser_skipped_=0;diagnostics_.reset();
    for(auto& sequence:sequences_)sequence.reset();
    numeric_sources_.clear();source_collisions_=0;numeric_secondary_seen_=false;
    for(auto& engine:derived_)engine.reset();
    derived_generations_={{UINT64_MAX,UINT64_MAX}};
    capture_.reset(static_cast<size_t>(model_.get_max_channels()));
    data_page_.reset();channel_panel_.reset();
    snapshot_=false;plot_view_.set_frozen(false);
    ::SendMessageW(btn_snapshot_,BM_SETCHECK,BST_UNCHECKED,0);
    ::SetWindowTextW(btn_snapshot_,L"Snapshot");
    plot_view_.reset_visual();plot_view_.update_from_model(0);
    if(clear_messages)message_page_.reset();
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        if(clear_messages){
            pending_=PendingData{};pending_dropped_=0;pending_line_loss_=0;oversized_lines_=0;
            source_lines_=SourceLineTracker{};source_lines_.configure(numeric_framing_[replay_.data()?2:0]);
        }
        else pending_.lines.clear();
    }
    set_right_status(L"Samples: 0 | CH: 0");
}

bool CMainDialog::apply_filter(bool save) {
    const CaptureSettingsInput input{text(edit_include_), text(edit_exclude_),
        ::SendMessageW(combo_filter_, CB_GETCURSEL, 0, 0) == 1 ? L"1" : L"0", text(edit_limit_)};
    CaptureSettings candidate;
    std::wstring validation;
    if (!validate_capture_settings(input, candidate, validation)) { set_filter_status(validation); return false; }
    std::string error;
    LineFilter filter;
    if (!filter.configure(utf8(candidate.include), utf8(candidate.exclude), candidate.regex, error)) {
        const auto message = std::wstring(error.begin(), error.end()) + L" Previous filter remains active.";
        set_filter_status(message);
        log_line(std::wstring(error.begin(), error.end()));
        return false;
    }
    // Persistence precedes every live-state change, including capture/Snapshot reset.
    if (save && !persist_settings(candidate.patch())) {
        set_filter_status(L"Cannot save Capture settings. Previous filter and capture retained.");
        return false;
    }
    line_filter_ = std::move(filter);
    recorder_.event("NUMERIC CONFIG CHANGED",message_unix_ms());
    model_.set_max_channels(candidate.max_channels);
    filter_configured_=true;
    active_include_ = std::move(candidate.include);
    active_exclude_ = std::move(candidate.exclude);
    active_regex_ = candidate.regex;
    reset_numeric_capture();
    set_filter_status(std::wstring(L"Filter applied (") + (active_regex_ ? L"Regex" : L"Text") + L", case-sensitive). New capture started.");
    update_source_status();
    return true;
}

void CMainDialog::load_settings() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (settings_path_.empty()) {
        settings_path_ = exe;
        settings_path_ = settings_path_.substr(0, settings_path_.find_last_of(L"\\/") + 1) + L"SimpleComChart.ini";
    }
    const auto saved = load_capture_settings(settings_path_);
    ::SetWindowTextW(edit_include_, saved.include.c_str());
    ::SetWindowTextW(edit_exclude_, saved.exclude.c_str());
    ::SetWindowTextW(edit_limit_, saved.max_channels.c_str());
    ::SendMessageW(combo_filter_, CB_SETCURSEL, saved.regex == L"1" ? 1 : 0, 0);
    CaptureSettings checked; std::wstring error;
    // Invalid saved settings remain on disk and never become a permissive active filter.
    filter_configured_ = false;
    if (validate_capture_settings(saved, checked, error)) apply_filter(false);
    else set_filter_status(error);
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
    message_page_.load_options(settings_path_);
    load_extraction();
    load_input_framing();
    load_display_preferences();
    load_serial_settings();
    load_binary_replay();
    load_derived();
    load_lab_preferences();
}

bool CMainDialog::persist_settings(const std::vector<IniSetting>& values) {
    std::wstring error;
    if (save_ini_patch(settings_path_, values, error)) return true;
    show_status_message(error, 10000); return false;
}

bool CMainDialog::save_settings() {
    if (settings_path_.empty() || !ui_ready_) return false;
    // An invalid loaded Capture block is never overwritten by startup/shutdown saves.
    std::vector<IniSetting> values;
    if (filter_configured_) values = CaptureSettings{active_include_, active_exclude_, active_regex_, model_.get_max_channels()}.patch();
    const auto options = message_page_.option_settings();
    values.insert(values.end(), options.begin(), options.end());
    values.push_back({L"Messages", L"FontSizePt", std::to_wstring(message_page_.font_points())});
    for (size_t i = 0; i < 4; ++i) {
        const auto key = L"Chart" + std::to_wstring(i + 1);
        const auto& selected = data_page_.selected_field(i);
        // Prefix prevents profile APIs from stripping quotes around a field name.
        const auto value = selected.empty() ? L"" : L"key:" + wide(selected);
        values.push_back({L"FourCharts", key, value});
    }
    return persist_settings(values);
}

bool CMainDialog::save_message_settings() {
    if (!ui_ready_) return false;
    auto values = message_page_.option_settings();
    values.push_back({L"Messages", L"FontSizePt", std::to_wstring(message_page_.font_points())});
    return persist_settings(values);
}

bool CMainDialog::save_chart_settings() {
    if (!ui_ready_) return false;
    std::vector<IniSetting> values;
    for (size_t i = 0; i < 4; ++i) {
        const auto& selected = data_page_.selected_field(i);
        values.push_back({L"FourCharts", L"Chart" + std::to_wstring(i + 1), selected.empty() ? L"" : L"key:" + wide(selected)});
    }
    return persist_settings(values);
}

LRESULT CMainDialog::OnChartSelectionChanged(WPARAM, LPARAM) {
    save_chart_settings(); return 0;
}

LRESULT CMainDialog::OnMessageFontChanged(WPARAM, LPARAM) {
    save_message_settings(); return 0;
}
