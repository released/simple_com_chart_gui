#pragma once
#include <windows.h>

inline constexpr wchar_t kButtonBusyProperty[] = L"SimpleComChart.ExecutingButton";
inline constexpr COLORREF kButtonActiveRed = RGB(180, 48, 48);

struct ButtonColors { COLORREF background, text; };
inline ButtonColors button_colors(bool pressed, bool connected, bool disabled, bool hot) {
    if (disabled) return {GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_GRAYTEXT)};
    if (pressed || connected) return {kButtonActiveRed, RGB(255, 255, 255)};
    return {GetSysColor(hot ? COLOR_3DLIGHT : COLOR_BTNFACE), GetSysColor(COLOR_BTNTEXT)};
}

// Mark only this native control's lifetime; never touch an owner after dispatch.
class ButtonFeedback {
public:
    explicit ButtonFeedback(HWND control) : control_(control) {
        if (!IsWindow(control_)) return;
        previous_ = GetPropW(control_, kButtonBusyProperty);
        if (SetPropW(control_, kButtonBusyProperty, reinterpret_cast<HANDLE>(this)))
            RedrawWindow(control_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    ~ButtonFeedback() {
        // A destroyed/reused HWND cannot retain this per-window identity token.
        if (!IsWindow(control_) || GetPropW(control_, kButtonBusyProperty) != reinterpret_cast<HANDLE>(this)) return;
        if (previous_) SetPropW(control_, kButtonBusyProperty, previous_);
        else RemovePropW(control_, kButtonBusyProperty);
        RedrawWindow(control_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    ButtonFeedback(const ButtonFeedback&) = delete;
    ButtonFeedback& operator=(const ButtonFeedback&) = delete;
private:
    HWND control_ = nullptr;
    HANDLE previous_ = nullptr;
};
