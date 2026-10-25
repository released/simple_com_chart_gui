#include "bounded_regex.h"
#include "line_filter.h"
#include "field_extractor.h"
#include "display_preferences.h"
#include "source_mapping.h"
#include "light_terminal.h"
#include "uart_offline.h"
#include "file_transfer.h"
#include <fstream>
#include <windows.h>
#include <cmath>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void check(bool ok,const char* name) {if(!ok) throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}

void reject_reparse_ancestors(const std::filesystem::path& path) {
    std::filesystem::path part;
    for(const auto& component:path) {
        part/=component;
        const auto attributes=GetFileAttributesW(part.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("Test fixture path contains a reparse point");
    }
}

class DisposablePayload {
public:
    explicit DisposablePayload(std::filesystem::path path):path_(std::move(path)) {
        if(std::filesystem::exists(path_)) throw std::runtime_error("Disposable fixture already exists");
    }
    ~DisposablePayload() noexcept {
        if(removed_) return;
        try {
            reject_reparse_ancestors(path_);
            std::error_code error;
            std::filesystem::remove(path_,error);
            if(error) std::cerr<<"CLEANUP FAIL: "<<path_.filename().string()<<": "<<error.message()<<'\n';
        } catch(...) {std::cerr<<"CLEANUP FAIL: disposable path could not be safely validated\n";}
    }
    void remove_checked() {
        reject_reparse_ancestors(path_);
        std::filesystem::remove(path_);
        check(!std::filesystem::exists(path_),"disposable test payload removed after readers close");
        removed_=true;
    }
private:
    std::filesystem::path path_;
    bool removed_=false;
};
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc!=2) throw std::runtime_error("Expected test-owned evidence folder");
        const auto root=std::filesystem::absolute(argv[1]).lexically_normal();
        reject_reparse_ancestors(root);std::filesystem::create_directories(root);reject_reparse_ancestors(root);
        const auto folder=root/(std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
        check(std::filesystem::create_directory(folder),"fresh test-owned run directory");
        std::atomic<bool> cancel{false};
        std::vector<std::unique_ptr<DisposablePayload>> small_payloads;
        const auto own_small=[&](const std::filesystem::path& path) {small_payloads.push_back(std::make_unique<DisposablePayload>(path));};
        const auto failure_payload=folder/L"cleanup-failure.tmp";
        try {
            DisposablePayload owned(failure_payload);
            std::ofstream(failure_payload,std::ios::binary)<<"disposable";
            throw std::runtime_error("intentional fixture failure");
        } catch(const std::runtime_error&) {}
        check(!std::filesystem::exists(failure_payload),"disposable payload cleaned during exception unwinding");
        const auto cancelled_payload=folder/L"cleanup-cancel.log";bool cancelled_rejected=false;
        try {
            DisposablePayload owned(cancelled_payload);
            std::ofstream(cancelled_payload,std::ios::binary)<<"n:1\n";
            const std::atomic<bool> cancelled{true};
            load_replay(cancelled_payload,1,cancelled);
        } catch(const std::exception&) {cancelled_rejected=true;}
        check(cancelled_rejected&&!std::filesystem::exists(cancelled_payload),"cancelled replay releases and cleans its owned input");
        std::string error;BoundedRegex regex;
        check(regex.compile(R"((name):([+-]?[0-9]+(?:\.[0-9]+)?))",error),"capture regex compiles");
        auto match=regex.search("prefix name:-12.5 suffix");
        check(match.found && match.bounds[2]==7 && match.bounds[3]==11 && match.bounds[4]==12 && match.bounds[5]==17,"capture group byte offsets");
        check(regex.compile(R"(^(a+)+$)",error),"nested quantifiers compile to NFA");
        const auto begin=std::chrono::steady_clock::now();
        match=regex.search(std::string(32000,'a')+"!");
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count();
        check(!match.found && !match.limited && ms<2000,"catastrophic backtracking input completes linearly");
        check(regex.compile("[a-z]+",error) && regex.search("abc",1).limited,"work budget explicitly limits matching");
        check(!regex.compile(R"((a)\1)",error) && !regex.compile("a{999999}",error),"backreferences and oversized expansions rejected");
        LineFilter filter;check(filter.configure("fan|ac_adc", "error|timeout",true,error),"filter alternatives");
        check(filter.matches("fan,rpm:123") && !filter.matches("fan,error:1"),"exclude wins");
        check(!filter.configure("[","",true,error) && filter.matches("ac_adc:1"),"invalid configure preserves active filter");
        check(!filter.matches(std::string(70000,'a')) && filter.limited()==1,"oversized lines are observably skipped");
        std::cout<<"Regex adversarial time: "<<ms<<" ms\n";
        FieldExtractor extractor;ExtractionConfig config;config.mode=ExtractionConfig::Csv;config.columns="time,voltage,current";config.time_field="time";
        check(extractor.configure(config,error),"CSV config");auto fields=extractor.extract("1.25,\"3.3V\",-2.5");
        check(fields.values.size()==3 && fields.seconds==1.25 && fields.values.at("voltage")==3.3,"CSV quoted fields and time");
        check(!extractor.extract("1e308,3,4").error.empty(),"unusable time magnitudes rejected without changing raw numeric rules");
        check(extractor.extract("1,2").values.empty(),"mismatched CSV has no fabricated values");
        fields=extractor.extract("1,\"2,3\",4");check(fields.values.count("voltage")==0,"quoted delimiter is not numeric prefix");
        config.mode=ExtractionConfig::Tab;check(extractor.configure(config,error) && extractor.extract("2\t3\t4").values.size()==3,"Tab fields");
        config.mode=ExtractionConfig::Json;config.time_field="";check(extractor.configure(config,error),"JSON config");
        fields=extractor.extract(R"({"rpm":201,"voltage":-1.25e2,"text":"5","nested":{"value":7},"array":[1,2]})");
        check(fields.values.size()==2 && fields.values.at("voltage")==-125,"flat JSON numeric-only fields");
        check(!extractor.extract(R"({"a":01})").error.empty() && !extractor.extract(R"({"a":1e999})").error.empty(),"invalid/nonfinite JSON rejected");
        config.mode=ExtractionConfig::Regex;config.pattern=R"(([A-Za-z_]+)\s*=\s*([+-]?[0-9.]+) at ([0-9.]+))";config.time_group=3;
        check(extractor.configure(config,error),"generic regex extraction config");fields=extractor.extract("temp = -12.5 at 8.25");
        check(fields.values.at("temp")==-12.5 && fields.seconds==8.25,"name/value/time capture extraction");
        const auto restored=decode_extraction(encode_extraction(config));check(restored.pattern==config.pattern && restored.time_group==3,"portable parser roundtrip");
        DisplayPreferences preferences;ChannelDisplay display;display.alias="Voltage";display.unit="V";display.decimals=2;display.scale=.001;display.offset=-1;
        check(preferences.set("adc",display,error) && std::abs(preferences.project("adc",3300)-2.3)<1e-12 && preferences.format("adc",2.3)=="2.30 V","display-only projection and precision");
        check(DisplayPreferences::decode(preferences.encode()).name("adc")=="Voltage","display preferences roundtrip");
        SourceLineTracker tracker;check(tracker.feed("a:1\r").empty(),"split source line waits for LF");auto mapped=tracker.feed("\nb:2\n");
        check(mapped.size()==2 && mapped[0].offset==0 && mapped[1].offset==5 && tracker.end()==9,"exact source offsets across chunks");
        tracker.feed(std::string(70000,'x'));mapped=tracker.feed("\na:3\n");check(tracker.skipped()==1 && mapped.size()==1 && mapped[0].text=="a:3","oversized source line resynchronizes without fabricated tail");
        MessageDisplayOptions options;check(format_light_terminal("12345\rAB",options).text==L"AB345","terminal CR overwrites same row");
        check(format_light_terminal("ab\bZ",options).text==L"aZ" && format_light_terminal("123\r\x1b[2KX",options).text==L"X","terminal BS and erase-line");
        check(format_light_terminal(std::string(256*1024,'\n'),options).text.size()<=16384,"terminal short-line retention uses bounded constant-time eviction");
        ReplayRecords records;for(size_t i=0;i<12000;++i) records.push_back({i,1,static_cast<double>(i),0});records.finish();
        check(records.paged() && records[0].offset==0 && records[6553].offset==6553 && records[11999].offset==11999,"disk-paged index random access and page-boundary record");
        const auto large=folder/L"large.log";DisposablePayload large_payload(large);
        {std::ofstream recipe(folder/L"fixture-recipe.txt");recipe<<"large.log: 290000 rows of n:1, + 251 ASCII spaces + LF; 74240000 bytes.\nDisposable input; removed after replay readers close.\n";if(!recipe) throw std::runtime_error("Cannot record fixture recipe");}
        {std::ofstream output(large,std::ios::binary);const std::string row="n:1,"+std::string(251,' ')+"\n";for(size_t i=0;i<290000;++i) output.write(row.data(),static_cast<std::streamsize>(row.size()));output.close();if(!output) throw std::runtime_error("Cannot generate complete large-log fixture");}
        auto archive=load_replay(large,1,cancel);check(archive->total_bytes>64*1024*1024 && archive->records.size()>200000 && archive->records.paged(),"large log exceeds old byte/record limits with bounded resident index");
        check(replay_seek_time(*archive,200)==200000 && archive->read_record(200000,cancel).rfind("n:1,",0)==0,"large log time seek and background-readable record");archive.reset();
        large_payload.remove_checked();
        std::cout<<"Retention: generated=74240000 removed=74240000 retained_bulk=0 bytes; recipe=fixture-recipe.txt\n";
        for(int i=1;i<=10;++i) {const auto raw=folder/(L"rotate_"+std::to_wstring(i)+L".bin");own_small(raw);uart_write_file(raw,i==1?"END":i==2?"START":"x",false,cancel);}
        archive=load_replay(folder/L"rotate_5.bin",10,cancel);
        check(archive->paths.size()==10 && archive->paths[1].filename()==L"rotate_2.bin" && archive->paths[9].filename()==L"rotate_10.bin","rotation uses numeric rather than lexical ordering");
        check(replay_find(*archive,0,"ENDSTART",cancel)==0 && replay_find(*archive,0,"STARTx",cancel)==1,"cross-segment search returns source record");archive.reset();
        bool rotation_limit=false;try{load_replay(folder/L"rotate_5.bin",10,cancel,10);}catch(const std::exception&){rotation_limit=true;}
        check(rotation_limit,"selected byte limit rejects cumulative rotation before indexing");
        const auto standalone=folder/L"unrelated_123.BIN";own_small(standalone);uart_write_file(standalone,"a:1\n",false,cancel);archive=load_replay(standalone,1,cancel);check(archive->paths.size()==1 && !archive->receipt_times,"ordinary uppercase numbered binary is not mistaken for missing rotations");archive.reset();
        const auto binary=folder/L"send.bin";own_small(binary);const std::string original("A\0B\xff\r\n",6);uart_write_file(binary,original,false,cancel);
        FileTransferOptions transfer_options;transfer_options.chunk=1;auto transfer=FileTransfer::prepare(binary,transfer_options,cancel);std::string sent;
        do{const auto part=transfer->next(cancel);check(part.bytes.size()<=1,"file TX bounded chunk");sent+=part.bytes;if(part.end) break;}while(true);
        check(sent==original,"binary file TX preserves NUL and non-ASCII bytes");transfer.reset();transfer_options.text=true;
        bool rejected=false;try{FileTransfer::prepare(binary,transfer_options,cancel);}catch(...){rejected=true;}check(rejected,"invalid UTF-8 text rejected before any transmission");
        const auto text_file=folder/L"send.txt";own_small(text_file);uart_write_file(text_file,"a\r\nb\rc\n",false,cancel);transfer_options.newline=2;transfer_options.chunk=2;transfer=FileTransfer::prepare(text_file,transfer_options,cancel);sent.clear();
        do{const auto part=transfer->next(cancel);sent+=part.bytes;if(part.end) break;}while(true);check(sent=="a\r\nb\r\nc\r\n","explicit text newline conversion and chunk splitting");
        transfer.reset();
        for(const auto& payload:small_payloads) payload->remove_checked();
        std::cout<<"Retention: removed 13 small synthetic fixtures; only fixture-recipe.txt retained\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
