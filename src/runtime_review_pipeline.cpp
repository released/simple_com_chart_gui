#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include "runtime_capture.h"
#include "../tests/real_log_cases.h"
#include <array>

namespace {
uint64_t review_ms(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
class ReviewPacer {
public:
    ReviewPacer():timer_(CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE)){}
    ~ReviewPacer(){if(timer_)CloseHandle(timer_);}
    bool precise()const{return timer_!=nullptr;}
    bool wait(unsigned ms){
        if(!timer_){std::this_thread::sleep_for(std::chrono::milliseconds(ms));return true;}
        LARGE_INTEGER due{};due.QuadPart=-static_cast<LONGLONG>(ms)*10000;
        return SetWaitableTimer(timer_,&due,0,nullptr,nullptr,FALSE)&&WaitForSingleObject(timer_,ms+2000)==WAIT_OBJECT_0;
    }
private:HANDLE timer_;
};
class ReviewTransport:public FakeSerialTransport {
public:
    explicit ReviewTransport(std::shared_ptr<FakeSerialState> state):FakeSerialTransport(state),state_(std::move(state)){}
    std::vector<std::string> read_lines(std::wstring*,std::string* raw) override {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if(raw){const auto count=std::min<size_t>(96,state_->raw.size());*raw=state_->raw.substr(0,count);state_->raw.erase(0,count);}
        return {};
    }
private:std::shared_ptr<FakeSerialState> state_;
};
struct Producer {
    std::atomic<bool> stop{false};std::thread worker;
    ~Producer(){stop=true;if(worker.joinable())worker.join();}
};
}
bool CMainDialog::verify_review_pipeline() {
    const auto folder=std::filesystem::path(settings_path_).parent_path();std::ofstream report(folder/L"review-pipeline-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=pass&&ok;};
    const auto scanner=port_scanner_;const auto factory=transport_factory_;auto original=std::move(serial_mgr_);
    auto state=std::make_shared<FakeSerialState>();serial_mgr_=std::make_unique<ReviewTransport>(state);
    const SerialPortInfo port{L"COM196",L"Review fake transport",L"REVIEW_FAKE",L"fake://review"};
    port_scanner_=[port](bool* ok){if(ok)*ok=true;return std::vector<SerialPortInfo>{port};};
    transport_factory_=[state]{return std::make_unique<ReviewTransport>(state);};
    const auto pump=[&](const std::function<bool()>& ready,unsigned timeout=7000){const auto until=GetTickCount64()+timeout;do{
        MSG message{};unsigned count=0;while(count++<32&&PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        poll_serial_connection();poll_uart_operations();flush_pending_lines();if(ready())return true;Sleep(1);
    }while(GetTickCount64()<until);return ready();};
    const auto exact_row=[&](size_t row,const RealLogCase& sample){
        if(row>=capture_.rows().size())return false;const auto& value=*capture_.rows()[row];
        if(value.cells.size()!=sample.expected.size())return false;
        for(const auto& field:sample.expected){const auto found=std::find(capture_.keys().begin(),capture_.keys().end(),field.first);double number=0;
            if(found==capture_.keys().end()||!CaptureStore::value(value,static_cast<size_t>(found-capture_.keys().begin()),number)||number!=field.second)return false;}
        return true;
    };
    try{
        extractor_=FieldExtractor{};::SetWindowTextW(edit_include_,L"");::SetWindowTextW(edit_exclude_,L"");::SetWindowTextW(edit_limit_,L"256");::SendMessageW(combo_filter_,CB_SETCURSEL,0,0);apply_filter(false);
        scan_ports();::SendMessageW(combo_port_,CB_SETCURSEL,0,0);on_connect_toggle();
        if(!pump([&]{return serial_mgr_->is_connected()&&serial_running_;}))throw std::runtime_error("Review fake transport did not connect");
        model_.set_time_window(30);plot_view_.set_time_window(30);
        for(int page:{kChartTab,kDataTab}){
            reset_numeric_capture(true);data_page_.set_selected_fields({"P3","value","MCFG","[formula].S7"});
            for(size_t i=0;i<4;++i)data_page_.set_chart_time(i,30);
            TabCtrl_SetCurSel(tabs_,page);switch_page();
            std::array<std::atomic<uint64_t>,24> sent{};std::atomic<bool> produced{false};std::string raw;
            for(auto& value:sent)value=0;
            for(size_t i=0;i<sent.size();++i)raw+=std::string(real_log_cases()[i%8].input)+'\n';
            ReviewPacer pacer;report<<"producer_high_resolution_timer="<<pacer.precise()<<"; requested_intervals_ms=50,62,75\n";
            Producer producer;producer.worker=std::thread([&]{
                for(size_t i=0;i<sent.size()&&!producer.stop;++i){
                    if(i){const unsigned delays[]{50,62,75};if(!pacer.wait(delays[(i-1)%3]))break;}
                    if(producer.stop)break;std::lock_guard<std::mutex> lock(state->mutex);sent[i]=review_ms();state->raw+=std::string(real_log_cases()[i%8].input)+'\n';
                }produced=true;
            });
            size_t seen=0;std::vector<uint64_t> latency;uint64_t previous=review_ms(),max_gap=0;bool selected=false;
            const bool drained=pump([&]{
                if(!selected&&!capture_.keys().empty()){channel_panel_.set_checkbox_state_map({{"P3",true}});OnChannelChanged(0,0);selected=true;}
                if(seen<capture_.rows().size()){
                    if(page==kChartTab)::UpdateWindow(plot_view_.hwnd());
                    else for(size_t chart=0;chart<4;++chart)::UpdateWindow(data_page_.chart_plot(chart).hwnd());
                }
                const auto now=review_ms();max_gap=std::max(max_gap,now-previous);previous=now;
                while(seen<capture_.rows().size()&&seen<sent.size()){const auto at=sent[seen].load();if(at)latency.push_back(now-at);++seen;}
                return produced&&capture_.rows().size()==sent.size();
            });
            producer.stop=true;producer.worker.join();
            bool exact=drained&&message_page_.buffer().bytes()==raw&&message_page_.buffer().skipped()==0&&pending_line_loss_==0;
            uint64_t offset=0;for(size_t i=0;i<sent.size()&&exact;++i){const auto& sample=real_log_cases()[i%8];exact=exact_row(i,sample)&&capture_.rows()[i]->source_offset==offset&&capture_.rows()[i]->source_count==std::string(sample.input).size()+1;offset+=std::string(sample.input).size()+1;}
            check(page==kChartTab?"visible Chart: eight reviewed formats with requested 50/62/75 ms pacing retain every exact field and RX byte":"visible 4 Charts: eight reviewed formats with requested 50/62/75 ms pacing retain every exact field and RX byte",exact);
            std::sort(latency.begin(),latency.end());const auto percentile=[&](size_t percent){return latency.empty()?0:latency[(latency.size()-1)*percent/100];};
            uint64_t minimum=UINT64_MAX,maximum=0;for(size_t i=1;i<sent.size();++i){const auto delta=sent[i].load()-sent[i-1].load();minimum=std::min(minimum,delta);maximum=std::max(maximum,delta);}
            report<<"page="<<page<<" samples="<<seen<<" producer_interval_min_ms="<<minimum<<" max_ms="<<maximum<<" accepted_and_painted_p50_ms="<<percentile(50)<<" p95_ms="<<percentile(95)<<" p99_ms="<<percentile(99)<<" max_ui_gap_ms="<<max_gap<<" (host paint completed; not physical display latency)\n";
            check("paced visible rendering has no prolonged UI stall",max_gap<1000);
            pump([]{return false;},250);
            ::RedrawWindow(m_hWnd,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
            check("actual selected plot paints latest labels",page==kChartTab?!plot_view_.end_labels().empty():!data_page_.chart_plot(0).end_labels().empty());
            check("paced visible plot screenshot",save_runtime_frame(m_hWnd,folder/(page==kChartTab?L"review-chart.png":L"review-four-charts.png")));
        }
        for(const auto& sample:real_log_cases()){
            ::SetWindowTextW(edit_include_,uart_wide(sample.include).c_str());::SetWindowTextW(edit_exclude_,uart_wide(sample.exclude).c_str());apply_filter(false);
            const auto before=uart_statistics_.snapshot(GetTickCount64()).rx_bytes;const auto retained=message_page_.buffer().bytes().size();const auto bytes=std::string(sample.input)+'\n';{std::lock_guard<std::mutex> lock(state->mutex);state->raw+=bytes;}
            const bool received=pump([&]{return uart_statistics_.snapshot(GetTickCount64()).rx_bytes>=before+bytes.size()&&message_page_.buffer().bytes().size()>=retained+bytes.size();});flush_pending_lines();
            check((std::string("actual raw-line Include/Exclude and exact map: ")+sample.file).c_str(),received&&(sample.accepted?capture_.rows().size()==1&&exact_row(0,sample):capture_.rows().empty()));
        }
    }catch(const std::exception& e){check(e.what(),false);}
    disconnect();check("review fake workers drain",pump([&]{return !disconnect_pending_;}));
    serial_mgr_=std::move(original);port_scanner_=scanner;transport_factory_=factory;connection_.cancel();load_settings();return pass;
}
