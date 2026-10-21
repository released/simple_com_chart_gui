#include "help_dialog.h"

#include <string>

namespace {
const wchar_t* kHelpText =
    L"Text Telemetry / Waveforms\r\n"
    L"=========================\r\n"
    L"\r\n"
    L"Connect to a Windows COM port, then select the channels to draw.\r\n"
    L"Channels are discovered from incoming data; no firmware-specific names are required.\r\n"
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
    L"- Apply starts a new capture and discovers channels again.\r\n"
    L"- Invalid expressions keep the previous applied filter active.\r\n"
    L"\r\n"
    L"Max CH: 1..1024; default 256. Limits are applied on Apply.\r\n"
    L"Use None, then select a few signals with similar units and press Fit.\r\n"
    L"Fit rescales Y; signed values and decimals remain visible.\r\n"
    L"Snapshot freezes the plot; Live resumes. Overlay toggles end values.\r\n"
    L"Refresh clears samples while retaining channels. Changing filters clears both.\r\n"
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
