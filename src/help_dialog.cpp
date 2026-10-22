#include "help_dialog.h"

#include <string>

namespace {
const wchar_t* kHelpText =
    L"Text Telemetry / Waveforms\r\n"
    L"=========================\r\n"
    L"\r\n"
    L"Connect to a Windows COM port, then select the channels to draw.\r\n"
    L"Channels are discovered from incoming data; no firmware-specific names are required.\r\n"
    L"Tabs: Messages / Chart / 4 Charts. Choose Settings > Startup tab (default Chart).\r\n"
    L"The chosen startup tab saves immediately to INI and applies next launch; tab clicks do not change it.\r\n"
    L"The selected tab is dark grey with white text. Messages is green on black.\r\n"
    L"Last successful COM/settings are remembered; startup restores selection, not a live connection.\r\n"
    L"After bridge removal/read failure, reconnect the same identified bridge automatically.\r\n"
    L"COM renumbering is supported; retry delay grows from 2 to at most 10 seconds.\r\n"
    L"Cancel/Disconnect stops recovery. Unknown/ambiguous devices require manual connection.\r\n"
    L"Automatic recovery preserves charts, captured rows and Messages. No command replay.\r\n"
    L"\r\n"
    L"Input: one text line per sample, ending in LF or CRLF.\r\n"
    L"Fields: comma separated key:value or key=value.\r\n"
    L"Example:\r\n"
    L"fan,duty:920,fg_hz:69.1,offset:-2.5,err:11%,cmd:0x3202,cap:105/76/29\r\n"
    L"\r\n"
    L"Numeric values: signed integers, decimals, scientific notation, 0x hex.\r\n"
    L"ASCII unit suffixes (mv, Hz) are accepted; units are not rescaled.\r\n"
    L"11% means 11. cap:105/76/29 creates cap[0], cap[1], cap[2].\r\n"
    L"Text-only tokens such as fan and malformed values are skipped.\r\n"
    L"Keys may contain spaces or symbols; max 128 UTF-8 bytes.\r\n"
    L"\r\n"
    L"Include / Exclude:\r\n"
    L"- Text: case-sensitive substring search over the entire raw line.\r\n"
    L"- Regex: search using a common ECMAScript-style expression.\r\n"
    L"- Include must match (empty = all); Exclude takes priority (empty = none).\r\n"
    L"- Example Include ^(fan,|ac_adc_mv:) selects both line types in Regex mode.\r\n"
    L"- Multiple strings: select Regex and use | for OR, e.g. fan,|ac_adc_mv:.\r\n"
    L"- Example Exclude error|timeout discards either match, even if Include matches.\r\n"
    L"- Text matches one literal string. Comma/semicolon are not list separators.\r\n"
    L"- Filtering keeps/discards whole lines, not individual chart channels.\r\n"
    L"- Apply starts a new capture and discovers channels again.\r\n"
    L"- Invalid expressions keep the previous applied filter active.\r\n"
    L"\r\n"
    L"Max CH: 1..1024; default 256. Limits are applied on Apply.\r\n"
    L"Use None, then select a few signals with similar units. Y scales automatically.\r\n"
    L"Click Channel to toggle natural A-Z / Z-A order (P2 before P10); each tab sorts independently.\r\n"
    L"Y follows enabled signals in the time window, expanding AND shrinking.\r\n"
    L"0..50 is only the empty-data fallback; Fit recalculates the same range.\r\n"
    L"Snapshot freezes plot and axes; Live resumes. Overlay toggles end values.\r\n"
    L"Toolbar: grey when idle, red only while pressed/executing.\r\n"
    L"Connect stays red while connected and returns to grey after disconnect.\r\n"
    L"Hover over Refresh/Fit/Snapshot/Overlay for explanations.\r\n"
    L"Auto Refresh periodically clears Chart samples, NOT the receive/update interval.\r\n"
    L"Off stops timed clearing; captured table/log and Messages remain intact.\r\n"
    L"Include/Exclude explanations appear when hovering over their labels/fields.\r\n"
    L"Refresh clears samples while retaining channels. Changing filters clears both.\r\n"
    L"\r\n"
    L"4 Charts: use the left checkboxes / All / None to choose table columns.\r\n"
    L"Only the first four fields are initially shown; newer fields stay unchecked.\r\n"
    L"Hidden fields are still captured and saved. Newest rows append at bottom.\r\n"
    L"The header stays visible. Uncheck Auto-scroll to inspect older rows.\r\n"
    L"Each of the four plots has its own field, time window and automatic Y scale.\r\n"
    L"Plots start at (none). Chosen field names are remembered beside the EXE.\r\n"
    L"Saved names wait for incoming data; Apply/Connect retain these choices.\r\n"
    L"Reset to none clears all four plots and their saved choices, not the table/log.\r\n"
    L"Table headers/cells are left-aligned; widths follow titles/recent values.\r\n"
    L"All four show a marker and live value at the latest sample, even for one point.\r\n"
    L"Show name (default on) switches both tabs between name:value and value only.\r\n"
    L"Long names may be shortened to fit; numeric values are retained.\r\n"
    L"Chart-page checkboxes, Refresh and Snapshot do not change this tab.\r\n"
    L"Save As: .log (default) or .txt = raw filtered text; .csv/.cvs or binary .xls = table.\r\n"
    L"Suggested filename uses local PC time: yyyyMMdd_HHmmss.log; you can edit it.\r\n"
    L"Capture retains up to 10000 lines / 32 MiB payload; discarded count is visible.\r\n"
    L"Save exports retained lines at click time; later incoming lines are not included.\r\n"
    L"XLS supports at most 254 telemetry columns; choose CSV/log for wider captures.\r\n"
    L"Manual Connect or valid Apply starts a new capture; automatic recovery keeps history.\r\n"
    L"\r\n"
    L"Messages: unfiltered raw RX, even before a newline, default ASCII or exact HEX.\r\n"
    L"ASCII escapes nonprintable/non-ASCII bytes as \\xNN; HEX retains CR/LF/NUL.\r\n"
    L"Switch modes without clearing history. Clear messages affects only this tab.\r\n"
    L"Auto-scroll defaults on; turn it off to inspect older messages.\r\n"
    L"Messages Font -/+ changes only message text (8-32 pt, one point per click).\r\n"
    L"Size is saved immediately in the portable INI and restored next launch.\r\n"
    L"History/pending buffers: 256 KiB each; Dropped/Input lost count evictions.\r\n"
    L"Apply/automatic recovery keep raw messages; successful manual Connect resets them.\r\n"
    L"Messages Save As: current ASCII/HEX text to .log (default) or .txt.\r\n"
    L"Uses PC date/time filename; saves retained history, not continuous/binary recording.\r\n"
    L"Chart-only controls remain inactive on Messages.\r\n"
    L"\r\n"
    L"Native MFC settings are saved in SimpleComChart.ini beside the EXE.\r\n"
    L"Timestamps use PC receipt time, not device timestamps.\r\n"
    L"Launch with --demo to view simulated telemetry without a serial device.\r\n"
    L"USB-UART hardware may require its vendor driver.\r\n";
}

