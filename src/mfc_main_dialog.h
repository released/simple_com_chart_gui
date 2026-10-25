#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <afxwin.h>
#include <afxdlgs.h>
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
#include <functional>
#include <condition_variable>

#include "serial_manager.h"
#include "serial_open_job.h"
#include "log_parser.h"
#include "line_filter.h"
#include "channel_model.h"
#include "plot_view.h"
#include "channel_panel.h"
#include "help_dialog.h"
#include "capture_store.h"
#include "capture_export.h"
#include "data_page.h"
#include "message_page.h"
#include "hover_help.h"
#include "uart_recorder.h"
#include "uart_console.h"
#include "advanced_tools.h"
#include "background_job.h"
#include "uart_offline.h"
#include "replay_binary.h"
#include "derived_channels.h"
#include "uart_profiles.h"
#include "uart_automation.h"
#include "field_extractor.h"
#include "source_mapping.h"
#include "file_transfer.h"
class TrafficStore;
class UartWorkbench;

#include "resource.h"

struct PendingData {
    struct Item {
        std::string line;
        double ts = 0.0;
        uint64_t source_offset=UINT64_MAX;
        uint64_t source_count=0;
    };
    std::vector<Item> lines;
    MessageBuffer raw{};
    uint64_t source_end=UINT64_MAX;
};
void bound_pending_lines(PendingData&,int& dropped);

// The native controls do not use Feature Pack themes/ribbon support.
class CMainDialog : public CDialog {
public:
    explicit CMainDialog(CWnd* pParent = nullptr);
    ~CMainDialog() override;
#ifdef CHART_RUNTIME_CHECKS
    int smoke_exit_code() const { return smoke_exit_code_; }
#endif

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;
    BOOL PreTranslateMessage(MSG* message) override;

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
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
    BOOL OnNotify(WPARAM, LPARAM, LRESULT*) override;
    afx_msg LRESULT OnExportComplete(WPARAM, LPARAM);
    afx_msg LRESULT OnChartSelectionChanged(WPARAM, LPARAM);
    afx_msg LRESULT OnMessageFontChanged(WPARAM, LPARAM);
    afx_msg LRESULT OnUartRequest(WPARAM, LPARAM);
    afx_msg LRESULT OnAdvancedRequest(WPARAM, LPARAM);
    afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
    afx_msg LRESULT OnSourceNavigate(WPARAM, LPARAM);
    afx_msg LRESULT OnLocateSource(WPARAM, LPARAM);
    afx_msg LRESULT OnPlotCursor(WPARAM, LPARAM);
    afx_msg LRESULT OnKeyboardTx(WPARAM, LPARAM);
    afx_msg LRESULT OnKeyboardArm(WPARAM, LPARAM);
    afx_msg LRESULT OnWorkbenchRequest(WPARAM, LPARAM);

private:
    enum { kMessagesTab = 0, kChartTab = 1, kDataTab = 2 };
    enum { IDT_HOTPLUG = 1, IDT_UI = 2, IDT_AUTO = 3, IDT_STATUS = 4 };
    void build_ui();
    void layout_ui(int w, int h);
    void build_filter_ui();
    void build_hover_help(HFONT font);
    void draw_tab(const DRAWITEMSTRUCT*);
    void size_tabs(int height);
    void set_filter_status(const std::wstring& message);
    bool apply_filter(bool save = true);
    void load_settings();
    void save_settings();
#ifdef CHART_RUNTIME_CHECKS
    void feed_demo();
    void run_smoke_test();
    bool verify_layout_after_repaint();
    bool verify_y_axis_autoscale();
    bool verify_data_display();
    bool verify_filter_guidance();
    bool verify_channel_sorting();
    bool verify_messages();
    bool verify_message_export();
    bool verify_message_font();
    bool verify_chart_preferences();
    bool verify_button_theme();
    bool verify_ui_help();
    bool verify_serial_connection();
    bool verify_connection_redraw();
    bool verify_startup_tab();
    bool verify_uart_receive();
    bool verify_uart_operations();
    bool verify_advanced_tools();
    bool verify_optimizations();
    bool verify_receive_soak();
    bool verify_workbench();
    bool verify_analysis_expansion();
    void finish_smoke_test(bool pass);
#endif
    void switch_page();
    void load_startup_tab();
    void apply_startup_tab();
    void update_startup_menu();
    void choose_startup_tab(int page);
    bool save_startup_tab(int page);
    void save_capture_as();
    bool begin_capture_export(const std::filesystem::path&, CaptureFormat);
    bool begin_message_export(const std::filesystem::path&);
    bool begin_export_job(std::function<void()>, std::wstring);

