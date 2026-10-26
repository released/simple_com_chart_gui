#include "recording_tools.h"
#include <windows.h>
#include <algorithm>
#include <charconv>
#include <sstream>
#include <stdexcept>

namespace {
uint64_t integer(std::string_view text) {
    uint64_t n=0;const auto r=std::from_chars(text.data(),text.data()+text.size(),n);
    if(r.ec!=std::errc{}||r.ptr!=text.data()+text.size())throw std::runtime_error("Malformed index number.");return n;
}
void check_cancel(const std::atomic<bool>& c){if(c)throw std::runtime_error("Cancelled; originals unchanged.");}
class CreatedFile {
public:
    explicit CreatedFile(const std::filesystem::path& path) {
        handle_=CreateFileW(path.c_str(),GENERIC_WRITE|DELETE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(handle_==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot create output; choose a new writable filename (no overwrite).");
    }
    ~CreatedFile(){rollback();}
    void write(std::string_view bytes,const std::atomic<bool>& cancel) {
        while(!bytes.empty()){check_cancel(cancel);DWORD count=0;const auto n=static_cast<DWORD>(std::min<size_t>(bytes.size(),65536));
            if(!WriteFile(handle_,bytes.data(),n,&count,nullptr)||!count)throw std::runtime_error("Output write failed/disk full.");bytes.remove_prefix(count);}
    }
    void flush(){if(!FlushFileBuffers(handle_))throw std::runtime_error("Output flush failed.");}
    void commit(){if(handle_!=INVALID_HANDLE_VALUE){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;}}
    bool rollback()noexcept {
        if(handle_==INVALID_HANDLE_VALUE)return true;
        FILE_DISPOSITION_INFO remove{TRUE};const bool ok=SetFileInformationByHandle(handle_,FileDispositionInfo,&remove,sizeof(remove))!=FALSE;
        CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;return ok;
    }
private:HANDLE handle_=INVALID_HANDLE_VALUE;
};
bool generation_event(std::string text) {
    std::transform(text.begin(),text.end(),text.begin(),[](unsigned char c){return static_cast<char>(tolower(c));});
    for(const auto prefix:{"com: connected","com: reconnected","manual disconnect","unexpected disconnect","b connected","connect","disconnect","recorder input gap"})if(text.rfind(prefix,0)==0)return true;
    return false;
}
}
std::string RecordingHealth::report()const {
    std::ostringstream out;out<<"Selected segment only; original files are read-only.\r\nRaw bytes: "<<raw_bytes<<"\r\nValid indexed prefix: "<<valid_bytes
        <<"\r\nReplay records: "<<(prefix?prefix->records.size():0)<<"\r\nStructure: "<<(complete?"complete/matching":"incomplete or invalid")
        <<"\r\nRecorder input gap: "<<(input_gap?"YES":"not declared")<<"\r\n"<<detail
        <<"\r\nNo payload hash exists in this recording format: structural checks do not prove byte integrity or power-loss durability.";return out.str();
}
RecordingHealth inspect_recording(const std::filesystem::path& selected,const std::atomic<bool>& cancel) {
    check_cancel(cancel);RecordingHealth h;auto raw=std::make_shared<ReplayFile>(selected);h.raw_bytes=raw->size();
    if(h.raw_bytes>64ULL*1024*1024*1024)throw std::runtime_error("Selected segment exceeds 64 GiB.");
    auto index=selected;index.replace_extension(L".idx");
    auto ext=selected.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),towlower);
    if(ext!=L".bin"||!std::filesystem::exists(index)){h.detail="An adjacent .idx is required; receipt timing cannot be reconstructed from raw bytes.";return h;}
    ReplayFile idx(index);if(idx.size()>4ULL*1024*1024*1024)throw std::runtime_error("Index exceeds 4 GiB.");
    auto data=std::make_shared<ReplayData>();data->files.push_back(raw);data->paths.push_back(selected);data->receipt_times=true;
    uint64_t last_ms=0,generation=0;double seconds=0;size_t line_number=0;bool stopped=false;std::string carry;
    const auto consume=[&](std::string line) {
        if(stopped)return;if(!line.empty()&&line.back()=='\r')line.pop_back();
        try {
            if(line_number++==0){if(line!="SCC-RX-INDEX,1")throw std::runtime_error("Unsupported index header.");return;}
            if(line_number==2){if(line!="offset,length,unix_ms,event")throw std::runtime_error("Unsupported index columns.");return;}
            const auto a=line.find(','),b=a==line.npos?a:line.find(',',a+1),c=b==line.npos?b:line.find(',',b+1);
            if(c==line.npos)throw std::runtime_error("Truncated/malformed row.");
            const auto offset=integer(std::string_view(line).substr(0,a)),count=integer(std::string_view(line).substr(a+1,b-a-1)),ms=integer(std::string_view(line).substr(b+1,c-b-1));
            if(offset!=h.valid_bytes||count>h.raw_bytes-h.valid_bytes||ms>253402300799999ULL)throw std::runtime_error("Index offset/length/time exceeds matching raw bytes.");
            const auto event=line.substr(c+1);
            if(!count){if(event.find("INPUT GAP")!=event.npos)h.input_gap=true;if(generation_event(event))++generation;return;}
            if(count>65536||!ms||event!="RX")throw std::runtime_error("Invalid RX index record.");
            if(data->records.size()+(count+4095)/4096>50000000)throw std::runtime_error("Valid-prefix replay record limit reached.");
            if(last_ms)seconds+=static_cast<double>(ms>=last_ms?ms-last_ms:0)/1000;last_ms=ms;
            for(uint64_t part=0;part<count;part+=4096)data->records.push_back({static_cast<size_t>(offset+part),static_cast<size_t>(std::min<uint64_t>(4096,count-part)),seconds,static_cast<int64_t>(ms),0,generation});
            h.valid_bytes+=count;
        }catch(const std::exception& e){stopped=true;h.detail="Stopped at index line "+std::to_string(line_number)+": "+e.what();}
    };
    for(uint64_t offset=0;offset<idx.size()&&!stopped;) {
        check_cancel(cancel);const auto bytes=idx.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,idx.size()-offset)),cancel);offset+=bytes.size();
        for(char c:bytes){if(stopped)break;if(c=='\n'){consume(std::move(carry));carry.clear();}else{carry+=c;if(carry.size()>4096){stopped=true;h.detail="Index row exceeds 4096 bytes.";}}}
    }
    if(!stopped&&!carry.empty())consume(std::move(carry));
    h.complete=!stopped&&line_number>=2&&h.valid_bytes==h.raw_bytes;
    if(h.detail.empty())h.detail=h.complete?"Every indexed RX span matches the selected segment.":"Index ends before the raw segment, or has no complete header.";
    data->total_bytes=h.valid_bytes;data->has_input_gap=h.input_gap;data->records.finish();data->provenance=L"Salvaged valid indexed prefix; original evidence unchanged";h.prefix=std::move(data);return h;
}
std::string export_recording_range(const ReplayData& data,size_t first,size_t end,const std::filesystem::path& output,const std::atomic<bool>& cancel) {
    check_cancel(cancel);
    auto ext=output.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),towlower);
    if(ext!=L".bin"||first>=end||end>data.records.size())throw std::runtime_error("Select nonempty [first,past-last) records and a NEW .bin filename.");
    auto index=output;index.replace_extension(L".idx");
    CreatedFile raw(output);std::unique_ptr<CreatedFile> idx;
    uint64_t offset=0,generation=UINT64_MAX;
    try {
        if(data.receipt_times){idx=std::make_unique<CreatedFile>(index);idx->write("SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n",cancel);}
        if(idx&&data.has_input_gap)idx->write("0,0,"+std::to_string(data.records[first].unix_ms)+",SOURCE INPUT GAP declared; selected-range gap location unknown\r\n",cancel);
        for(size_t i=first;i<end;++i) {
            check_cancel(cancel);const auto r=data.records[i];const auto bytes=data.read_record(i,cancel);
            if(bytes.size()!=r.count)throw std::runtime_error("Source read count mismatch.");
            raw.write(bytes,cancel);
            if(idx) {
                if(r.generation!=generation){generation=r.generation;idx->write(std::to_string(offset)+",0,"+std::to_string(r.unix_ms)+",connect exported generation\r\n",cancel);}
                if(r.unix_ms<=0)throw std::runtime_error("Indexed source has no valid receipt time.");
                idx->write(std::to_string(offset)+','+std::to_string(bytes.size())+','+std::to_string(r.unix_ms)+",RX\r\n",cancel);
            }
            offset+=bytes.size();
        }
        raw.flush();if(idx)idx->flush();check_cancel(cancel);
        raw.commit();if(idx)idx->commit();
    }catch(...) {
        const bool a=raw.rollback(),b=!idx||idx->rollback();
        if(!a||!b)throw std::runtime_error("Output failed; owned partial output could not be removed. Originals unchanged; inspect destination.");throw;
    }
    std::ostringstream summary;summary<<"Saved "<<offset<<" bytes, records ["<<first<<','<<end<<"), source seconds "<<data.records[first].seconds<<".."<<data.records[end-1].seconds
        <<(data.receipt_times?"; new matching .idx, PC receipt timing.":"; raw .bin only, no receipt index invented (source time is synthetic).")
        <<" Original files unchanged; source record boundaries retained.";return summary.str();
}
