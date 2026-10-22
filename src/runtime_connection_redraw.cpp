#include "mfc_main_dialog.h"
#include "runtime_capture.h"
#include "ui_metrics.h"

namespace {
struct PositionProbe {
    unsigned events = 0;
    std::vector<HWND> controls;
    static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
        if (message == WM_WINDOWPOSCHANGING) ++reinterpret_cast<PositionProbe*>(data)->events;
        return DefSubclassProc(window, message, wp, lp);
    }
    explicit PositionProbe(std::initializer_list<HWND> windows) {
        for (const auto window : windows) if (::IsWindow(window) &&
            SetWindowSubclass(window, proc, 1, reinterpret_cast<DWORD_PTR>(this))) controls.push_back(window);
    }
    ~PositionProbe() { for (const auto window : controls) RemoveWindowSubclass(window, proc, 1); }
};
std::vector<RECT> child_bounds(HWND parent) {
    std::vector<RECT> result;
    for (auto child = ::GetWindow(parent, GW_CHILD); child; child = ::GetWindow(child, GW_HWNDNEXT)) {
        RECT bounds{}; ::GetWindowRect(child, &bounds); result.push_back(bounds);
    }
    return result;
}
bool equal_bounds(const std::vector<RECT>& before, const std::vector<RECT>& after) {
    return before.size() == after.size() && std::equal(before.begin(), before.end(), after.begin(),
        [](const RECT& a, const RECT& b) { return EqualRect(&a, &b) != FALSE; });
}
}

bool CMainDialog::verify_connection_redraw() {
    const auto evidence = std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(evidence / L"connection-redraw-result.txt");
    bool pass = true;
    const auto check = [&](const char* label, bool ok) { report << (ok ? "PASS: " : "FAIL: ") << label << '\n'; pass = ok && pass; };
    check("test starts disconnected with no pending driver open", connection_.mode() == SerialConnectionState::Mode::Idle && !serial_open_.busy());
    if (!pass) return false;
    ShowWindow(SW_RESTORE);
    SerialSettings settings; settings.port = {L"COM4", L"Simulated bridge", L"TEST\\REDRAW", L"fake://redraw"};
    for (const int width : {1120, 1500}) {
        SetWindowPos(nullptr, 0, 0, width, 780, SWP_NOMOVE | SWP_NOZORDER);
        for (int page = 0; page < 3; ++page) {
            TabCtrl_SetCurSel(tabs_, page); switch_page();
            RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            const auto before = child_bounds(m_hWnd);
            PositionProbe probe{channel_panel_.hwnd(), plot_view_.hwnd(), data_page_.hwnd(), message_page_.hwnd(),
                tabs_, label_auto_, combo_auto_, edit_include_, edit_exclude_, btn_save_, btn_apply_};
            check("all unrelated page/toolbar probes installed", probe.controls.size() == 11);
            for (int repeat = 0; repeat < 3; ++repeat) {
                for (int state = 0; state < 4; ++state) {
                    if (state == 0) connection_.begin(settings, false);
                    if (state == 1) connection_.connected(settings.port);
                    if (state == 2) connection_.lost(GetTickCount64());
                    if (state == 3) connection_.cancel();
                    update_connection_ui();
                    const bool idle = state == 3;
                    wchar_t text[32] = {}; ::GetWindowTextW(btn_connect_, text, 32);
                    const auto expected = idle ? L"Connect" : state == 1 ? L"Disconnect" : L"Cancel";
                    check("state caption and enabled controls remain correct", std::wstring(text) == expected &&
                        (::IsWindowEnabled(combo_port_) != FALSE) == idle && (::IsWindowEnabled(btn_scan_) != FALSE) == idle);
                    RECT button{}; ::GetClientRect(btn_connect_, &button);
                    check("each connection caption fits the reserved button", button.right >= control_text_width(btn_connect_, 32));
                    check("state transition keeps all top-level child geometry fixed", equal_bounds(before, child_bounds(m_hWnd)));
                    check("state transition never relayouts unrelated pages/toolbar", probe.events == 0);
                    if (repeat == 0 && state == 0 && width == 1500) {
                        const auto file = L"connection-opening-page-" + std::to_wstring(page) + L".png";
                        check("opening state screenshot", save_runtime_frame(m_hWnd, evidence / file));
                    }
                }
            }
            RECT stop{}, client{}; ::GetWindowRect(combo_stop_, &stop);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<POINT*>(&stop), 2); GetClientRect(&client);
            check("serial row fits the window after reserving all captions", stop.right <= client.right - 8);
            report << "width=" << width << " page=" << page << " unrelated-position-events=" << probe.events << '\n';
        }
    }
    connection_.cancel(); update_connection_ui();
    SetWindowPos(nullptr, 0, 0, 1180, 780, SWP_NOMOVE | SWP_NOZORDER);
    TabCtrl_SetCurSel(tabs_, kChartTab); switch_page();
    report << (pass ? "PASS" : "FAIL") << ": connection-only layout regression; no real COM open, no unrelated suites\n";
    return pass;
}