    void start_serial_thread();
    bool stop_serial_thread();
    void poll_serial_stop();
    void poll_uart_operations();
    void show_uart_console();
    void show_workbench();
    void poll_workbench();
    void show_advanced_tools(int page);
    void poll_advanced_tools();
    bool advanced_idle() const;
    void replay_request(const AdvancedTools::Request&);
    void reset_offline_view();
    void feed_replay(size_t first, size_t count);
    void poll_replay_io();
    struct ReplayBatch {size_t first=0,found=0;bool search=false,time_seek=false;std::vector<std::string> bytes;std::vector<BinaryFrame> frames;uint64_t crc_errors=0,discarded=0;};
    void binary_replay_options();
    void load_binary_replay();
    void reset_binary_replay();
    void drain_binary_replay();
    bool replay_binary_enabled_=false;
    BinaryFrameConfig replay_binary_config_;
    std::vector<std::wstring> replay_binary_fields_;
    std::shared_ptr<ReplayBinary> replay_binary_state_;
    std::deque<BinaryFrame> replay_frames_;
    uint64_t replay_crc_errors_=0,replay_discarded_=0;
    void profile_request(const AdvancedTools::Request&);
    UartProfile current_profile() const;
    bool apply_profile(const UartProfile&);
    void lock_profile_ui(bool lock);
    void analysis_request(const AdvancedTools::Request&);
    void automation_request(const AdvancedTools::Request&);
    void stop_automation(const std::wstring& reason);
    void extraction_request(const AdvancedTools::Request&);
    void load_extraction();
    void update_source_status();
    void channel_display_request(const AdvancedTools::Request&);
    void load_display_preferences();
    void file_tx_request(const AdvancedTools::Request&);
    void poll_file_tx();
    void cancel_file_tx(const std::wstring& reason);
    bool file_tx_busy() const {return file_tx_active_ || file_tx_job_.busy() || file_tx_id_;}

    void scan_ports();
    void update_port_combo(const std::vector<SerialPortInfo>& ports);

    void on_connect_toggle();
    bool connect_with_validation();
    bool fail_manual_connection(const std::wstring& port, const std::wstring& reason);
    void disconnect(bool unexpected = false);
    bool begin_serial_open(const SerialSettings&, bool automatic);
    void poll_serial_connection();
    void poll_serial_ports(uint64_t now = GetTickCount64());
    void update_connection_ui();
    void set_serial_status(const std::wstring&);
    void load_serial_settings();
    bool save_serial_settings();
    void start_recording();
    void recording_options();
    void poll_recording();

    void flush_pending_lines();
    void accept_numeric(const std::string&,const std::unordered_map<std::string,double>&,double,uint64_t offset=UINT64_MAX,uint64_t count=0,unsigned source=0);
    std::unordered_map<std::string,unsigned> numeric_sources_;
    uint64_t source_collisions_=0;
    bool numeric_secondary_seen_=false;
    std::array<DerivedChannels,2> derived_;
    std::array<uint64_t,2> derived_generations_{{UINT64_MAX,UINT64_MAX}};
    void derived_options();
    void load_derived();
    double now_seconds() const;
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
    HWND btn_save_ = nullptr, tabs_ = nullptr;
    HWND check_show_name_ = nullptr;
    CaptureStore capture_;
    DataPage data_page_;
    MessagePage message_page_;
    std::thread export_thread_;
    std::atomic<bool> exporting_{false};
    std::mutex export_mutex_;
    std::condition_variable export_wake_;
    std::function<void()> export_job_;
    bool stop_export_ = false;
    std::wstring export_result_;

    HWND status_ = nullptr;
    HWND source_status_ = nullptr;
    FieldExtractor extractor_;
    DisplayPreferences display_preferences_;
    uint64_t parser_skipped_=0, pending_line_loss_=0;
    bool extracted_clock_=false;
    double extracted_origin_=0, extracted_base_=0, extracted_now_=0;
    HWND label_include_ = nullptr, edit_include_ = nullptr;
    HWND label_exclude_ = nullptr, edit_exclude_ = nullptr;
    HWND combo_filter_ = nullptr, edit_limit_ = nullptr;
    HWND label_limit_ = nullptr, btn_apply_ = nullptr, label_filter_ = nullptr;
    HoverHelp hover_help_;
    LineFilter line_filter_;
    std::wstring settings_path_;
    std::wstring active_include_, active_exclude_;
    bool active_regex_ = false;
    bool filter_configured_=false;
    bool ui_ready_ = false;
    int startup_tab_ = kChartTab;
#ifdef CHART_RUNTIME_CHECKS
    bool demo_ = false;
    unsigned demo_tick_ = 0;
    bool smoke_test_ = false;
    int smoke_exit_code_ = 0;
    bool smoke_export_pending_ = false, smoke_checks_passed_ = false;
#endif

