#include "uart_recorder.h"
#include "message_format.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <cstring>

namespace {
constexpr std::string_view index_header="SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n";
class RecordingFile {
public:
    ~RecordingFile() { close(); }
    void open(const std::filesystem::path& path) {
        close(); path_ = path;
        handle_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create " + message_utf8(path.wstring()) + " (existing name, permissions or disk error), Windows error " + std::to_string(GetLastError()));
    }
    void write(std::string_view bytes) {
        if (bytes.empty()) return;
        DWORD written = 0;
        const bool ok = WriteFile(handle_, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) != FALSE;
        written_ += written;
        if (!ok || written != bytes.size())
            throw std::runtime_error("Recording write failed: " + message_utf8(path_.wstring()) + ", Windows error " + std::to_string(ok ? ERROR_WRITE_FAULT : GetLastError()));
    }
    void flush() {
        if (handle_ != INVALID_HANDLE_VALUE && !FlushFileBuffers(handle_))
            throw std::runtime_error("Recording flush failed: " + message_utf8(path_.wstring()) + ", Windows error " + std::to_string(GetLastError()));
    }
    void close() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); handle_ = INVALID_HANDLE_VALUE; }
    uint64_t written() const { return written_; }
    void reset_count() { written_ = 0; }
private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    std::filesystem::path path_;
    uint64_t written_ = 0;
};
std::wstring exception_text(const std::exception& ex) {
    const int length = static_cast<int>(strlen(ex.what()));
    const int n = MultiByteToWideChar(CP_UTF8, 0, ex.what(), length, nullptr, 0);
    std::wstring value(n, 0); MultiByteToWideChar(CP_UTF8, 0, ex.what(), length, value.data(), n);
    return value;
}
std::wstring failure_text(const std::exception& ex) { return L"Recording ERROR: " + exception_text(ex); }
int64_t day_at(int64_t ms) {
    ULARGE_INTEGER ticks{}; ticks.QuadPart = static_cast<uint64_t>(ms + 11644473600000LL) * 10000;
    FILETIME ft{ticks.LowPart, ticks.HighPart}, local{}; SYSTEMTIME st{};
    FileTimeToLocalFileTime(&ft, &local); FileTimeToSystemTime(&local, &st);
    return st.wYear * 10000 + st.wMonth * 100 + st.wDay;
}
struct RecordingLimit : std::runtime_error { using std::runtime_error::runtime_error; };
}

UartRecorder::UartRecorder() : worker_([this] { run(); }) {}
UartRecorder::~UartRecorder() {
    { std::lock_guard<std::mutex> lock(mutex_); shutdown_ = true; }
    wake_.notify_one(); if (worker_.joinable()) worker_.join();
}
bool UartRecorder::start(RecordingOptions options) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_.active || shutdown_ || options.base.empty()) return false;
    try { encode_recording_limits(options.limits); } catch (...) { return false; }
    if(options.metadata.size()>1100*1024)return false;
    options.rotate_bytes = std::clamp<uint64_t>(options.rotate_bytes, 1024, 1024ULL * 1024 * 1024);
    status_ = {}; status_.active = true; status_.detail = L"Starting recording...";
    queue_.push_back({Kind::Start, std::move(options), {}, 0, false}); wake_.notify_one(); return true;
}
void UartRecorder::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping) return;
    status_.stopping = true; queue_.push_back({Kind::Stop, {}, {}, 0, false}); wake_.notify_one();
}
void UartRecorder::append(std::string_view bytes, int64_t ms, bool tx) {
    if (bytes.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping) return;
    if (queue_.size() >= 4096 || bytes.size() > 8 * 1024 * 1024 - queued_bytes_) {
        // Stop visibly at the first gap. Never continue an apparently complete raw recording.
        status_.lost += bytes.size(); status_.stopping = true;
        queue_.push_back({Kind::Event, {}, "RECORDER INPUT GAP: " + std::to_string(bytes.size()) + " bytes not recorded; recording stopped", ms, false});
        queue_.push_back({Kind::Stop, {}, {}, 0, false}); wake_.notify_one(); return;
    }
    queue_.push_back({Kind::Data, {}, std::string(bytes), ms, tx}); queued_bytes_ += bytes.size(); wake_.notify_one();
}
void UartRecorder::event(std::string text, int64_t ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!status_.active || status_.stopping || queue_.size() > 4096) return;
    if (text.size() > 1024) text.resize(1024);
    queue_.push_back({Kind::Event, {}, std::move(text), ms, false}); wake_.notify_one();
}
RecordingStatus UartRecorder::status() const { std::lock_guard<std::mutex> lock(mutex_); return status_; }
bool UartRecorder::wait_idle(unsigned ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    return idle_.wait_for(lock, std::chrono::milliseconds(ms), [this] { return !status_.active; });
}

