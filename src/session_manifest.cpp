#include "session_manifest.h"
#include "command_panel.h"
#include "message_format.h"
#include "uart_offline.h"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <cwctype>
#include <set>
#include <sstream>
#include <stdexcept>
namespace {
constexpr uint64_t maximum_bytes=256ULL*1024*1024*1024;
void cancelled(const std::atomic<bool>& c){if(c)throw std::runtime_error("Session file work cancelled.");}
void plain(const std::filesystem::path& path){std::filesystem::path part;for(const auto& p:std::filesystem::absolute(path).lexically_normal()){part/=p;const auto a=GetFileAttributesW(part.c_str());if(a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Session paths cannot traverse links/junctions.");}}
bool same(const std::wstring& a,const std::wstring& b){return CompareStringOrdinal(a.c_str(),static_cast<int>(a.size()),b.c_str(),static_cast<int>(b.size()),TRUE)==CSTR_EQUAL;}
std::wstring leaf(std::string_view hex){std::string bytes;if(!uart_unhex(hex,bytes))throw std::runtime_error("Invalid session filename encoding.");auto name=uart_wide(bytes);if(name.empty()||name.size()>255||name.back()==L'.'||name.back()==L' '||name.find_first_of(L"/\\:<>\"|?*")!=name.npos||std::any_of(name.begin(),name.end(),[](wchar_t c){return c<32;}))throw std::runtime_error("Session references must be plain relative filenames.");
    const std::filesystem::path p(name);auto extension=p.extension().wstring();for(auto& c:extension)c=static_cast<wchar_t>(std::towlower(c));if(extension!=L".bin"&&extension!=L".idx"&&extension!=L".log"&&extension!=L".sccmeta")throw std::runtime_error("Unsupported session file role.");
    auto stem=p.stem().wstring();for(auto& c:stem)c=static_cast<wchar_t>(std::towupper(c));if(stem==L"CON"||stem==L"PRN"||stem==L"AUX"||stem==L"NUL"||(stem.size()==4&&(stem.substr(0,3)==L"COM"||stem.substr(0,3)==L"LPT")&&stem[3]>=L'0'&&stem[3]<=L'9'))throw std::runtime_error("Reserved session filename.");return name;
}
uint64_t number(const std::string& s,uint64_t limit){uint64_t n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(s.empty()||r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||n>limit)throw std::runtime_error("Invalid session integer.");return n;}
void add(std::string& out,const std::vector<std::string>& values){for(size_t i=0;i<values.size();++i){if(i)out+=',';out+=uart_csv_cell(values[i]);}out+="\r\n";}
std::filesystem::path metadata_path(const std::filesystem::path& source){auto stem=source.stem().wstring();const auto at=stem.find_last_of(L'_');if(at!=stem.npos&&at+1<stem.size()&&std::all_of(stem.begin()+at+1,stem.end(),[](wchar_t c){return c>=L'0'&&c<=L'9';}))stem.resize(at);return source.parent_path()/(stem+L".sccmeta");}
void archive_matches(const ReplayData& replay,const SessionManifest& s){std::vector<std::wstring> expected,actual;for(const auto& f:s.files)if(same(std::filesystem::path(f.leaf).extension().wstring(),L".bin"))expected.push_back(f.leaf);for(const auto& p:replay.paths)actual.push_back(p.filename().wstring());
    if(expected.size()!=actual.size())throw std::runtime_error("Recording segment set changed since the manifest.");for(const auto& a:actual)if(std::none_of(expected.begin(),expected.end(),[&](const auto& b){return same(a,b);}))throw std::runtime_error("Recording segment set does not match the manifest.");}
}
std::string encode_session_manifest(const SessionManifest& s){
    if(s.files.empty()||s.files.size()>4096||s.interval_ms<1||s.interval_ms>60000)throw std::runtime_error("Session limit: 1..4096 files / interval 1..60000 ms.");
    std::string out="SCC_SESSION_V1\r\n";add(out,{"source",uart_hex(message_utf8(s.source)),std::to_string(s.interval_ms)});
    auto meta=encode_capture_metadata(s.metadata.profile,s.metadata.version,s.metadata.source);if(s.metadata.changed)meta+="Changed=1\r\n";
    for(size_t at=0,n=0;at<meta.size();at+=8192,++n)add(out,{"metadata",std::to_string(n),uart_hex(std::string_view(meta).substr(at,8192))});
    for(const auto& e:s.files)add(out,{"file",uart_hex(message_utf8(e.leaf)),std::to_string(e.bytes),e.sha256});
    if(out.size()>4*1024*1024)throw std::runtime_error("Session manifest exceeds 4 MiB.");return out;
}
SessionManifest decode_session_manifest(std::string_view text){
    constexpr std::string_view header="SCC_SESSION_V1\r\n";if(text.substr(0,header.size())!=header)throw std::runtime_error("Unsupported session manifest.");std::vector<std::vector<std::string>> rows;std::string error;
    if(!uart_csv_table(text.substr(header.size()),rows,error,4*1024*1024,8192,4))throw std::runtime_error(error);
    SessionManifest s;bool source=false,files_started=false;std::string meta;size_t chunk=0;uint64_t total=0;
    for(const auto& r:rows){if(r.size()==3&&r[0]=="source"&&!source&&!files_started&&chunk==0){s.source=leaf(r[1]);s.interval_ms=static_cast<unsigned>(number(r[2],60000));if(!s.interval_ms)throw std::runtime_error("Invalid session interval.");source=true;}
        else if(r.size()==3&&r[0]=="metadata"&&source&&!files_started){if(number(r[1],256)!=chunk++)throw std::runtime_error("Metadata chunks are not contiguous.");std::string bytes;if(!uart_unhex(r[2],bytes)||bytes.empty()||bytes.size()>8192||meta.size()+bytes.size()>1100*1024)throw std::runtime_error("Invalid/bounded session metadata.");meta+=bytes;}
        else if(r.size()==4&&r[0]=="file"&&source&&chunk){files_started=true;SessionEntry e;e.leaf=leaf(r[1]);e.bytes=number(r[2],maximum_bytes);e.sha256=r[3];if(e.sha256.size()!=64||e.sha256.find_first_not_of("0123456789ABCDEF")!=e.sha256.npos||e.bytes>maximum_bytes-total||s.files.size()>=4096||std::any_of(s.files.begin(),s.files.end(),[&](const auto& old){return same(e.leaf,old.leaf);}))throw std::runtime_error("Invalid/duplicate session file hash or byte bound.");total+=e.bytes;s.files.push_back(std::move(e));}
        else throw std::runtime_error("Unknown/out-of-order session row.");}
    if(!source||s.files.empty()||std::none_of(s.files.begin(),s.files.end(),[&](const auto& e){return same(e.leaf,s.source);})||!same(std::filesystem::path(s.source).extension().wstring(),L".bin"))throw std::runtime_error("Session must reference its source .bin.");s.metadata=decode_capture_metadata(meta);return s;
}
SessionManifest create_session_manifest(const std::filesystem::path& input,const std::filesystem::path& destination,CaptureMetadata current,bool capture_start,bool logs,unsigned interval,const std::atomic<bool>& cancel){
    const auto source=std::filesystem::absolute(input).lexically_normal(),output=std::filesystem::absolute(destination).lexically_normal();plain(source);plain(output);
    if(!same(source.parent_path().wstring(),output.parent_path().wstring())||!same(output.extension().wstring(),L".sccsession"))throw std::runtime_error("Save a NEW .sccsession beside the recordings; no raw file copies are created.");
    auto replay=load_replay(source,interval,cancel);SessionManifest s;s.source=source.filename().wstring();s.interval_ms=interval;
    std::vector<std::filesystem::path> paths;std::vector<std::wstring> missing_logs;size_t included_logs=0;
    for(const auto& p:replay->paths){
        paths.push_back(p);auto idx=p;idx.replace_extension(L".idx");if(replay->receipt_times)paths.push_back(idx);
        auto log=p;log.replace_extension(L".log");
        if(logs){if(std::filesystem::exists(log)){paths.push_back(log);++included_logs;}else missing_logs.push_back(log.filename().wstring());}
    }
    if(!missing_logs.empty()){
        std::string detail="Readable logs requested: "+std::to_string(included_logs)+" found, "+std::to_string(missing_logs.size())+" missing beside the raw recordings. No manifest created.\r\n";
        for(size_t i=0;i<std::min<size_t>(missing_logs.size(),16);++i)detail+="Missing: "+message_utf8(missing_logs[i])+"\r\n";
        detail+="Readable logs saved in another folder are not included by this relative-filename format. Choose No for a raw/index-only session, or provide reviewed log copies in the recording folder.";
        throw std::runtime_error(detail);
    }
    const auto meta_path=metadata_path(source);if(capture_start)paths.push_back(meta_path);if(paths.size()>4096)throw std::runtime_error("Too many session files (max 4096).");
    std::vector<std::shared_ptr<ReplayFile>> held;uint64_t total=0;
    for(const auto& p:paths){cancelled(cancel);plain(p);auto file=std::make_shared<ReplayFile>(p);if(file->size()>maximum_bytes-total)throw std::runtime_error("Session hashing exceeds 256 GiB.");total+=file->size();held.push_back(std::move(file));}
    if(capture_start){const auto& file=*held.back();if(file.size()>1100*1024)throw std::runtime_error("Capture metadata exceeds 1100 KiB.");std::string text;for(uint64_t offset=0;offset<file.size();){auto block=file.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,file.size()-offset)),cancel);offset+=block.size();text+=block;}s.metadata=decode_capture_metadata(text);}
    else {current.profile=session_safe_profile(std::move(current.profile));current.source="Current";s.metadata=std::move(current);}
    for(size_t i=0;i<paths.size();++i)s.files.push_back({paths[i].filename().wstring(),held[i]->size(),file_sha256(*held[i],cancel)});
    const auto checked=load_replay(source,interval,cancel);archive_matches(*checked,s);const auto encoded=encode_session_manifest(s);decode_session_manifest(encoded);uart_write_file(output,encoded,false,cancel);return s;
}
SessionManifest verify_session_manifest(const std::filesystem::path& input,const std::atomic<bool>& cancel){
    const auto path=std::filesystem::absolute(input).lexically_normal();plain(path);ReplayFile manifest(path);if(manifest.size()>4*1024*1024)throw std::runtime_error("Manifest exceeds 4 MiB.");std::string text;
    for(uint64_t offset=0;offset<manifest.size();){auto block=manifest.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,manifest.size()-offset)),cancel);offset+=block.size();text+=block;}
    auto s=decode_session_manifest(text);std::vector<std::shared_ptr<ReplayFile>> held;
    for(const auto& e:s.files){cancelled(cancel);const auto source=path.parent_path()/e.leaf;plain(source);auto file=std::make_shared<ReplayFile>(source);if(file->size()!=e.bytes)throw std::runtime_error("Session file length mismatch: "+message_utf8(e.leaf));held.push_back(std::move(file));}
    for(size_t i=0;i<s.files.size();++i)if(file_sha256(*held[i],cancel)!=s.files[i].sha256)throw std::runtime_error("Session SHA256 mismatch: "+message_utf8(s.files[i].leaf));
    const auto replay=load_replay(path.parent_path()/s.source,s.interval_ms,cancel);archive_matches(*replay,s);return s;
}
std::string session_manifest_report(const SessionManifest& s){uint64_t bytes=0;for(const auto& f:s.files)bytes+=f.bytes;std::string out="Session files: "+std::to_string(s.files.size())+" / "+std::to_string(bytes)+" bytes. SHA256 identifies the recorded file contents, not authorship or hardware correctness.\r\nTool at snapshot: "+s.metadata.version+"; origin: "+s.metadata.source+"; numeric settings changed later: "+(s.metadata.changed?"YES":"not marked")+"\r\n";
    out+=s.metadata.source=="Current"?"Settings are a CURRENT reviewed snapshot, not claimed to be capture-time settings.\r\n":"Capture-start settings only; later configuration changes are not replayed automatically.\r\n";
    out+="Source: "+message_utf8(s.source)+"; synthetic interval if unindexed: "+std::to_string(s.interval_ms)+" ms.\r\nNo port is opened, no TX replay occurs. Settings require explicit Stage and existing idle Apply.\r\n";
    for(size_t i=0;i<std::min<size_t>(s.files.size(),64);++i)out+=message_utf8(s.files[i].leaf)+" | "+std::to_string(s.files[i].bytes)+" | "+s.files[i].sha256+"\r\n";if(s.files.size()>64)out+="List limited to first 64 files.\r\n";return out;
}
