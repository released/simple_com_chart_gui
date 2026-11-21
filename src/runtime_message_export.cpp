#include "mfc_main_dialog.h"
#include "capture_export.h"

bool CMainDialog::verify_message_export() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"message-export-result.txt");
    bool pass = true;
    // This fixture exercises visible escaped ASCII, independent of personal defaults.
    const bool original_hide_nul=message_page_.hide_nul_;
    const bool original_terminal=message_page_.terminal_;
    struct DisplayRestore {std::function<void()> action;~DisplayRestore(){action();}};
    DisplayRestore restore{[&]{message_page_.hide_nul_=original_hide_nul;message_page_.terminal_=original_terminal;}};
    message_page_.hide_nul_=false;message_page_.terminal_=false;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    const auto read = [](const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary); return std::string(std::istreambuf_iterator<char>(file), {});
    };
    const auto mode = [&](int index) {
        ::SendMessageW(message_page_.mode_control(), CB_SETCURSEL, index, 0);
        ::SendMessageW(message_page_.hwnd(), WM_COMMAND, MAKEWPARAM(1, CBN_SELCHANGE), 0);
    };
    const auto wait = [&] {
        const auto deadline = GetTickCount64() + 2000;
        while (exporting_ && GetTickCount64() < deadline) {
            MSG message{};
            if (::PeekMessageW(&message, m_hWnd, WM_APP + 2, WM_APP + 2, PM_REMOVE)) ::DispatchMessageW(&message);
            else std::this_thread::sleep_for(std::chrono::milliseconds(1));
            OnExportComplete(0,0);
        }
        return !exporting_;
    };
    ::SetWindowTextW(edit_include_, L"fan,"); ::SetWindowTextW(edit_exclude_, L"error");
    ::SendMessageW(combo_filter_, CB_SETCURSEL, 0, 0); check("filtered capture fixture is empty", apply_filter(false) && capture_.rows().empty());
    message_page_.reset(); TabCtrl_SetCurSel(tabs_, kMessagesTab); switch_page(); mode(0);
    check("empty raw history does not start export", !begin_message_export(evidence / L"messages-empty.log") && !exporting_);
    const std::string raw = std::string("A\0\xFF\r\n", 5) + "other,error:9\r\npartial";
    message_page_.append(raw); message_page_.refresh();
    const std::string ascii = "A\\x00\\xFF\r\nother,error:9\r\npartial";
    check("ASCII export is the current display text, not filtered capture", message_page_.export_text() == ascii && capture_.rows().empty());
    const auto ascii_path = evidence / L"messages-saved-ascii.log";
    check("Messages can save with no numeric/accepted capture", begin_message_export(ascii_path) && exporting_ && !::IsWindowEnabled(btn_save_));
    check("second export is rejected while busy", !begin_message_export(evidence / L"messages-duplicate.log"));
    OnCancel(); check("Messages export shares the close guard", exporting_ && ::IsWindow(m_hWnd));
    for (int page : {kChartTab, kDataTab, kMessagesTab}) {
        TabCtrl_SetCurSel(tabs_, page); switch_page();
        check("tab changes do not re-enable Save As while saving", !::IsWindowEnabled(btn_save_));
    }
    message_page_.append(" later"); message_page_.refresh(); mode(1);
    check("ASCII worker completes and re-enables Save As on Messages", wait() && ::IsWindowEnabled(btn_save_));
    check("later receive/mode changes do not alter immutable ASCII snapshot", read(ascii_path) == ascii && message_page_.saved_log_bytes_==ascii.size() && export_result_.find(L"Saved Messages (ASCII)") == 0);
    const auto hex = message_page_.export_text();
    check("HEX export includes original NUL FF CR LF and offsets", hex.find("00000000  41 00 FF 0D 0A") == 0 && hex != ascii);
    const auto hex_path = evidence / L"messages-saved-hex.txt";
    check("HEX text export starts", begin_message_export(hex_path));
    message_page_.reset(); mode(0);
    check("HEX snapshot survives Clear and mode changes", wait() && read(hex_path) == hex && export_result_.find(L"Saved Messages (HEX)") == 0);
    check("raw exports do not fabricate filtered capture", capture_.rows().empty());
    message_page_.append("replacement"); message_page_.refresh();
    const auto locked_path = evidence / L"messages-locked.log";
    std::filesystem::copy_file(ascii_path, locked_path, std::filesystem::copy_options::overwrite_existing);
    const auto handle = CreateFileW(locked_path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    check("test-owned existing destination locked", handle != INVALID_HANDLE_VALUE);
    if (handle != INVALID_HANDLE_VALUE) {
        check("locked destination uses worker failure path", begin_message_export(locked_path) && wait() && export_result_.find(L"Save failed:") == 0);
        CloseHandle(handle);
        check("failed Messages save preserves original file and restores button", read(locked_path) == ascii && ::IsWindowEnabled(btn_save_));
    }
    check("confirmed replacement succeeds after unlock", begin_message_export(locked_path) && wait() && read(locked_path) == "replacement");
    check("unsupported Messages CSV rejected without creating a file", begin_message_export(evidence / L"messages-not-supported.csv") && wait() &&
        export_result_.find(L"Save failed:") == 0 && !std::filesystem::exists(evidence / L"messages-not-supported.csv"));
    auto release=std::make_shared<std::atomic<bool>>(false);
    const auto typed_path=evidence/L"typed-success.log";
    check("held export starts",begin_export_job([release,typed_path]{while(!release->load())Sleep(1);export_message_text("typed",typed_path);},L"completed without a success prefix"));
    message_export_pending_=true;message_page_.stage_log_export(typed_path.filename().wstring(),5);
    OnExportComplete(0,0);
    check("premature or stale wake cannot complete an active writer",exporting_&&!::IsWindowEnabled(btn_save_));
    OnCancel();check("close retains ownership of held export",exporting_&&::IsWindow(m_hWnd));
    *release=true;
    // Drop every posted completion hint, then exercise the actual timer fallback.
    const auto deadline=GetTickCount64()+2000;
    while(exporting_&&GetTickCount64()<deadline){
        MSG hint{};while(::PeekMessageW(&hint,m_hWnd,WM_APP+2,WM_APP+2,PM_REMOVE)){}
        OnTimer(IDT_UI);Sleep(1);
    }
    check("timer collects retained result when all completion hints are lost",!exporting_&&export_result_==L"completed without a success prefix");
    check("message success uses committed outcome rather than presentation prefix",read(typed_path)=="typed"&&message_page_.saved_log_==typed_path.filename().wstring());
    OnExportComplete(0,0);check("duplicate completion is harmless",!exporting_&&::IsWindowEnabled(btn_save_));
    check("unknown worker exception reaches visible failure and can retry",begin_export_job([]{throw 17;},L"unused")&&wait()&&export_result_.find(L"Save failed:")==0);
    message_page_.reset();
    message_page_.hide_nul_=true;message_page_.append(std::string(4,'\0'));
    check("hidden-only export reports empty data and preserves existing target",begin_message_export(locked_path)&&wait()&&export_result_==L"No messages to save."&&read(locked_path)=="replacement");
    message_page_.hide_nul_=false;message_page_.reset();
    message_page_.append(std::string(MessageBuffer::kCapacity + 16, 'x')); mode(1);
    check("retained HEX export starts at eviction offset", message_page_.export_text().find("00000010  78") == 0);
    message_page_.reset(); mode(0);
    ::SetWindowTextW(edit_include_, L""); ::SetWindowTextW(edit_exclude_, L""); check("restore filter fixture", apply_filter(false));
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": Messages ASCII/HEX immutable text export, atomic failure and shared worker/close guard\n";
    return pass;
}
