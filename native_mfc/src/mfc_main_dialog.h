#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxdialogex.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <windowsx.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <fstream>

#include "serial_manager.h"
#include "log_parser.h"
#include "channel_model.h"
#include "plot_view.h"
#include "channel_panel.h"
#include "help_dialog.h"

#include "resource.h"

struct PendingData {
    struct Item {
        std::string line;
        double ts = 0.0;
    };
    std::vector<Item> lines;
};

class CMainDialog : public CDialogEx {
public:
    explicit CMainDialog(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    afx_msg BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg LRESULT OnChannelChanged(WPARAM wParam, LPARAM lParam);
    afx_msg void OnHelpLogFormat();
    afx_msg void OnSnapshotClicked();
    afx_msg void OnOverlayClicked();

private:
    void build_ui();
    void layout_ui(int w, int h);

    void start_serial_thread();
    void stop_serial_thread();

    void scan_ports();
    void update_port_combo(const std::vector<SerialPortInfo>& ports);

    void on_connect_toggle();
    bool connect_with_validation();
    void disconnect();

    void flush_pending_lines();
    void sync_channels();

    void set_left_status(const std::wstring& text);
    void set_right_status(const std::wstring& text);
    void show_status_message(const std::wstring& text, int ms);

    void draw_owner_button(const DRAWITEMSTRUCT* dis);
    void update_hover_button(POINT pt, bool leave);

    void log_line(const std::wstring& msg);

    HWND combo_port_ = nullptr;
    HWND btn_scan_ = nullptr;
    HWND btn_connect_ = nullptr;
    HWND combo_baud_ = nullptr;
    HWND combo_data_ = nullptr;
    HWND combo_parity_ = nullptr;
    HWND combo_stop_ = nullptr;
    HWND label_com_ = nullptr;
    HWND label_baud_ = nullptr;
    HWND label_data_ = nullptr;
    HWND label_parity_ = nullptr;
    HWND label_stop_ = nullptr;

    HWND btn_refresh_ = nullptr;
    HWND btn_fit_ = nullptr;
    HWND btn_snapshot_ = nullptr;
    HWND btn_overlay_ = nullptr;
    HWND combo_auto_ = nullptr;
    HWND combo_time_ = nullptr;
    HWND label_auto_ = nullptr;
    HWND label_time_ = nullptr;

    HWND status_ = nullptr;

    ChannelPanel channel_panel_;
    PlotView plot_view_;
    HelpDialog help_dialog_;

    SerialManager serial_mgr_;
    ChannelModel model_;

    std::vector<SerialPortInfo> known_ports_;

    bool snapshot_ = false;
    bool overlay_enabled_ = true;
    bool is_minimized_ = false;

    std::mutex pending_mutex_;
    PendingData pending_;
    int pending_dropped_ = 0;
    std::mutex error_mutex_;
    std::wstring serial_error_;
    std::atomic<bool> serial_error_pending_{false};

    std::atomic<bool> serial_running_{false};
    std::thread serial_thread_;

    HFONT btn_font_ = nullptr;
    HWND hover_btn_ = nullptr;
    bool tracking_mouse_ = false;

    std::wstring left_status_;
    std::wstring right_status_;
    std::wstring left_flash_;
    bool flash_active_ = false;

    std::mutex log_mutex_;
};
