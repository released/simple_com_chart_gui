#include "recording_library.h"
#include "uart_files.h"
#include "message_format.h"
#include "session_manifest.h"
#include <sstream>
#include <algorithm>
#include <set>
#include <chrono>
namespace {
void validate(const RecordingReference& r){if((r.resource.kind!=ResourceKind::Recording&&r.resource.kind!=ResourceKind::Session)||r.resource.path.empty()||r.resource.path.size()>32760||r.resource.name.size()>64||r.tags.size()>128||r.resource.related.size()>32760)throw std::runtime_error("Invalid recording reference or tag length.");for(const auto* text:{&r.resource.path,&r.resource.related,&r.resource.name,&r.tags})if(text->find(L'\0')!=text->npos||uart_wide(message_utf8(*text))!=*text)throw std::runtime_error("Invalid Unicode or NUL in recording reference.");}
std::string encoded(const std::wstring& s){return uart_hex(message_utf8(s));}
std::wstring decoded(const std::string& s){std::string value;if(!uart_unhex(s,value))throw std::runtime_error("Invalid library text encoding.");return uart_wide(value);}
}
std::string encode_recording_library(const std::vector<RecordingReference>& entries){
    if(entries.size()>1000)throw std::runtime_error("Recording library is limited to 1000 references.");std::string out="SCCLIBRARY1\n";std::set<std::wstring> paths;
    for(const auto& r:entries){validate(r);auto path=r.resource.path;std::transform(path.begin(),path.end(),path.begin(),towlower);if(!paths.insert(path).second)throw std::runtime_error("Duplicate recording reference.");out+=std::to_string(static_cast<unsigned>(r.resource.kind))+"|"+encoded(r.resource.name)+"|"+encoded(r.resource.path)+"|"+encoded(r.resource.related)+"|"+encoded(r.tags)+"\n";if(out.size()>1024*1024)throw std::runtime_error("Recording library exceeds 1 MiB.");}return out;
}
std::vector<RecordingReference> decode_recording_library(std::string_view bytes){
    if(bytes.size()>1024*1024||bytes.substr(0,12)!="SCCLIBRARY1\n")throw std::runtime_error("Invalid recording library header/size.");
    std::istringstream input(std::string(bytes.substr(12)));std::string line;std::vector<RecordingReference> result;
    while(std::getline(input,line)){std::vector<std::string> fields;size_t p=0;for(;;){const auto end=line.find('|',p);fields.push_back(line.substr(p,end-p));if(end==line.npos)break;p=end+1;}if(fields.size()!=5||(fields[0]!="0"&&fields[0]!="3"))throw std::runtime_error("Invalid recording library row.");RecordingReference r;r.resource.kind=fields[0]=="0"?ResourceKind::Recording:ResourceKind::Session;r.resource.name=decoded(fields[1]);r.resource.path=decoded(fields[2]);r.resource.related=decoded(fields[3]);r.tags=decoded(fields[4]);validate(r);result.push_back(std::move(r));if(result.size()>1000)throw std::runtime_error("Recording library exceeds 1000 references.");}
    encode_recording_library(result);return result;
}
RecordingLibrary read_recording_library(const std::filesystem::path& path,const std::vector<RecentResource>& recent,const std::atomic<bool>& cancel){
    RecordingLibrary out;out.existed=std::filesystem::exists(path);if(out.existed){out.original=uart_read_file(path,1024*1024,cancel);out.entries=decode_recording_library(out.original);}
    for(const auto& r:recent)if(r.kind==ResourceKind::Recording||r.kind==ResourceKind::Session){const auto found=std::find_if(out.entries.begin(),out.entries.end(),[&](const auto& e){return _wcsicmp(e.resource.path.c_str(),r.path.c_str())==0;});if(found==out.entries.end()&&out.entries.size()<1000){RecordingReference entry;entry.resource=r;out.entries.push_back(std::move(entry));}}
    for(auto& r:out.entries){if(cancel)throw std::runtime_error("Cancelled");try{
        const std::filesystem::path source(r.resource.path);if(!std::filesystem::is_regular_file(source)){r.status=L"Missing file";continue;}r.bytes=std::filesystem::file_size(source);
        const auto stamp=std::filesystem::last_write_time(source);const auto system=std::chrono::time_point_cast<std::chrono::milliseconds>(stamp-std::filesystem::file_time_type::clock::now()+std::chrono::system_clock::now());
        const auto t=std::chrono::system_clock::to_time_t(system);tm local{};localtime_s(&local,&t);wchar_t date[40]{};wcsftime(date,40,L"%Y-%m-%d %H:%M:%S",&local);r.modified=date;r.status=L"File present; hashes not verified";
        auto meta=source;
        if(r.resource.kind==ResourceKind::Session){const auto m=decode_session_manifest(uart_read_file(source,1024*1024,cancel));r.version=uart_wide(m.metadata.version);r.source=uart_wide(m.metadata.source);}
        else {auto stem=meta.stem().wstring();const auto split=stem.find_last_of(L'_');if(split!=stem.npos&&split+1<stem.size()&&stem.find_first_not_of(L"0123456789",split+1)==stem.npos)stem.resize(split);meta=meta.parent_path()/(stem+L".sccmeta");if(std::filesystem::is_regular_file(meta)){const auto m=decode_capture_metadata(uart_read_file(meta,1024*1024,cancel));r.version=uart_wide(m.version);r.source=uart_wide(m.source);}}
    }catch(const std::exception& e){r.status=uart_wide(e.what());}}return out;
}
void save_recording_reference(const std::filesystem::path& path,const RecordingLibrary& snapshot,RecordingReference entry,const std::atomic<bool>& cancel){
    validate(entry);if(snapshot.existed!=std::filesystem::exists(path)||(snapshot.existed&&uart_read_file(path,1024*1024,cancel)!=snapshot.original))throw std::runtime_error("Library changed externally; reopen it before saving.");
    auto rows=snapshot.existed?decode_recording_library(snapshot.original):std::vector<RecordingReference>{};const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& r){return _wcsicmp(r.resource.path.c_str(),entry.resource.path.c_str())==0;});if(found==rows.end())rows.push_back(std::move(entry));else *found=std::move(entry);
    uart_write_file(path,encode_recording_library(rows),snapshot.existed,cancel);
}