    ChannelPanel channel_panel_;
    PlotView plot_view_;
    HelpDialog help_dialog_;

    std::unique_ptr<SerialTransport> serial_mgr_ = std::make_unique<SerialManager>();
    SerialOpenJob::Factory transport_factory_ = [] { return std::make_unique<SerialManager>(); };
    std::function<std::vector<SerialPortInfo>(bool*)> port_scanner_ = SerialManager::scan_ports;
    std::function<int(HWND, LPCWSTR, LPCWSTR, UINT)> connection_notice_ = ::MessageBoxW;
    SerialOpenJob serial_open_;
    SerialConnectionState connection_;
    SerialSettings remembered_serial_;
    SerialPortInfo selection_hint_;
    bool closing_serial_ = false;
    uint64_t retry_log_at_ = 0;
    std::wstring retry_log_error_;
    ChannelModel model_;

    std::vector<SerialPortInfo> known_ports_;

    bool snapshot_ = false;
    bool overlay_enabled_ = true;
    bool is_minimized_ = false;

    std::mutex pending_mutex_;
    PendingData pending_;
    SourceLineTracker source_lines_;
    std::atomic<uint64_t> oversized_lines_{0};
    int pending_dropped_ = 0;
    std::mutex error_mutex_;
    std::wstring serial_error_;
    std::atomic<bool> serial_error_pending_{false};

    std::atomic<bool> serial_running_{false};
    std::thread serial_thread_;
    std::thread serial_operation_thread_;
    std::atomic<bool> serial_done_{true}, serial_operation_done_{true};
    bool disconnect_pending_ = false, disconnect_unexpected_ = false, close_after_serial_ = false;
    SerialOperationQueue uart_operations_;
    UartStatisticsStore uart_statistics_;
    UartConsole uart_console_;
    std::shared_ptr<TrafficStore> primary_traffic_;
    std::atomic<bool> workbench_feed_enabled_{false};
    std::atomic<bool> workbench_numeric_owned_{false};
    std::unique_ptr<UartWorkbench> workbench_;
    bool close_after_workbench_=false;
    SerialControls desired_controls_;
    uint64_t controls_pending_id_ = 0;
    std::function<int(HWND, LPCWSTR, LPCWSTR, UINT)> uart_notice_ = ::MessageBoxW;
    AdvancedTools advanced_tools_;
    BackgroundJob<std::shared_ptr<ReplayData>> replay_load_;
    BackgroundJob<UartProfile> profile_job_;
    ReplayClock replay_;
    BackgroundJob<ReplayBatch> replay_io_;
    uint64_t replay_epoch_=0,replay_io_epoch_=0;
    UartProfile staged_profile_;
    bool has_staged_profile_ = false, close_after_tools_ = false;
    int profile_job_action_ = 0;
    std::vector<std::pair<HWND,bool>> profile_locked_controls_;
    UartAutomation automation_;
    uint64_t automation_id_ = 0, automation_generation_ = 0;
    struct FileTxWork {std::shared_ptr<FileTransfer> transfer;FileTransferChunk chunk;};
    BackgroundJob<FileTxWork> file_tx_job_;
    std::shared_ptr<FileTransfer> file_tx_;
    std::string file_tx_next_;
    bool file_tx_active_=false,file_tx_paused_=false,file_tx_end_=false;
    uint64_t file_tx_id_=0,file_tx_generation_=0,file_tx_at_=0,file_tx_accepted_=0,file_tx_source_=0;

    HFONT btn_font_ = nullptr;
    HFONT ui_font_ = nullptr;
    HWND hover_btn_ = nullptr;
    bool tracking_mouse_ = false;

    std::wstring left_status_;
    std::wstring right_status_;
    std::wstring left_flash_;
    bool flash_active_ = false;

    std::mutex log_mutex_;
    UartRecorder recorder_;
    bool close_after_recording_ = false;
    std::wstring recorder_notice_;
};
