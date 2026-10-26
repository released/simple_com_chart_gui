#include "uart_offline.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace {
bool binary_file(const std::filesystem::path& path) {auto ext=path.extension().wstring();std::transform(ext.begin(),ext.end(),ext.begin(),towlower);return ext==L".bin";}
uint64_t integer(std::string_view text) {
    uint64_t n=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),n);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()) throw std::runtime_error("Malformed index/segment number.");return n;
}
std::vector<std::filesystem::path> segments(const std::filesystem::path& selected) {
    const auto stem=selected.stem().wstring();const auto split=stem.find_last_of(L'_');
    if(split==stem.npos || !binary_file(selected)) return {selected};
    const auto suffix=stem.substr(split+1);if(suffix.empty()||suffix.find_first_not_of(L"0123456789")!=suffix.npos) return {selected};
    const auto base=stem.substr(0,split+1);std::vector<std::pair<uint64_t,std::filesystem::path>> found;
    auto adjacent_index=selected;adjacent_index.replace_extension(L".idx");
    if(!std::filesystem::exists(selected.parent_path()/(base+L"1.bin")) && !std::filesystem::exists(adjacent_index)) return {selected};
    for(const auto& entry:std::filesystem::directory_iterator(selected.parent_path())) {
        const auto name=entry.path().stem().wstring();if(!binary_file(entry.path()) || name.rfind(base,0)!=0) continue;
        const auto number=name.substr(base.size());if(number.empty()||number.find_first_not_of(L"0123456789")!=number.npos) continue;
        std::string digits;for(const auto c:number) digits+=static_cast<char>(c);
        found.emplace_back(integer(digits),entry.path());if(found.size()>4096) throw std::runtime_error("More than 4096 rotation segments.");
    }
    std::sort(found.begin(),found.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    for(size_t i=0;i<found.size();++i) if(found[i].first!=i+1) throw std::runtime_error("Rotation sequence is missing/duplicated a segment; keep completed contiguous _1.._N files together.");
    std::vector<std::filesystem::path> result;for(const auto& entry:found) result.push_back(entry.second);return result;
}
template<class F> void index_lines(const ReplayFile& file,const std::atomic<bool>& cancel,F consume) {
    std::string carry;
    for(uint64_t offset=0;offset<file.size();) {
        const auto bytes=file.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,file.size()-offset)),cancel);offset+=bytes.size();
        for(const char c:bytes) {
            if(c=='\n') {if(!carry.empty()&&carry.back()=='\r') carry.pop_back();consume(carry);carry.clear();}
            else {carry+=c;if(carry.size()>4096) throw std::runtime_error("Recording index line exceeds 4096 bytes.");}
        }
    }
    if(!carry.empty()) {if(carry.back()=='\r') carry.pop_back();consume(carry);}
}
}
std::string ReplayData::read_record(size_t index,const std::atomic<bool>& cancel) const {
    const auto r=records[index];return files.at(r.file)->read(r.offset,r.count,cancel);
}
std::shared_ptr<ReplayData> load_replay(const std::filesystem::path& path,unsigned interval,const std::atomic<bool>& cancel,uint64_t input_limit) {
    if(interval<1||interval>60000) throw std::runtime_error("Synthetic interval must be 1..60000 ms.");
    if(!input_limit||input_limit>64ULL*1024*1024*1024) throw std::runtime_error("Invalid replay disk-input limit.");
    auto data=std::make_shared<ReplayData>();data->paths=segments(std::filesystem::absolute(path));
    for(const auto& file:data->paths){
        if(cancel)throw std::runtime_error("Cancelled");auto raw=std::make_shared<ReplayFile>(file);
        if(raw->size()>input_limit-data->total_bytes)throw std::runtime_error("Replay archive exceeds selected disk-input byte limit.");
        data->total_bytes+=raw->size();data->files.push_back(std::move(raw));
    }
    size_t indexed=0;for(const auto& file:data->paths) {auto index=file;index.replace_extension(L".idx");if(binary_file(file) && std::filesystem::exists(index)) ++indexed;}
    if(indexed && indexed!=data->paths.size()) throw std::runtime_error("Mixed indexed/unindexed segments: restore all matching .idx files.");
    data->receipt_times=indexed!=0;uint64_t last_ms=0,generation=0;double elapsed=0;bool gap=false;
    for(size_t f=0;f<data->paths.size();++f) {
        if(cancel) throw std::runtime_error("Cancelled");const auto& raw=data->files[f];
        if(indexed) {
            auto index=data->paths[f];index.replace_extension(L".idx");ReplayFile input(index);size_t line_number=0;uint64_t expected=0;
            if(input_limit<64ULL*1024*1024*1024&&input.size()>input_limit)throw std::runtime_error("Recording index exceeds selected disk-input byte limit.");
            index_lines(input,cancel,[&](const std::string& line) {
                if(line_number++==0) {if(line!="SCC-RX-INDEX,1") throw std::runtime_error("Unsupported recording index header.");return;}
                if(line_number==2) {if(line!="offset,length,unix_ms,event") throw std::runtime_error("Unsupported index columns.");return;}
                const auto a=line.find(','),b=line.find(',',a==line.npos?a:a+1),c=line.find(',',b==line.npos?b:b+1);
                if(a==line.npos||b==line.npos||c==line.npos) throw std::runtime_error("Malformed recording index row.");
                const auto offset=integer(std::string_view(line).substr(0,a)),count=integer(std::string_view(line).substr(a+1,b-a-1)),ms=integer(std::string_view(line).substr(b+1,c-b-1));
                if(offset!=expected||count>raw->size()-expected||ms>253402300799999ULL) throw std::runtime_error("Recording index offset/time mismatch.");
                if(!count) {gap=gap||line.find("INPUT GAP")!=line.npos;auto event=line.substr(c+1);std::transform(event.begin(),event.end(),event.begin(),[](unsigned char v){return static_cast<char>(std::tolower(v));});if(event.rfind("com: connected",0)==0||event.rfind("com: reconnected",0)==0||event.rfind("manual disconnect",0)==0||event.rfind("unexpected disconnect",0)==0||event.rfind("b connected",0)==0||event.rfind("connect",0)==0||event.rfind("disconnect",0)==0||event.rfind("recorder input gap",0)==0)++generation;return;}
                if(count>65536||!ms||line.substr(c+1)!="RX") throw std::runtime_error("Unsupported recording RX index record.");
                if(last_ms) elapsed+=static_cast<double>(ms>=last_ms?ms-last_ms:0)/1000;last_ms=ms;
                for(size_t part=0;part<count;part+=4096) data->records.push_back({static_cast<size_t>(offset)+part,std::min<size_t>(4096,static_cast<size_t>(count)-part),elapsed,static_cast<int64_t>(ms),static_cast<uint32_t>(f),generation});
                expected+=count;
            });
            if(line_number<2 || expected!=raw->size()) throw std::runtime_error("Incomplete index: use completed, matching segments.");
        } else {
            uint64_t start=0;
            for(uint64_t offset=0;offset<raw->size();) {
                const auto bytes=raw->read(offset,static_cast<size_t>(std::min<uint64_t>(65536,raw->size()-offset)),cancel);
                for(size_t i=0;i<bytes.size();++i) {
                    const uint64_t at=offset+i;
                    if(bytes[i]=='\n'||at-start+1==4096||at+1==raw->size()) {
                        data->records.push_back({static_cast<size_t>(start),static_cast<size_t>(at-start+1),static_cast<double>(data->records.size())*interval/1000,0,static_cast<uint32_t>(f)});start=at+1;
                    }
                }
                offset+=bytes.size();
            }
        }
    }
    if(data->records.empty()) throw std::runtime_error("Input contains no replay bytes.");data->records.finish();
    data->provenance=data->receipt_times?L"PC receipt intervals; backwards clock steps clamped, not wire timing":L"SYNTHETIC time: "+std::to_wstring(interval)+L" ms per line/chunk; not file/device timestamps";
    data->provenance+=L" | "+std::to_wstring(data->paths.size())+L" segment(s), "+std::to_wstring(data->total_bytes)+L" bytes; background reads / paged index";
    data->has_input_gap=gap;if(gap) data->provenance+=L"; WARNING: recorder input gap";return data;
}
size_t replay_seek_time(const ReplayData& data,double seconds) {
    size_t a=0,b=data.records.size();while(a<b){const auto mid=a+(b-a)/2;if(data.records[mid].seconds<seconds) a=mid+1;else b=mid;}return a;
}
size_t replay_find(const ReplayData& data,size_t first,std::string_view pattern,const std::atomic<bool>& cancel) {
    if(pattern.empty()||pattern.size()>4096) throw std::runtime_error("Search pattern must be 1..4096 bytes.");if(first>=data.records.size()) return data.records.size();
    const auto begin=data.records[first];std::string carry;std::vector<uint64_t> bases(1,0);for(const auto& file:data.files) bases.push_back(bases.back()+file->size());
    for(size_t f=begin.file;f<data.files.size();++f) {
        for(uint64_t offset=f==begin.file?begin.offset:0;offset<data.files[f]->size();) {
            const auto bytes=data.files[f]->read(offset,static_cast<size_t>(std::min<uint64_t>(65536,data.files[f]->size()-offset)),cancel);const auto previous=carry.size();carry+=bytes;
            const auto found=carry.find(pattern);
            if(found!=carry.npos) {
                const uint64_t global=bases[f]+offset-previous+found;
                const size_t source=static_cast<size_t>(std::upper_bound(bases.begin(),bases.end(),global)-bases.begin()-1);
                const uint64_t at=global-bases[source];
                size_t a=0,b=data.records.size();
                while(a<b){const auto mid=a+(b-a)/2;const auto r=data.records[mid];if(r.file<source||(r.file==source && r.offset+r.count<=at)) a=mid+1;else b=mid;}return a;
            }
            if(carry.size()>=pattern.size()) carry.erase(0,carry.size()-(pattern.size()-1));offset+=bytes.size();
        }
    }
    return data.records.size();
}
void ReplayClock::load(std::shared_ptr<const ReplayData> data) { data_ = std::move(data); position_ = 0; anchor_seconds_ = 0; playing_ = false; }
double ReplayClock::time(uint64_t now) const { return anchor_seconds_ + (playing_ && now >= anchor_ms_ ? static_cast<double>(now-anchor_ms_) * speed_ / 1000 : 0); }
void ReplayClock::play(uint64_t now, double speed) {
    if (!data_ || position_ == data_->records.size() || !std::isfinite(speed) || speed < 0.1 || speed > 100) return;
    anchor_seconds_ = time(now); anchor_ms_ = now; speed_ = speed; playing_ = true;
}
void ReplayClock::pause(uint64_t now) { anchor_seconds_ = time(now); playing_ = false; }
void ReplayClock::seek(size_t record, uint64_t now) { if (!data_) return; position_ = std::min(record, data_->records.size()); anchor_seconds_ = position_ < data_->records.size() ? data_->records[position_].seconds : 0; anchor_ms_ = now; }
size_t ReplayClock::due(uint64_t now, size_t budget) {
    if (!data_ || !playing_) return 0;
    size_t count = 0; const auto current = time(now);
    while (position_ < data_->records.size() && count < budget && data_->records[position_].seconds <= current) { ++position_; ++count; }
    if (position_ == data_->records.size()) playing_ = false;
    return count;
}
