#include "recording_overwrite.h"
#include "message_format.h"
#include <windows.h>
#include <atomic>
#include <stdexcept>
#include <cwchar>
#include <map>

namespace {
std::runtime_error file_error(const char* action,const std::filesystem::path& path,DWORD error) {
    return std::runtime_error(std::string(action)+": "+message_utf8(path.wstring())+", Windows error "+std::to_string(error));
}
RecordingOverwrite identity(HANDLE file,const std::filesystem::path& path) {
    BY_HANDLE_FILE_INFORMATION info{};
    if(!GetFileInformationByHandle(file,&info))throw file_error("Cannot inspect recording file",path,GetLastError());
    if(GetFileType(file)!=FILE_TYPE_DISK||(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))throw std::runtime_error("Recording overwrite requires a regular file: "+message_utf8(path.wstring()));
    return {std::filesystem::absolute(path).lexically_normal(),info.dwVolumeSerialNumber,
        (uint64_t(info.nFileIndexHigh)<<32)|info.nFileIndexLow,(uint64_t(info.nFileSizeHigh)<<32)|info.nFileSizeLow,
        (uint64_t(info.ftLastWriteTime.dwHighDateTime)<<32)|info.ftLastWriteTime.dwLowDateTime};
}
bool same_file(const RecordingOverwrite& a,const RecordingOverwrite& b,bool contents=true) {
    return a.volume==b.volume&&a.file_id==b.file_id&&(!contents||(a.size==b.size&&a.modified==b.modified));
}
struct PathLess {bool operator()(const std::wstring& a,const std::wstring& b)const{return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_LESS_THAN;}};
void add_matches(std::vector<RecordingOverwrite>& files,size_t& units,const std::filesystem::path& base,const wchar_t* extension,bool numbered,const std::filesystem::path& exclude={}) {
    const auto pattern=base.wstring()+(numbered?L"_*":L"")+extension;
    WIN32_FIND_DATAW data{};const auto find=FindFirstFileW(pattern.c_str(),&data);
    if(find==INVALID_HANDLE_VALUE){const auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND)return;throw file_error("Cannot inspect recording files",base,error);}
    try{do{
        const std::wstring leaf=data.cFileName;
        const auto expected=base.filename().wstring();const auto ext_size=wcslen(extension);
        if(numbered){const auto prefix=expected+L"_";const auto first=prefix.size();
            if(leaf.size()<=first+ext_size||CompareStringOrdinal(leaf.c_str(),static_cast<int>(first),prefix.c_str(),static_cast<int>(first),TRUE)!=CSTR_EQUAL||CompareStringOrdinal(leaf.c_str()+leaf.size()-ext_size,static_cast<int>(ext_size),extension,static_cast<int>(ext_size),TRUE)!=CSTR_EQUAL)continue;
            const auto end=leaf.size()-ext_size;bool digits=leaf[first]!=L'0';for(auto n=first;n<end;++n)digits=digits&&leaf[n]>=L'0'&&leaf[n]<=L'9';if(!digits)continue;
        }else if(CompareStringOrdinal(leaf.c_str(),-1,(expected+extension).c_str(),-1,TRUE)!=CSTR_EQUAL)continue;
        const auto path=base.parent_path()/leaf;
        if(!exclude.empty()&&recording_same_path(path,exclude))continue;
        if(files.size()>=16384||path.native().size()>2*1024*1024-units)throw std::runtime_error("Too many existing recording segments.");
        units+=path.native().size();files.push_back(recording_file_identity(path));
    }while(FindNextFileW(find,&data));const auto error=GetLastError();if(error!=ERROR_NO_MORE_FILES)throw file_error("Recording enumeration failed",base,error);}
    catch(...){FindClose(find);throw;}FindClose(find);
}
void validate_approvals(const RecordingOptions& options) {
    const auto current=existing_recording_files(options);
    if(current.size()!=options.overwrite.size())throw std::runtime_error("Recording files changed after overwrite confirmation; review and start again.");
    std::map<std::wstring,const RecordingOverwrite*,PathLess> approved;
    for(const auto& file:options.overwrite)if(!approved.emplace(std::filesystem::absolute(file.path).lexically_normal().native(),&file).second)throw std::runtime_error("Duplicate recording overwrite approval.");
    for(const auto& file:current){const auto found=approved.find(file.path.native());
        if(found==approved.end()||!same_file(file,*found->second))throw std::runtime_error("Recording file changed after overwrite confirmation: "+message_utf8(file.path.wstring()));}
}
}
bool recording_same_path(const std::filesystem::path& a,const std::filesystem::path& b) {
    return CompareStringOrdinal(std::filesystem::absolute(a).lexically_normal().c_str(),-1,std::filesystem::absolute(b).lexically_normal().c_str(),-1,TRUE)==CSTR_EQUAL;
}
RecordingOverwrite recording_file_identity(const std::filesystem::path& path) {
    const auto file=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(file==INVALID_HANDLE_VALUE)throw file_error("Cannot inspect recording file",path,GetLastError());
    try{const auto result=identity(file,path);CloseHandle(file);return result;}catch(...){CloseHandle(file);throw;}
}
std::vector<RecordingOverwrite> existing_recording_files(const RecordingOptions& options) {
    std::vector<RecordingOverwrite> files;size_t units=0;add_matches(files,units,options.base,L".sccmeta",false);
    add_matches(files,units,options.base,L".bin",true);add_matches(files,units,options.base,L".idx",true);
    if(!options.first_readable.empty())add_matches(files,units,options.first_readable,L"",false);
    add_matches(files,units,options.readable_base.empty()?options.base:options.readable_base,L".log",true,options.first_readable);return files;
}
std::filesystem::path recording_log_path(const RecordingOptions& options,uint64_t segment) {
    if(segment==1&&!options.first_readable.empty())return options.first_readable;
    return (options.readable_base.empty()?options.base:options.readable_base).wstring()+L"_"+std::to_wstring(segment)+L".log";
}
struct RecordingReplacement::State {
    struct Old {RecordingOverwrite file;HANDLE guard=INVALID_HANDLE_VALUE;std::filesystem::path backup;bool moved=false;};
    struct Output {std::filesystem::path destination,stage;RecordingOverwrite file;bool published=false;};
    std::vector<Old> old;
    std::vector<Output> outputs;
    std::vector<std::filesystem::path> directories;
    RecordingOptions options;
    bool committed=false,rolled_back=false;
    ~State(){unlock();}
    void unlock(){for(auto& file:old)if(file.guard!=INVALID_HANDLE_VALUE){CloseHandle(file.guard);file.guard=INVALID_HANDLE_VALUE;}}
    std::filesystem::path directory(const std::filesystem::path& parent) {
        for(const auto& path:directories)if(recording_same_path(path.parent_path(),parent))return path;
        static std::atomic<unsigned> sequence{0};
        for(unsigned n=0;n<64;++n){const auto path=parent/(L".scc-recording-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++sequence));
            if(CreateDirectoryW(path.c_str(),nullptr)){directories.push_back(path);return path;}
            const auto error=GetLastError();if(error!=ERROR_ALREADY_EXISTS)throw file_error("Cannot stage recording",path,error);
        }
        throw std::runtime_error("Cannot reserve a fresh recording staging folder.");
    }
    Old* previous(const std::filesystem::path& path){for(auto& file:old)if(recording_same_path(file.file.path,path))return &file;return nullptr;}
    void validate(const Old& file){if(!same_file(file.file,recording_file_identity(file.file.path)))throw std::runtime_error("Recording file changed after overwrite confirmation: "+message_utf8(file.file.path.wstring()));}
    void remove_directories(){for(const auto& path:directories)if(!RemoveDirectoryW(path.c_str())){const auto error=GetLastError();if(error!=ERROR_PATH_NOT_FOUND&&error!=ERROR_FILE_NOT_FOUND)throw file_error("Recording recovery folder retained",path,error);}}
};
RecordingReplacement::RecordingReplacement(const RecordingOptions& options):state_(std::make_unique<State>()) {
    const auto& approved=options.overwrite;state_->options=options;validate_approvals(options);
    if(approved.size()>16384)throw std::runtime_error("Too many recording overwrite approvals.");
    for(const auto& file:approved){
        const auto guard=CreateFileW(file.path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
        if(guard==INVALID_HANDLE_VALUE)throw file_error("Cannot lock confirmed recording file",file.path,GetLastError());
        try{if(!same_file(file,identity(guard,file.path)))throw std::runtime_error("Recording file changed after overwrite confirmation: "+message_utf8(file.path.wstring()));state_->old.push_back({file,guard,{ },false});}
        catch(...){CloseHandle(guard);throw;}
    }
}
RecordingReplacement::~RecordingReplacement(){if(!state_->committed&&!state_->rolled_back)rollback();}
std::filesystem::path RecordingReplacement::output(const std::filesystem::path& destination) {
    for(const auto& file:state_->outputs)if(recording_same_path(file.destination,destination))return file.stage;
    const auto path=state_->directory(destination.parent_path())/(L"new-"+std::to_wstring(state_->outputs.size()));
    state_->outputs.push_back({destination,path,{},false});return path;
}
void RecordingReplacement::created(const std::filesystem::path& destination) {
    for(auto& file:state_->outputs)if(recording_same_path(file.destination,destination)){file.file=recording_file_identity(file.stage);return;}
    throw std::runtime_error("Unknown recording stage.");
}
void RecordingReplacement::commit() {
    validate_approvals(state_->options);
    for(auto& file:state_->outputs){
        auto* old=state_->previous(file.destination);
        if(old){state_->validate(*old);old->backup=state_->directory(file.destination.parent_path())/(L"old-"+std::to_wstring(old-state_->old.data()));
            if(!ReplaceFileW(file.destination.c_str(),file.stage.c_str(),old->backup.c_str(),0,nullptr,nullptr)){
                const auto error=GetLastError();
                // ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 may already have moved the old file.
                if(GetFileAttributesW(old->backup.c_str())!=INVALID_FILE_ATTRIBUTES&&same_file(old->file,recording_file_identity(old->backup),false))old->moved=true;
                throw file_error("Cannot replace confirmed recording file",file.destination,error);
            }old->moved=true;
        }else if(!MoveFileExW(file.stage.c_str(),file.destination.c_str(),MOVEFILE_WRITE_THROUGH))throw file_error("Cannot publish new recording file",file.destination,GetLastError());
        file.published=true;
    }
    // Retire old tail segments too, so replay cannot mix a new prefix with old data.
    for(auto& file:state_->old)if(!file.moved){state_->validate(file);file.backup=state_->directory(file.file.path.parent_path())/(L"old-"+std::to_wstring(&file-state_->old.data()));
        if(!MoveFileExW(file.file.path.c_str(),file.backup.c_str(),MOVEFILE_WRITE_THROUGH))throw file_error("Cannot replace old recording segment",file.file.path,GetLastError());file.moved=true;
    }
}
void RecordingReplacement::validate_output(const std::filesystem::path& destination) const {
    for(const auto& file:state_->outputs)if(file.published&&recording_same_path(file.destination,destination)&&same_file(file.file,recording_file_identity(destination),false))return;
    throw std::runtime_error("Published recording file changed before opening: "+message_utf8(destination.wstring()));
}
void RecordingReplacement::finish() {
    state_->committed=true;state_->unlock();
    for(auto& file:state_->old)if(file.moved){if(!DeleteFileW(file.backup.c_str()))throw file_error("New recording committed; old backup retained",file.backup,GetLastError());file.moved=false;}
    state_->remove_directories();
}
std::string RecordingReplacement::rollback() {
    if(state_->committed||state_->rolled_back)return {};
    state_->rolled_back=true;std::string failures;
    const auto attempt=[&](auto action){try{action();}catch(const std::exception& e){failures+="\n";failures+=e.what();}};
    for(auto it=state_->outputs.rbegin();it!=state_->outputs.rend();++it)if(it->published)attempt([&]{
        if(!same_file(it->file,recording_file_identity(it->destination),false))throw std::runtime_error("Changed output retained; recovery required: "+message_utf8(it->destination.wstring()));
        auto* old=state_->previous(it->destination);
        if(old){if(!MoveFileExW(old->backup.c_str(),it->destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw file_error("Old recording backup retained",old->backup,GetLastError());old->moved=false;}
        else if(!DeleteFileW(it->destination.c_str()))throw file_error("New recording output retained",it->destination,GetLastError());
        it->published=false;
    });
    for(auto& file:state_->old)if(file.moved)attempt([&]{if(!MoveFileExW(file.backup.c_str(),file.file.path.c_str(),MOVEFILE_WRITE_THROUGH))throw file_error("Old recording backup retained",file.backup,GetLastError());file.moved=false;});
    state_->unlock();
    for(auto& file:state_->outputs)if(!file.published&&file.file.file_id)attempt([&]{
        const auto attributes=GetFileAttributesW(file.stage.c_str());if(attributes==INVALID_FILE_ATTRIBUTES){const auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND)return;throw file_error("Recording stage retained",file.stage,error);}
        if(!same_file(file.file,recording_file_identity(file.stage),false))throw std::runtime_error("Changed recording stage retained: "+message_utf8(file.stage.wstring()));
        if(!DeleteFileW(file.stage.c_str()))throw file_error("Recording stage retained",file.stage,GetLastError());
    });
    attempt([&]{state_->remove_directories();});return failures;
}
