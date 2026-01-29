#include "help_dialog.h"

#include <string>

namespace {
const wchar_t* kHelpText =
    L"Supported Log Format\r\n"
    L"====================\r\n\r\n"
    L"Each UART log must be exactly ONE line and must end with CRLF (\\r\\n).\r\n\r\n"
    L"General Format:\r\n"
    L"  key:value,key:value,key:value,...\\r\\n\r\n"
    L"Example (single line):\r\n"
    L"  state:5,CHG:4179mv,T1:2296mv,T2:1589mv,Q6:2111mv,Q2/Q3:21mv\\r\\n\r\n"
    L"Rules:\r\n"
    L"- Fields are separated by comma ','\r\n"
    L"- Key and value are separated by colon ':'\r\n"
    L"- Spaces are ignored\r\n"
    L"- Field order does not matter\r\n"
    L"- Unknown keys are ignored\r\n"
    L"- One log line represents one sample\r\n"
    L"- Line termination must be CRLF (\\r\\n)\r\n\r\n"
    L"MCU Firmware Example (C):\r\n"
    L"  printf(\"state:%d,CHG:%dmv,T1:%dmv,T2:%dmv,Q6:%dmv,Q2/Q3:%dmv\\r\\n\",\r\n"
    L"         state, chg_mv, t1_mv, t2_mv, q6_mv, q23_mv);\r\n\r\n"
    L"Notes:\r\n"
    L"- Timestamp is generated on the PC side when data is received\r\n"
    L"- This tool does not control MCU output timing or content\r\n"
    L"- Any change in log format on MCU side must be reflected in the parser\r\n";
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
