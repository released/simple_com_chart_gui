#pragma once
#include <afxwin.h>
#include "uart_preferences.h"
#include <vector>

class UartConsole : public CDialog {
public:
    static constexpr UINT kRequest = WM_APP + 7;
    struct Request {
        enum class Action { Send, Controls, Break, ResetStatistics } action;
        TxEntry tx;
        SerialControls controls;
    };
    bool show(CWnd* owner, const std::wstring& ini, HFONT font);
    void set_controls(const SerialControls& value);
    void update(bool connected, bool configurable, bool applying, const UartStatistics&, size_t queued, uint64_t now);
    void notice(const std::wstring& text);
    void remember(const TxEntry& entry);
    void result(const SerialOperationResult& result);
    bool verify(const std::wstring& folder);
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog() override;
    void OnOK() override {}
    void OnCancel() override { ShowWindow(SW_HIDE); }
    BOOL OnCommand(WPARAM, LPARAM) override;
    BOOL PreTranslateMessage(MSG*) override;
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
    afx_msg void OnDestroy();
private:
    enum { kInput = 100, kMode, kEnding, kSend, kFlow, kDtr, kRts, kApply, kBreak, kReset,
        kPreset, kSavePreset, kLoadPreset, kClearPreset, kClearView, kClearHistory };
    void layout();
    void update_handshake();
    TxEntry input() const;
    void set_input(const TxEntry& value);
    void rebuild_history();
    void request(Request value);
    HWND label(const wchar_t* text);
    HWND button(const wchar_t* text, int id, bool checkbox = false);
    HWND combo(int id, std::initializer_list<const wchar_t*> names, bool editable = false);
    HWND flow_label_ = nullptr, flow_ = nullptr, dtr_ = nullptr, rts_ = nullptr, apply_ = nullptr, break_ = nullptr, reset_ = nullptr;
    HWND statistics_ = nullptr, mode_label_ = nullptr, mode_ = nullptr, ending_label_ = nullptr, ending_ = nullptr, rx_hint_ = nullptr;
    HWND input_ = nullptr, send_ = nullptr, preset_label_ = nullptr, preset_ = nullptr, save_preset_ = nullptr, load_preset_ = nullptr;
    HWND clear_preset_ = nullptr, clear_view_ = nullptr, clear_history_ = nullptr, transcript_ = nullptr;
    HFONT font_ = nullptr;
    HFONT owned_font_ = nullptr;
    std::wstring ini_;
    bool ready_ = false, configurable_ = true;
    std::vector<TxEntry> history_;
    std::array<TxEntry, 8> presets_;
};