void HelpDialog::show(HWND parent) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = HelpDialog::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"HelpDialogWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        wc.lpszClassName,
        L"How to Use / MCU Log Format",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 800,
        parent,
        nullptr,
        wc.hInstance,
        this
    );
    if (!hwnd_) {
        return;
    }

    ShowWindow(hwnd_, SW_SHOW);

    MSG msg;
    while (IsWindow(hwnd_) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (IsDialogMessageW(hwnd_, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK HelpDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HelpDialog* self = reinterpret_cast<HelpDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<HelpDialog*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        return self->handle_message(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT HelpDialog::handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        edit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
                                WS_VSCROLL | ES_AUTOVSCROLL, 10, 10, 980, 700, hwnd, nullptr, nullptr, nullptr);
        font_ = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        if (font_) {
            SendMessageW(edit_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        }
        SetWindowTextW(edit_, kHelpText);

        btn_close_ = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   900, 720, 80, 26, hwnd, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (edit_ && btn_close_) {
            int margin = 10;
            int btn_h = 26;
            int btn_w = 80;
            MoveWindow(edit_, margin, margin, w - 2 * margin, h - 3 * margin - btn_h, TRUE);
            MoveWindow(btn_close_, w - btn_w - margin, h - btn_h - margin, btn_w, btn_h, TRUE);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (reinterpret_cast<HWND>(lParam) == btn_close_) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (font_) {
            DeleteObject(font_);
            font_ = nullptr;
        }
        hwnd_ = nullptr;
        edit_ = nullptr;
        btn_close_ = nullptr;
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
