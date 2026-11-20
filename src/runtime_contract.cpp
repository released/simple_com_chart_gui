#include "mfc_main_dialog.h"
#include "runtime_serial_fixture.h"
#include "uart_workbench.h"
#include "capture_export.h"

bool CMainDialog::verify_runtime_contract(){
    const auto folder=std::filesystem::path(settings_path_).parent_path();
    std::ofstream report(folder/L"runtime-contract-result.txt");report<<std::unitbuf;bool pass=true;
    const auto check=[&](const char* name,bool ok){report<<(ok?"PASS: ":"FAIL: ")<<name<<'\n';pass=ok&&pass;};
    const auto wait=[&](auto ready){const auto until=GetTickCount64()+5000;while(GetTickCount64()<until){if(ready())return true;Sleep(1);}return false;};
    const auto read=[](const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),{});};
    demo_=false;
    check("snapshot Save As, failure, lost/stale wake and close regression",verify_message_export());
    auto original=std::move(serial_mgr_);auto original_workbench=std::move(workbench_);
    auto state=std::make_shared<FakeSerialState>();serial_mgr_=std::make_unique<FakeSerialTransport>(state);
    serial_mgr_->connect(L"fake://runtime-contract",115200,8,0,0,nullptr);
    ::SetWindowTextW(edit_include_,L"");::SetWindowTextW(edit_exclude_,L"");::SendMessageW(combo_filter_,CB_SETCURSEL,0,0);apply_filter(false);
    extractor_=FieldExtractor{};message_page_.reset();uart_statistics_.reset();
    std::string raw;for(unsigned n=0;n<3000;++n)raw+="v:"+std::to_string(n)+",ignored:"+std::string(128,'x')+"\r\n";
    RecordingOptions recording;recording.base=folder/L"pipeline-raw";recording.rotate_daily=false;
    check("private raw recording starts",recorder_.start(recording));
    {std::lock_guard<std::mutex> lock(state->mutex);state->raw=raw;}
    start_serial_thread();
    check("fake acquisition receives complete oversized UI burst",wait([&]{return uart_statistics_.snapshot(GetTickCount64()).rx_bytes==raw.size();}));
    const bool stopped=wait([&]{return stop_serial_thread();});
    check("fake reader and operation workers stop without UI join",stopped);if(!stopped)return false;
    flush_pending_lines();
    size_t remaining=0;{std::lock_guard<std::mutex> lock(pending_mutex_);remaining=pending_.lines.size();}
    check("Main numeric drain consumes at most64 rows and retains the rest",capture_.rows().size()>0&&capture_.rows().size()<=64&&remaining+capture_.rows().size()==2000);
    check("UI byte/line limits expose loss independently",message_page_.buffer().bytes().size()<=MessageBuffer::kCapacity&&message_page_.buffer().skipped()>0&&pending_line_loss_==1000);
    recorder_.stop();check("recorder drains complete input",recorder_.wait_idle(5000)&&!recorder_.status().failed);
    check("raw recording is byte-exact despite UI eviction and numeric loss",read(folder/L"pipeline-raw_1.bin")==raw&&recorder_.status().bytes==raw.size());
    reset_numeric_capture(true);
    workbench_=std::make_unique<UartWorkbench>();workbench_->initialize((folder/L"pipeline-workbench.ini").wstring(),primary_traffic_);
    workbench_->binary_[0].enabled=true;
    {std::lock_guard<std::mutex> lock(pending_mutex_);pending_dropped_=7;}
    flush_pending_lines();wchar_t status[1024]{};::SendMessageW(status_,SB_GETTEXTW,0,reinterpret_cast<LPARAM>(status));
    check("loss warning remains visible when binary owns numeric input",std::wstring(status).find(L"Input overrun")!=std::wstring::npos&&pending_line_loss_==7);
    workbench_->binary_[0].enabled=false;
    TextTelemetryConfig text;text.enabled=true;std::string error;check("Port B text parser configures",workbench_->text_b_.configure(text,error));
    workbench_->text_revision_=workbench_->secondary_.traffic.after(UINT64_MAX,1).revision;
    std::string lines;for(unsigned n=0;n<100;++n)lines+="rpm:"+std::to_string(n)+'\n';
    workbench_->text_b_.receive({1,1700000000000,false,lines,20,7});
    for(unsigned n=0;n<150;++n){BinaryFrame frame;frame.bytes="x";frame.values={{"n",static_cast<double>(n)}};frame.sample_seconds=20;frame.generation=7;workbench_->decoded_.push_back({n%2,std::move(frame)});++workbench_->decoded_bytes_;}
    const auto before=capture_.rows().size();poll_workbench();const auto first=capture_.rows().size()-before;
    check("Workbench shared budget bounds combined binary/text model work",first>0&&first<=64&&workbench_->decoded_.size()>=118);
    check("bounded Workbench drains retain every pending sample",wait([&]{poll_workbench();return capture_.rows().size()-before==250;}));
    check("alternation services both binary and Port B text",std::find(capture_.keys().begin(),capture_.keys().end(),"B.rpm")!=capture_.keys().end()&&std::find(capture_.keys().begin(),capture_.keys().end(),"A.n")!=capture_.keys().end());
    size_t a=0,b=0,t=0;bool ordered=true;
    for(const auto& row:capture_.rows())if(row->raw.rfind("B text: ",0)==0){ordered&=row->raw=="B text: rpm:"+std::to_string(t++);}else{const auto key=row->numeric_source==0?"A.n":"B.n";const auto column=std::find(capture_.keys().begin(),capture_.keys().end(),key)-capture_.keys().begin();double value=0;const auto expected=row->numeric_source==0?2*a++:2*b++ +1;ordered&=CaptureStore::value(*row,column,value)&&value==expected;}
    check("per-source values and ordering survive successive drains",ordered&&a==75&&b==75&&t==100);
    const auto first_value=[&](double expected){const auto list=::FindWindowExW(channel_panel_.hwnd(),nullptr,WC_LISTVIEWW,nullptr);wchar_t value[64]{};ListView_GetItemText(list,0,1,value,64);wchar_t* end=nullptr;const auto actual=wcstod(value,&end);return ListView_GetItemCount(list)==1&&end!=value&&!*end&&actual==expected;};
    reset_numeric_capture();TextTelemetryConfig disabled;check("single binary fixture disables text input",workbench_->text_b_.configure(disabled,error));
    BinaryFrame single;single.bytes="s";single.values={{"single",42}};single.sample_seconds=now_seconds();single.generation=8;
    workbench_->decoded_.push_back({0,std::move(single)});++workbench_->decoded_bytes_;poll_workbench();
    check("first A binary sample populates a new channel value immediately",first_value(42));
    reset_numeric_capture();check("single text fixture configures",workbench_->text_b_.configure(text,error));
    workbench_->text_b_.receive({2,1700000001000,false,"single:43\n",now_seconds(),8});poll_workbench();
    check("first B text sample populates a new channel value immediately",first_value(43));
    reset_numeric_capture();TextTelemetryConfig excluded=text;excluded.exclude="drop";
    std::string filtered;for(unsigned n=0;n<64;++n)filtered+="drop:1\n";
    bool observed=false,complete=false;
    for(unsigned attempt=0;attempt<5&&!observed;++attempt){
        check("single-source budget fixture configures",workbench_->text_b_.configure(excluded,error));
        workbench_->text_b_.receive({3,1700000001000,false,filtered,now_seconds(),8});
        const auto began=GetTickCount64();poll_workbench();
        if(GetTickCount64()-began<8){observed=true;complete=!workbench_->text_pending();}
    }
    check("empty binary source does not halve the64-row text budget",observed&&complete&&capture_.rows().empty());
    check("checkpoint fixture text parser configures",workbench_->text_b_.configure(text,error));
    workbench_->text_b_.receive({4,1700000002000,false,"stale:2\n",now_seconds(),9});
    BinaryFrame stale;stale.bytes="old";stale.values={{"stale",1}};stale.sample_seconds=now_seconds();stale.generation=9;
    workbench_->decoded_.push_back({0,std::move(stale)});workbench_->decoded_bytes_+=3;
    BinaryFrameConfig schema;schema.header="H";schema.fixed_bytes=2;schema.fields={{"part","u8",1,1,0}};
    check("checkpoint binary parser configures",workbench_->binary_[0].decoder.configure(schema,error));
    workbench_->binary_[0].decoder.feed("H",1700000002000,now_seconds());
    check("checkpoint contains old complete frames and binary partial",workbench_->binary_pending()&&workbench_->text_pending()&&workbench_->binary_[0].decoder.pending_bytes()>0);
    primary_traffic_->append("retained-A",1700000002000);workbench_->secondary_.traffic.append("retained-B",1700000002000);
    const auto before_a=primary_traffic_->snapshot(),before_b=workbench_->secondary_.traffic.snapshot();uint64_t cut=0;
    {std::lock_guard<std::mutex> lock(pending_mutex_);pending_.lines.push_back({"stale:3",now_seconds()});source_lines_.feed("stale:",now_seconds());cut=source_lines_.end();}
    reset_numeric_capture();poll_workbench();flush_pending_lines();
    check("new capture retires old Main/binary/B-text pending results",capture_.rows().empty()&&!workbench_->binary_pending()&&!workbench_->text_pending()&&workbench_->binary_[0].decoder.pending_bytes()==0);
    const auto after_a=primary_traffic_->snapshot(),after_b=workbench_->secondary_.traffic.snapshot();
    check("numeric checkpoint retains both original traffic histories",before_a.revision==after_a.revision&&before_a.rx==after_a.rx&&before_a.records.size()==after_a.records.size()&&before_b.revision==after_b.revision&&before_b.rx==after_b.rx&&before_b.records.size()==after_b.records.size());
    {std::lock_guard<std::mutex> lock(pending_mutex_);const auto fresh=source_lines_.feed("9\n",now_seconds());check("new capture excludes pre-checkpoint text partial with exact new offset",fresh.size()==1&&fresh[0].text=="9"&&fresh[0].offset==cut&&fresh[0].count==2);}
    workbench_.reset();workbench_=std::move(original_workbench);serial_mgr_->disconnect();serial_mgr_=std::move(original);
    reset_numeric_capture();message_page_.reset();
    report<<(pass?"PASS":"FAIL")<<": runtime ownership, bounded drains, exact recording and snapshot export\n";
    return pass;
}
