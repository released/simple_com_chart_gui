#include "transfer_job.h"
#include "uart_files.h"
#include "message_format.h"
#include <set>
#include <cctype>
#include <deque>
#include <stdexcept>
#include <algorithm>

namespace {
struct InputFile {HANDLE file=INVALID_HANDLE_VALUE;~InputFile(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);}};
struct StagedFile {
    HANDLE file=INVALID_HANDLE_VALUE;std::filesystem::path temporary,destination;bool replace=false;
    ~StagedFile(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(!temporary.empty())DeleteFileW(temporary.c_str());}
    void begin(std::filesystem::path target,bool overwrite){destination=std::move(target);replace=overwrite;if(!temporary.empty())throw std::runtime_error("Previous receive file is unfinished.");
        static std::atomic<uint64_t> counter{0};for(unsigned n=0;n<32;++n){const auto candidate=destination.parent_path()/(L".scc-transfer-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++counter)+L".tmp");const auto h=CreateFileW(candidate.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);if(h!=INVALID_HANDLE_VALUE){temporary=candidate;file=h;return;}if(GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)break;}throw std::runtime_error("Cannot create receive staging file; check destination/permissions.");}
    void append(std::string_view bytes){DWORD n=0;if(file==INVALID_HANDLE_VALUE||!WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&n,nullptr)||n!=bytes.size())throw std::runtime_error("Receive disk write failed; destination retained.");}
    void commit(){if(file==INVALID_HANDLE_VALUE||!FlushFileBuffers(file))throw std::runtime_error("Receive flush failed; destination retained.");CloseHandle(file);file=INVALID_HANDLE_VALUE;if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_WRITE_THROUGH|(replace?MOVEFILE_REPLACE_EXISTING:0)))throw std::runtime_error("Cannot commit received file (existing batch filename is never overwritten).");temporary.clear();}
};
}
bool ProtocolTransferJob::start(Options options,TrafficStore& traffic,uint64_t generation,Submit submit,Current current,Cancel cancel){
    if(job_.busy())return false;state_=std::make_shared<State>();state_->generation=generation;const auto state=state_;const auto initial=traffic.after(UINT64_MAX,1);
    return job_.start([options=std::move(options),&traffic,generation,submit=std::move(submit),current=std::move(current),cancel=std::move(cancel),state,initial](const std::atomic<bool>& stopped){
        const auto started=GetTickCount64();uint64_t pending=0,sequence=initial.newest;std::deque<unsigned char> input;bool interacted=false;
        const auto check=[&]{if(stopped)throw std::runtime_error("Cancelled; no automatic resume. Completed batch files remain.");if(!current())throw std::runtime_error("Connection changed; transfer stopped without resume.");if(GetTickCount64()-started>=3600000)throw std::runtime_error("Transfer stopped at the one-hour limit.");};
        const auto completion=[state](const SerialOperationResult& result){std::lock_guard<std::mutex> lock(state->mutex);if(state->results.size()<64){auto value=result;value.operation.completion={};state->results[result.operation.id]=std::move(value);}};
        const auto output=[&](std::string_view bytes){check();std::wstring error;if(!submit(std::string(bytes),pending,error,completion))throw std::runtime_error("Protocol TX queue unavailable.");interacted=true;const auto at=GetTickCount64();while(true){check();{std::lock_guard<std::mutex> lock(state->mutex);const auto it=state->results.find(pending);if(it!=state->results.end()){const auto result=it->second;state->results.erase(it);pending=0;if(!result.success||result.cancelled||result.accepted!=bytes.size())throw std::runtime_error("Protocol TX partial/failed/cancelled; no retry of uncertain bytes.");return;}}if(GetTickCount64()-at>60000)throw std::runtime_error("Protocol driver write timed out.");Sleep(2);}};
        const auto byte=[&](unsigned timeout){const auto at=GetTickCount64();while(true){check();if(!input.empty()){interacted=true;const auto c=input.front();input.pop_front();return static_cast<int>(c);}const auto batch=traffic.after(sequence,32);if(batch.revision!=initial.revision||(!batch.records.empty()&&batch.records.front().sequence>sequence+1))throw std::runtime_error("Raw RX history gap; cannot safely continue transfer.");for(const auto& record:batch.records){sequence=record.sequence;if(!record.tx)for(const unsigned char c:record.bytes)input.push_back(c);}if(input.size()>128*1024)throw std::runtime_error("Protocol input backlog exceeded 128 KiB.");if(!input.empty())continue;if(GetTickCount64()-at>=timeout)return -1;Sleep(2);}};
        BlockLink link{byte,output,check,[state](uint64_t bytes,unsigned retry){if(!retry)state->bytes=bytes;else ++state->retries;}};
        try{
            if(options.receive){if(options.paths.size()!=1)throw std::runtime_error("Choose one receive destination.");const auto root=std::filesystem::absolute(options.paths.front()).lexically_normal();StagedFile staged;const bool batch=options.protocol.mode==BlockMode::Ymodem;std::set<std::string> names;
                if(batch&&!std::filesystem::is_directory(root))throw std::runtime_error("YMODEM receive destination must be an existing folder.");
                BlockSink sink{[&](const std::string& name,std::optional<uint64_t>){check();std::filesystem::path target=root;if(batch){if(!safe_transfer_name(name))throw std::runtime_error("Unsafe received filename.");std::string lower=name;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});if(!names.insert(lower).second)throw std::runtime_error("Duplicate batch filename.");target/=uart_wide(name);}staged.begin(target,!batch&&options.replace);},[&](std::string_view bytes){check();staged.append(bytes);},[&]{check();staged.commit();++state->files;}};
                block_receive(options.protocol,sink,link,options.x_size);
            }else {std::vector<BlockSource> sources;for(const auto& path:options.paths){check();auto file=std::make_shared<InputFile>();file->file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);LARGE_INTEGER length{};if(file->file==INVALID_HANDLE_VALUE||!GetFileSizeEx(file->file,&length)||length.QuadPart<0)throw std::runtime_error("Cannot open source file read-only.");sources.push_back({message_utf8(path.filename().wstring()),static_cast<uint64_t>(length.QuadPart),[file](uint64_t at,size_t count){LARGE_INTEGER offset{};offset.QuadPart=at;DWORD n=0;std::string data(count,0);if(!SetFilePointerEx(file->file,offset,nullptr,FILE_BEGIN)||!ReadFile(file->file,data.data(),static_cast<DWORD>(count),&n,nullptr))throw std::runtime_error("Source file read failed.");data.resize(n);return data;}});}block_send(options.protocol,sources,link);state->files=static_cast<unsigned>(sources.size());}
        }catch(...){if(pending)cancel(pending);if(interacted&&current()){uint64_t abort_id=0;std::wstring error;if(submit(std::string(8,char(24)),abort_id,error,completion)){const auto at=GetTickCount64();bool done=false;while(current()&&GetTickCount64()-at<250){{std::lock_guard<std::mutex> lock(state->mutex);const auto it=state->results.find(abort_id);if(it!=state->results.end()){state->results.erase(it);done=true;break;}}Sleep(2);}if(!done)cancel(abort_id);}}throw;}
        return state->files.load();
    });
}
bool ProtocolTransferJob::poll(std::wstring& result){unsigned files=0;std::wstring error;if(!job_.take(files,error,false))return false;result=error.empty()?L"Transfer completed: "+std::to_wstring(files)+L" file(s); protocol ACKs received / output committed.":L"Transfer stopped: "+error+L" | committed files "+std::to_wstring(state_->files.load());return true;}
std::wstring ProtocolTransferJob::progress()const{return !state_?L"Transfer idle":L"Transfer "+std::to_wstring(state_->bytes.load())+L" bytes | retries "+std::to_wstring(state_->retries.load())+L" | files "+std::to_wstring(state_->files.load());}
