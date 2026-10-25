#include "replay_storage.h"
#include <windows.h>
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <cstring>

struct ReplayFile::Impl {HANDLE file=INVALID_HANDLE_VALUE;uint64_t bytes=0;mutable std::mutex mutex;~Impl(){if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);}};
ReplayFile::ReplayFile(const std::filesystem::path& path):impl_(std::make_unique<Impl>()) {
    impl_->file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};
    if(impl_->file==INVALID_HANDLE_VALUE || GetFileType(impl_->file)!=FILE_TYPE_DISK || !GetFileInformationByHandle(impl_->file,&info) || (info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) || !GetFileSizeEx(impl_->file,&size) || size.QuadPart<0) throw std::runtime_error("Cannot open a completed regular file (busy, unavailable or invalid).");
    impl_->bytes=static_cast<uint64_t>(size.QuadPart);
}
ReplayFile::~ReplayFile()=default;
uint64_t ReplayFile::size() const{return impl_->bytes;}
std::string ReplayFile::read(uint64_t offset,size_t count,const std::atomic<bool>& cancel) const {
    if(cancel) throw std::runtime_error("Cancelled");if(count>1024*1024 || offset>impl_->bytes || count>impl_->bytes-offset) throw std::runtime_error("Invalid bounded file read.");
    std::lock_guard<std::mutex> lock(impl_->mutex);LARGE_INTEGER at{};at.QuadPart=static_cast<LONGLONG>(offset);
    if(!SetFilePointerEx(impl_->file,at,nullptr,FILE_BEGIN)) throw std::runtime_error("File seek failed.");
    std::string result(count,0);DWORD got=0;if(count && (!ReadFile(impl_->file,result.data(),static_cast<DWORD>(count),&got,nullptr)||got!=count)) throw std::runtime_error("File changed, became unavailable or read failed.");
    if(cancel) throw std::runtime_error("Cancelled");return result;
}
struct ReplayRecords::Impl {
    std::vector<ReplayRecord> buffer;size_t count=0;HANDLE file=INVALID_HANDLE_VALUE,mapping=nullptr;
    mutable void* view=nullptr;mutable uint64_t view_offset=0;mutable size_t view_bytes=0;mutable std::mutex mutex;bool finished=false;
    ~Impl(){if(view) UnmapViewOfFile(view);if(mapping) CloseHandle(mapping);if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);}
    void flush(){if(buffer.empty()) return;DWORD wrote=0;const auto bytes=buffer.size()*sizeof(ReplayRecord);if(!WriteFile(file,buffer.data(),static_cast<DWORD>(bytes),&wrote,nullptr)||wrote!=bytes) throw std::runtime_error("Replay temporary index write failed (disk full?).");buffer.clear();}
};
ReplayRecords::ReplayRecords():impl_(std::make_unique<Impl>()){impl_->buffer.reserve(4096);}
ReplayRecords::~ReplayRecords()=default;
void ReplayRecords::push_back(ReplayRecord value) {
    auto& p=*impl_;if(p.finished || p.count>=50000000) throw std::runtime_error("Replay index limit is 50 million records.");
    if(p.buffer.size()==4096) {
        if(p.file==INVALID_HANDLE_VALUE) {
            wchar_t folder[MAX_PATH]{};if(!GetTempPathW(MAX_PATH,folder)) throw std::runtime_error("Cannot find replay index temporary folder.");
            static std::atomic<unsigned> sequence{0};
            for(int attempt=0;attempt<16 && p.file==INVALID_HANDLE_VALUE;++attempt) {
                const auto name=std::filesystem::path(folder)/(L"SCC-Replay-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++sequence)+L".tmp");
                p.file=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
                if(p.file==INVALID_HANDLE_VALUE && GetLastError()!=ERROR_FILE_EXISTS) break;
            }
            if(p.file==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create private replay index.");
        }
        p.flush();
    }
    p.buffer.push_back(value);++p.count;
}
void ReplayRecords::finish(){auto& p=*impl_;if(p.finished) return;if(p.file!=INVALID_HANDLE_VALUE){p.flush();p.mapping=CreateFileMappingW(p.file,nullptr,PAGE_READONLY,0,0,nullptr);if(!p.mapping) throw std::runtime_error("Cannot map paged replay index.");}p.finished=true;}
size_t ReplayRecords::size() const{return impl_->count;}
bool ReplayRecords::paged() const{return impl_->file!=INVALID_HANDLE_VALUE;}
ReplayRecord ReplayRecords::operator[](size_t index) const {
    auto& p=*impl_;if(index>=p.count) throw std::out_of_range("Replay record");if(!p.mapping) return p.buffer.at(index);
    std::lock_guard<std::mutex> lock(p.mutex);const uint64_t offset=index*sizeof(ReplayRecord),total=p.count*sizeof(ReplayRecord);
    if(!p.view || offset<p.view_offset || offset+sizeof(ReplayRecord)>p.view_offset+p.view_bytes) {
        if(p.view){UnmapViewOfFile(p.view);p.view=nullptr;}SYSTEM_INFO system{};GetSystemInfo(&system);p.view_offset=offset/system.dwAllocationGranularity*system.dwAllocationGranularity;p.view_bytes=static_cast<size_t>(std::min<uint64_t>(256*1024,total-p.view_offset));
        p.view=MapViewOfFile(p.mapping,FILE_MAP_READ,static_cast<DWORD>(p.view_offset>>32),static_cast<DWORD>(p.view_offset),p.view_bytes);if(!p.view) throw std::runtime_error("Replay index page could not be read.");
    }
    ReplayRecord result;memcpy(&result,static_cast<const char*>(p.view)+(offset-p.view_offset),sizeof(result));return result;
}
