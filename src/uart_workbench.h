#pragma once
#include <afxwin.h>
#include <afxdlgs.h>
#include "workbench_session.h"
#include "message_buffer.h"
#include "background_job.h"
#include "binary_frame.h"
#include <array>
#include "uart_command_window.h"
#include "transfer_job.h"
#include "terminal_screen.h"
#include "modbus_rtu.h"
#include "uart_instruments.h"
#include "traffic_framer.h"
#include "text_telemetry.h"
#include "signal_view.h"
#include "serial_forwarder.h"
bool workbench_binary_config(const std::vector<std::wstring>&,BinaryFrameConfig&,std::wstring&);

class UartPortGroup : public CWnd {
protected:
    LRESULT WindowProc(UINT,WPARAM,LPARAM)override;
private:
    HFONT font_=nullptr;
};

class UartWorkbench : public CDialog {
public:
    static constexpr UINT kRequest=WM_APP+24;
    struct Request {enum Action{ConnectA,RecordA,SendA,RefreshA,CheckBinary,ResetNumeric,CheckTx,SendRaw,CancelTx,CheckClock,DiscardText,CheckLive} action=ConnectA;SerialSettings settings;TxEntry tx;std::string bytes;uint64_t generation=0,id=0;bool was_active=false,flag=false;};
    struct Primary {SerialSettings settings;bool active=false,connected=false,stopping=false;uint64_t generation=0;RecordingStatus recording;std::wstring status;};
    bool show(CWnd* owner,const std::wstring& ini,HFONT font,std::shared_ptr<TrafficStore> primary);
    void initialize(const std::wstring&,std::shared_ptr<TrafficStore>);
    void reload_preferences();
    void load_framing();
    void load_views();
    void persist_view(size_t);
    void tick();
    bool binary_enabled(size_t i)const{return binary_[i].enabled;}
    bool any_binary()const{return binary_[0].enabled||binary_[1].enabled;}
    bool secondary_connected()const{return secondary_.connected();}
    bool secondary_active()const{return secondary_.reserved()||secondary_.recorder.status().active||secondary_.recorder.status().stopping;}
    bool tx_reserved(size_t i)const{return runs_[i].kind!=ToolRun::None;}
    TxPacing pacing(size_t i)const{return pacing_[i];}
    bool numeric_suspended(size_t i)const{return runs_[i].kind==ToolRun::Transfer||runs_[i].kind==ToolRun::Modbus;}
    bool requires_receipt_clock()const{return any_binary()||text_b_.config().enabled||runs_[0].kind==ToolRun::Modbus||runs_[1].kind==ToolRun::Modbus;}
    void operation_result(size_t,const SerialOperationResult&);
    void primary_queue(SerialOperationQueue* queue){primary_queue_=queue;}
    void display_sources(UartInstruments::Keys keys,UartInstruments::Series series){instruments_.sources(std::move(keys),std::move(series));}
    void analysis_sources(SignalView::Snapshot snapshot,SignalView::Options options,SignalView::Project project){signals_.sources(std::move(snapshot),std::move(options),std::move(project));}
    bool analysis_busy()const{return signals_.busy();}
    struct Decoded {size_t port=0;BinaryFrame frame;};
    std::vector<Decoded> take_binary();
    std::vector<TextTelemetryFrame> take_text(){return text_b_.take();}
    void suspend_binary(){for(auto& b:binary_)b.enabled=false;}
    void suspend_text(){auto config=text_b_.config();config.enabled=false;std::string error;text_b_.configure(config,error);}
    void update_primary(const Primary&);
    bool secondary_owns(const SerialPortInfo&)const;
    void shutdown();
    bool shutdown_complete()const;
    std::wstring close_recording_error();
    void cancel_shutdown(){shutdown_=false;commands_.resume();signals_.resume();}
    void lock_ui(bool);
    void set_scanner(std::function<std::vector<SerialPortInfo>(bool*)> scanner){scanner_=std::move(scanner);}
protected:
    DECLARE_MESSAGE_MAP()
    BOOL OnInitDialog()override;
    void OnOK()override{}
    void OnCancel()override{ShowWindow(SW_HIDE);}
    BOOL OnCommand(WPARAM,LPARAM)override;
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnDestroy();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    afx_msg LRESULT OnDpiChanged(WPARAM,LPARAM);
    afx_msg HBRUSH OnCtlColor(CDC*,CWnd*,UINT);
    afx_msg BOOL OnEraseBkgnd(CDC*);
    afx_msg void OnLButtonDown(UINT,CPoint);
    afx_msg void OnLButtonUp(UINT,CPoint);
    afx_msg void OnMouseMove(UINT,CPoint);
    afx_msg BOOL OnSetCursor(CWnd*,UINT,UINT);
    afx_msg void OnDrawItem(int,LPDRAWITEMSTRUCT);
private:
    friend class CMainDialog;
    enum Control{Port=1,Scan,Connect,Baud,Data,Parity,Stop,View,Follow,Clear,Save,Record,Input,Hex,Send};
    struct Group {
        UartPortGroup panel;
        HWND box=nullptr,port_label=nullptr,port=nullptr,scan=nullptr,connect=nullptr;
        HWND baud_label=nullptr,baud=nullptr,data_label=nullptr,data=nullptr;
        HWND parity_label=nullptr,parity=nullptr,stop_label=nullptr,stop=nullptr;
        HWND view=nullptr,follow=nullptr,clear=nullptr,save=nullptr,record=nullptr;
        HWND input=nullptr,hex=nullptr,send=nullptr,log=nullptr,status=nullptr;
        MessageBuffer raw;
        TerminalScreen terminal;
        TrafficFramer framer;
        std::deque<TrafficRecord> packets;
        size_t packet_bytes=0;
        uint64_t sequence=0,revision=UINT64_MAX;
        uint64_t terminal_generation=UINT64_MAX;
        int mode=0;
        bool dirty=true;
        std::wstring rendered;
        std::wstring save_error;
        CRect bounds;
    };
    int id(size_t group,int control)const{return 2000+static_cast<int>(group)*100+control;}
    HWND child(const wchar_t* cls,const wchar_t* text,DWORD style,int id=0);
    HWND combo(size_t,int,std::initializer_list<const wchar_t*>,bool editable=false);
    void build_group(size_t);
    void layout();
    int layout_group(size_t,CRect,bool measure=false);
    void scan();
    void fill_ports(size_t,const SerialPortInfo& hint);
    SerialSettings chosen(size_t,std::wstring& error)const;
    void set_settings(size_t,const SerialSettings&);
    void refresh_group(size_t);
    void update_view(size_t);
    void save_group(size_t);
    bool begin_save(size_t,const std::filesystem::path&);
    void record_group(size_t);
    LRESULT request(Request);
    void warn(const std::wstring&);
    void load_b();
    void persist_b();
    std::string export_text(size_t)const;
    void binary_dialog(size_t);
    bool apply_binary(size_t,const std::vector<std::wstring>&,std::wstring& error,bool persist=true);
    void load_binary();
    void decode_binary();
    void show_commands();
    void poll_tools();
    bool acquire(size_t,int,std::wstring&);
    bool submit_tool(size_t,std::string,std::wstring&,bool unpaced=false);
    bool command_send(size_t,std::string,std::wstring&);
    bool start_script(size_t,std::vector<ScriptStep>,std::wstring&);
    void stop_tool(size_t);
    void pacing_dialog(size_t);
    void forward_dialog();
    bool start_forwarding(ForwardOptions,std::wstring&);
    void stop_forwarding(const std::string&);
    void poll_forwarding();
    SerialForwarder forwarder_;
    void transfer_dialog(size_t);
    bool start_transfer(size_t,ProtocolTransferJob::Options,std::wstring&);
    void rtu_dialog(size_t);
    bool start_rtu(size_t,RtuConfig,std::wstring&);
    void poll_rtu(size_t,uint64_t);
    void terminal_dialog(size_t);
    void framing_dialog(size_t);
    void inspect_dialog(size_t);
    void text_dialog();
    bool apply_text(const std::vector<std::wstring>&,std::wstring&,bool persist=true);
    void load_text();
    void decode_text();
    TextTelemetry text_b_;
    std::vector<std::wstring> text_fields_;
    uint64_t text_sequence_=0,text_revision_=UINT64_MAX,text_lost_=0;
    struct ToolRun {enum{None,Command,Script,Transfer,Modbus,Forward}kind=None;uint64_t generation=0,id=0,sequence=0,revision=0;StepScript script;std::wstring status=L"Idle";};
    std::array<ToolRun,2> runs_;
    std::array<TxPacing,2> pacing_;
    std::array<RtuPoller,2> rtu_;
    std::array<RtuConfig,2> rtu_config_;
    UartCommandWindow commands_;
    UartInstruments instruments_;
    SignalView signals_;
    struct BinarySlot {bool enabled=false;std::vector<std::wstring> fields;BinaryFrameDecoder decoder;uint64_t sequence=0,revision=0,lost=0,frames=0,generation=UINT64_MAX;};
    std::array<BinarySlot,2> binary_;
    std::deque<Decoded> decoded_;
    size_t decoded_bytes_=0;
    std::shared_ptr<TrafficStore> primary_;
    WorkbenchSession secondary_;
    std::array<ProtocolTransferJob,2> transfers_;
    SerialOperationQueue* primary_queue_=nullptr;
    Primary primary_status_;
    std::array<Group,2> groups_;
    std::vector<SerialPortInfo> ports_;
    std::function<std::vector<SerialPortInfo>(bool*)> scanner_=SerialManager::scan_ports;
    std::function<int(HWND,LPCWSTR,LPCWSTR,UINT)> notice_=::MessageBoxW;
    std::wstring ini_,saved_b_;
    std::array<BackgroundJob<int>,2> save_jobs_;
    HWND hint_=nullptr;
    HWND building_parent_=nullptr;
    HFONT font_=nullptr,owned_font_=nullptr,log_font_=nullptr;
    HBRUSH black_=nullptr;
    int row_=28,gap_=6,padding_=8;
    double ratio_=0.5;
    int split_y_=0;
    bool ready_=false,shutdown_=false,dragging_=false,had_b_=false;
    bool close_recording_=false;
    bool ui_locked_=false;
    std::vector<std::pair<HWND,bool>> locked_windows_;
};