void UartRecorder::run() {
    RecordingFile raw, text, index, metadata;
    RecordingOptions options; bool opened=false, armed=false, first=true, after_cr=false, changed=false;
    uint64_t segment=0,segment_bytes=0,started=0,checked=0,free_at_check=0,bytes_at_check=0;
    int64_t day=0;
    auto last_flush=std::chrono::steady_clock::now();
    const auto total=[&]{return raw.written()+text.written()+index.written()+metadata.written();};
    const auto close_segment=[&]{raw.close();text.close();index.close();opened=false;};
    const auto close=[&]{close_segment();metadata.close();};
    const auto flush=[&]{if(opened){raw.flush();text.flush();index.flush();metadata.flush();}last_flush=std::chrono::steady_clock::now();};
    const auto reserve=[&](uint64_t next) {
        if(!armed)return;
        const auto now=GetTickCount64();
        if(options.limits.free_bytes&&(!checked||now-checked>=250)) {
            ULARGE_INTEGER available{};
            auto directory=options.base.parent_path();if(directory.empty())directory=L".";
            if(!GetDiskFreeSpaceExW(directory.c_str(),&available,nullptr,nullptr))throw std::runtime_error("Cannot query available recording disk space.");
            free_at_check=available.QuadPart;bytes_at_check=total();checked=now;
        }
        const auto consumed=total()-bytes_at_check;
        const auto free=free_at_check>consumed?free_at_check-consumed:0;
        const auto reason=recording_limit_reason(options.limits,total(),next,now-started,free);
        if(!reason.empty())throw RecordingLimit(reason);
    };
    const auto open_segment=[&](int64_t ms) {
        reserve(index_header.size());if(opened)flush();close_segment();++segment;segment_bytes=0;day=day_at(ms);first=true;after_cr=false;
        const auto base=options.base.wstring()+L"_"+std::to_wstring(segment);
        raw.open(base+L".bin");text.open(base+L".log");index.open(base+L".idx");opened=true;index.write(index_header);
        std::lock_guard<std::mutex> lock(mutex_);status_.segments=segment;status_.detail=L"Recording: "+base;
    };
    const auto fail=[&](const std::exception& ex,bool limit) {
        std::wstring detail=limit?L"Recording LIMIT: "+exception_text(ex):failure_text(ex);
        if(limit)try{flush();}catch(const std::exception& error){detail=failure_text(error);}
        close();armed=false;std::lock_guard<std::mutex> lock(mutex_);
        status_.active=status_.stopping=false;status_.limit_reached=limit;status_.output_bytes=total();status_.detail=std::move(detail);
        queue_.clear();queued_bytes_=0;idle_.notify_all();
    };
    for(;;) {
        Work work;bool timed=false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            timed=!wake_.wait_for(lock,std::chrono::milliseconds(250),[this]{return shutdown_||!queue_.empty();});
            if(queue_.empty()&&shutdown_){lock.unlock();try{flush();}catch(...){}break;}
            if(!timed){work=std::move(queue_.front());queue_.pop_front();if(work.kind==Kind::Data)queued_bytes_-=work.bytes.size();}
        }
        try {
            if(timed){reserve(0);flush();continue;}
            if(work.kind==Kind::Start) {
                options=std::move(work.options);raw.reset_count();text.reset_count();index.reset_count();metadata.reset_count();segment=0;changed=false;
                started=GetTickCount64();checked=0;bytes_at_check=0;free_at_check=0;armed=true;
                if(!options.metadata.empty()){reserve(options.metadata.size()+index_header.size());metadata.open(options.base.wstring()+L".sccmeta");metadata.write(options.metadata);}
                open_segment(message_unix_ms());
            } else if(work.kind==Kind::Stop) {
                flush();close();armed=false;std::lock_guard<std::mutex> lock(mutex_);status_.active=status_.stopping=false;
                status_.detail=status_.lost?L"Recording STOPPED: input gap; see .idx":L"Recording saved (stopped)";idle_.notify_all();
            } else if(opened&&work.kind==Kind::Event) {
                for(auto& c:work.bytes)if(c=='\r'||c=='\n'||c==',')c=' ';
                const auto line=std::to_string(segment_bytes)+",0,"+std::to_string(work.unix_ms)+","+work.bytes+"\r\n";
                const bool mark=!changed&&!options.metadata.empty()&&work.bytes.rfind("NUMERIC CONFIG CHANGED",0)==0;
                reserve(line.size()+(mark?11:0));index.write(line);if(mark){metadata.write("Changed=1\r\n");changed=true;}
            } else if(opened&&work.kind==Kind::Data) {
                if(!work.tx&&!segment_bytes)day=day_at(work.unix_ms);
                if(!work.tx&&segment_bytes&&((options.rotate_daily&&day_at(work.unix_ms)!=day)||segment_bytes+work.bytes.size()>options.rotate_bytes))open_segment(work.unix_ms);
                std::string index_line,output;bool next_first=first,next_cr=after_cr;
                MessageDisplayOptions display;display.clock=MessageClock::Local;
                if(work.tx){output="\r\n[TX] "+message_utf8(format_message(work.bytes,0,{{0,work.unix_ms}},display).text)+"\r\n";next_first=true;next_cr=false;}
                else {
                    index_line=std::to_string(segment_bytes)+","+std::to_string(work.bytes.size())+","+std::to_string(work.unix_ms)+",RX\r\n";
                    for(char c:work.bytes){if(c=='\n'&&next_cr){next_cr=false;continue;}next_cr=c=='\r';
                        if(next_first){output+=message_utf8(format_message(" ",0,{{0,work.unix_ms}},display).text);output.pop_back();output+="[RX] ";next_first=false;}
                        if(c=='\r'||c=='\n'){output+="\r\n";next_first=true;}
                        else if(c=='\t'||(static_cast<unsigned char>(c)>=32&&static_cast<unsigned char>(c)<127))output+=c;
                        else {constexpr char hex[]="0123456789ABCDEF";const auto b=static_cast<unsigned char>(c);output+="\\x";output+=hex[b>>4];output+=hex[b&15];}}
                }
                reserve((work.tx?0:work.bytes.size())+index_line.size()+output.size());
                if(!work.tx){raw.write(work.bytes);index.write(index_line);segment_bytes+=work.bytes.size();}
                text.write(output);first=next_first;after_cr=next_cr;
                {std::lock_guard<std::mutex> lock(mutex_);if(work.tx)status_.tx_bytes+=work.bytes.size();else status_.bytes+=work.bytes.size();}
                if(std::chrono::steady_clock::now()-last_flush>=std::chrono::milliseconds(250))flush();
            }
            {std::lock_guard<std::mutex> lock(mutex_);status_.output_bytes=total();}
        } catch(const RecordingLimit& ex){fail(ex,true);}
          catch(const std::exception& ex){fail(ex,false);}
    }
}
