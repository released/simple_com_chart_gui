#include "message_buffer.h"
#include "uart_recorder.h"
#include "recording_overwrite.h"
#include "disposable_test_files.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void check(bool ok, const char* name) { if (!ok) throw std::runtime_error(name); std::cout << "PASS: " << name << '\n'; }
std::string read(const std::filesystem::path& path) { std::ifstream file(path, std::ios::binary); return {std::istreambuf_iterator<char>(file), {}}; }
struct FixtureDirectories {
    std::filesystem::path fixture,base;bool base_created=false;
    ~FixtureDirectories() noexcept {try{DisposableTestFiles::plain_path(fixture);std::error_code e;std::filesystem::remove(fixture,e);if(e)std::cerr<<"CLEANUP FAIL: UART fixture directory retained\n";if(base_created){DisposableTestFiles::plain_path(base);std::filesystem::remove(base,e);}}catch(...){std::cerr<<"CLEANUP FAIL: unsafe UART fixture directory retained\n";}}
};
}
int wmain(int argc, wchar_t** argv) {
    try {
        if(argc==3 && std::wstring(argv[2])==L"--recording-status") {
            wchar_t work[32768]{};
            if(!GetEnvironmentVariableW(L"SCC_TEST_WORK",work,32768) || std::filesystem::absolute(argv[1]).lexically_normal()!=std::filesystem::absolute(work).lexically_normal())throw std::runtime_error("Registered test work required");
            const auto folder=std::filesystem::path(work)/L"recording-status";
            if(!std::filesystem::create_directory(folder))throw std::runtime_error("Fresh recorder fixture required");
            UartRecorder recorder;RecordingOptions options;options.base=folder/L"ERROR_case";
            check(recorder.start(options),"ERROR filename recording starts");recorder.append("v:1\n",1700000000000);
            for(unsigned n=0;n<500 && recorder.status().bytes!=4 && !recorder.status().failed;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));
            const auto live=recorder.status();
            check(live.active && live.bytes==4 && !live.failed && live.detail.find(L"ERROR")!=live.detail.npos,"active recording with ERROR in path is successful");
            recorder.stop();check(recorder.wait_idle(5000)&&!recorder.status().failed&&!recorder.status().limit_reached,"normal stop retains success");
            check(read(folder/L"ERROR_case_1.bin")=="v:1\n","status correction preserves exact raw output");
            check(recorder.start(options),"collision starts asynchronously");
            check(recorder.wait_idle(5000)&&recorder.status().failed,"create-new collision has typed failure");
            options.base=folder/L"limited";options.limits.bytes=256;
            check(recorder.start(options),"limit fixture starts");recorder.append(std::string(500,'x'),1700000000000);
            check(recorder.wait_idle(5000)&&recorder.status().limit_reached&&!recorder.status().failed,"configured limit is distinct from I/O failure");
            options.base=folder/L"gap";options.limits={};
            check(recorder.start(options),"gap fixture starts");recorder.append(std::string(8*1024*1024+1,'x'),1700000000000);
            check(recorder.wait_idle(5000)&&recorder.status().lost>0&&!recorder.status().failed,"input gap remains distinct and observable");
            options.base=folder/L"missing"/L"capture";
            check(recorder.start(options),"I/O failure fixture starts");
            check(recorder.wait_idle(5000)&&recorder.status().failed&&!recorder.status().limit_reached,"open failure has typed failure");
            options.base=folder/L"recovered";
            check(recorder.start(options),"new recording after error starts");recorder.stop();
            check(recorder.wait_idle(5000)&&!recorder.status().failed&&!recorder.status().lost,"new run clears old failure and loss");
            const auto raw_folder=folder/L"raw",log_folder=folder/L"readable";
            std::filesystem::create_directory(raw_folder);std::filesystem::create_directory(log_folder);
            const auto put=[](const std::filesystem::path& path){std::ofstream file(path,std::ios::binary);file<<"USER SENTINEL";};
            for(unsigned kind=0;kind<4;++kind){
                const auto name=L"capture_"+std::to_wstring(kind);
                RecordingOptions requested;requested.base=raw_folder/name;requested.readable_base=log_folder/name;requested.metadata="new metadata";
                const auto occupied=kind==0?std::filesystem::path(requested.base.wstring()+L".sccmeta"):
                    kind==1?std::filesystem::path(requested.base.wstring()+L"_1.bin"):
                    kind==2?std::filesystem::path(requested.base.wstring()+L"_42.idx"):
                    std::filesystem::path(requested.readable_base.wstring()+L"_99.log");
                put(occupied);requested.overwrite=existing_recording_files(requested);
                check(requested.overwrite.size()==1&&read(occupied)=="USER SENTINEL","overwrite preview covers raw/metadata/readable/orphan files without modifying them");
                check(recorder.start(requested),"explicit confirmed split-destination overwrite starts");recorder.append("NEW\r\n",1700000000000);recorder.stop();
                const bool complete=recorder.wait_idle(5000);if(!complete||recorder.status().failed)std::wcerr<<L"DETAIL: "<<recorder.status().detail<<L'\n';
                check(complete&&!recorder.status().failed&&read(requested.base.wstring()+L"_1.bin")=="NEW\r\n"&&read(requested.base.wstring()+L".sccmeta")=="new metadata","confirmed overwrite retains requested names and exact new RX");
                check((kind<2||!std::filesystem::exists(occupied))&&recorder.status().log_filename==name+L"_1.log"&&read(requested.readable_base.wstring()+L"_1.log").find("NEW")!=std::string::npos,"old tails retire only after confirmation and new log keeps the chosen name");
            }
            RecordingOptions raced;raced.base=raw_folder/L"raced";raced.readable_base=log_folder/L"raced";raced.metadata="new";
            put(raced.base.wstring()+L".sccmeta");
            check(recorder.start(raced)&&recorder.wait_idle(5000)&&recorder.status().failed&&read(raced.base.wstring()+L".sccmeta")=="USER SENTINEL","post-preflight collision still fails closed without overwrite or retry");
            check(recorder.status().detail.find(L"Windows error 80")!=std::wstring::npos&&recorder.status().detail.find(L"raced.sccmeta")!=std::wstring::npos,"create failure preserves its immediate Windows code and filename");
            raced.overwrite=existing_recording_files(raced);put(raced.base.wstring()+L"_1.bin");
            check(recorder.start(raced)&&recorder.wait_idle(5000)&&recorder.status().failed&&read(raced.base.wstring()+L".sccmeta")=="USER SENTINEL"&&read(raced.base.wstring()+L"_1.bin")=="USER SENTINEL","files appearing after confirmation invalidate the entire approval");
            raced.overwrite=existing_recording_files(raced);{std::ofstream changed(raced.base.wstring()+L".sccmeta");changed<<"CHANGED AFTER CONFIRMATION";}
            check(recorder.start(raced)&&recorder.wait_idle(5000)&&recorder.status().failed&&read(raced.base.wstring()+L".sccmeta")=="CHANGED AFTER CONFIRMATION","changed approved file is preserved instead of overwritten");
            raced.overwrite=existing_recording_files(raced);
            HANDLE locked=CreateFileW((raced.base.wstring()+L".sccmeta").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
            check(locked!=INVALID_HANDLE_VALUE,"locked overwrite fixture opens");
            const bool locked_start=recorder.start(raced)&&recorder.wait_idle(5000)&&recorder.status().failed;CloseHandle(locked);
            check(locked_start&&read(raced.base.wstring()+L".sccmeta")=="CHANGED AFTER CONFIRMATION","active recording files cannot be overwritten despite confirmation");
            raced.readable_base=folder/L"missing-output-folder"/L"raced";raced.overwrite=existing_recording_files(raced);
            check(recorder.start(raced)&&recorder.wait_idle(5000)&&recorder.status().failed&&read(raced.base.wstring()+L".sccmeta")=="CHANGED AFTER CONFIRMATION"&&read(raced.base.wstring()+L"_1.bin")=="USER SENTINEL","failed output staging preserves the complete old recording");
            RecordingOptions transaction;transaction.base=raw_folder/L"rollback";transaction.readable_base=log_folder/L"rollback";transaction.metadata="NEW";
            const auto old_raw=std::filesystem::path(transaction.base.wstring()+L"_1.bin"),old_log=std::filesystem::path(transaction.readable_base.wstring()+L"_1.log"),old_meta=std::filesystem::path(transaction.base.wstring()+L".sccmeta");
            for(const auto& path:{old_raw,old_log,old_meta})put(path);const auto unrelated=log_folder/L"rollback_notes.log";put(unrelated);
            transaction.overwrite=existing_recording_files(transaction);check(transaction.overwrite.size()==3,"only exact metadata and numeric segments are candidates");
            RecordingReplacement replacement(transaction);
            for(const auto& path:{old_raw,old_log,old_meta}){const auto stage=replacement.output(path);{std::ofstream file(stage,std::ios::binary);file<<"NEW";}replacement.created(path);}
            HANDLE reader=CreateFileW(old_log.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);check(reader!=INVALID_HANDLE_VALUE,"late no-delete reader fixture opens");
            bool rejected=false;try{replacement.commit();}catch(...){rejected=true;}const auto recovery=replacement.rollback();CloseHandle(reader);
            check(rejected&&recovery.empty()&&read(old_raw)=="USER SENTINEL"&&read(old_log)=="USER SENTINEL"&&read(old_meta)=="USER SENTINEL"&&read(unrelated)=="USER SENTINEL","partial publication rolls back earlier files and preserves unrelated names");
            RecordingOptions exact;exact.base=raw_folder/L"custom";exact.readable_base=log_folder/L"自訂.檔名";exact.first_readable=log_folder/L"自訂.檔名.log";exact.metadata="metadata";exact.rotate_bytes=1024;
            check(recorder.start(exact),"exact Unicode first filename starts");recorder.append(std::string(800,'a'),1700000000000);recorder.append(std::string(800,'b'),1700000000001);recorder.stop();
            check(recorder.wait_idle(5000)&&!recorder.status().failed&&recorder.status().log_filename==L"自訂.檔名_2.log"&&std::filesystem::exists(exact.first_readable)&&!std::filesystem::exists(log_folder/L"自訂.檔名_1.log")&&read(exact.base.wstring()+L"_1.bin")==std::string(800,'a')&&read(exact.base.wstring()+L"_2.bin")==std::string(800,'b'),"selected first log name and later numbered rotations preserve exact raw bytes");
            exact.overwrite=existing_recording_files(exact);check(recorder.start(exact),"shorter confirmed session starts");recorder.append("short\n",1700000000000);recorder.stop();
            check(recorder.wait_idle(5000)&&!recorder.status().failed&&recorder.status().log_filename==exact.first_readable.filename().wstring()&&read(exact.first_readable).find("short")!=std::string::npos&&!std::filesystem::exists(exact.base.wstring()+L"_2.bin")&&!std::filesystem::exists(exact.base.wstring()+L"_2.idx")&&!std::filesystem::exists(log_folder/L"自訂.檔名_2.log"),"confirmed shorter session cannot retain stale old tail segments");
            return 0;
        }
        if (argc != 2) throw std::runtime_error("Expected fixture directory");
        const auto project=std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path()).lexically_normal();
        const auto legacy=project/L"build"/L"verify"/L"uart-core";
        const auto requested=std::filesystem::absolute(argv[1]).lexically_normal();
        wchar_t work[32768]{};const auto length=GetEnvironmentVariableW(L"SCC_TEST_WORK",work,32768);
        const bool registered=length>0&&length<32768&&requested==std::filesystem::absolute(work).lexically_normal();
        if(!registered&&requested!=legacy)throw std::runtime_error("UART fixture requires the selected project path or registered test work");
        const auto base=registered?requested/L"uart-core":legacy;
        DisposableTestFiles::plain_path(base);const bool created=std::filesystem::create_directories(base);
        const auto fixture=base/(std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
        DisposableTestFiles::plain_path(fixture);if(!std::filesystem::create_directory(fixture))throw std::runtime_error("UART fixture must be fresh");
        FixtureDirectories directories{fixture,base,created};DisposableTestFiles owned;
        for(const auto* name:{L"session",L"rotation",L"daily",L"overflow",L"duplex"})for(int i=1;i<=2;++i)for(const auto* ext:{L".bin",L".log",L".idx"})owned.own(fixture/(std::wstring(name)+L"_"+std::to_wstring(i)+ext));
        const auto split_raw=fixture/L"split-raw",split_text=fixture/L"split-text";
        owned.own(split_raw/L"separate_1.bin");owned.own(split_raw/L"separate_1.idx");owned.own(split_raw/L"separate.sccmeta");owned.own(split_text/L"separate_1.log");owned.own(split_raw);owned.own(split_text);
        std::filesystem::create_directory(split_raw);std::filesystem::create_directory(split_text);
        owned.own(fixture/L"fixture-recipe.txt");{std::ofstream recipe(fixture/L"fixture-recipe.txt");recipe<<"uart_receive private synthetic recording/rotation/error fixtures; registered paths only; selected project boundary; cleanup after pass/fail\n";}
        MessageBuffer history;
        history.append("A\r", 0, 1700000000000); history.append("\nB\n", 0, 1700000001250);
        MessageDisplayOptions options;
        check(format_message(history.bytes(), history.offset(), history.receipts(), options).text == history.ascii(), "legacy ASCII including split CRLF");
        options.clock = MessageClock::Elapsed;
        auto formatted = format_message(history.bytes(), history.offset(), history.receipts(), options, history.origin_ms());
        check(formatted.text == L"[0.000s] A\r\n[1.250s] B\r\n", "elapsed timestamps use first-byte receipt");
        options.clock = MessageClock::Delta;
        check(format_message(history.bytes(), 0, history.receipts(), options).text.find(L"[dt +1.250s] B") != std::wstring::npos, "per-line receipt delta");
        MessageBuffer transferred; transferred.append_history(history);
        check(transferred.bytes() == history.bytes() && transferred.receipts().size() == 2 && transferred.receipts()[1].unix_ms == 1700000001250, "pending delivery preserves receipt boundaries");
        options = {}; options.encoding = MessageEncoding::Utf8;
        MessageBuffer unicode; unicode.append("\xE4\xB8", 0, 1700000000000);
        check(format_message(unicode.bytes(), 0, unicode.receipts(), options).text.empty(), "partial UTF-8 sequence waits for next chunk");
        unicode.append("\xAD\xE6\x96\x87");
        check(format_message(unicode.bytes(), 0, unicode.receipts(), options).text == L"中文", "UTF-8 split chunks decode exact Unicode");
        check(format_message("\xFFx", 0, {}, options).text == L"\\xFFx", "invalid UTF-8 is visible, not silently discarded");
        options.encoding = MessageEncoding::Ansi; options.code_page = 950;
        check(format_message("\xA4\xA4", 0, {}, options).text == L"中", "explicit ANSI Big5 decoding");
        options = {}; options.ansi_sgr = true;
        const auto color = format_message("\x1b[31mERROR\x1b[0m ok", 0, {}, options);
        check(color.text == L"ERROR ok" && color.colors.size() >= 2 && color.colors.front().color == RGB(220,70,70), "optional ANSI SGR spans");
        options.ansi_sgr = false;
        check(format_message("\x1b[31mX", 0, {}, options).text == L"\\x1B[31mX", "ANSI off preserves visible escape bytes");
        options.newline = MessageNewline::CRLF;
        check(format_message("a\rb\nc\r\n", 0, {}, options).text == L"a\\x0Db\\x0Ac\r\n", "explicit CRLF delimiter");
        options = {}; options.encoding = MessageEncoding::Hex;
        check(format_message(history.bytes(), history.offset(), history.receipts(), options).text == history.hex(), "legacy HEX offsets and bytes");
        std::wstring error;
        check(find_message_matches(L"ERR err warn", L"err", false, false, error).size() == 2 && error.empty(), "literal case-insensitive search");
        check(find_message_matches(L"ERR err warn", L"err|warn", true, true, error).size() == 2, "regex alternatives and case");
        check(find_message_matches(L"x", L"[", true, true, error).empty() && !error.empty(), "invalid regex reports error");
        check(find_message_matches(std::wstring(10000, L'a'), L"(a+)+$", true, true, error).empty() && !error.empty(), "unsafe nested regex rejected before execution");
        check(find_message_matches(std::wstring(10000, L'a'), L"(a+|b)(a+|b)X", true, true, error).empty() && !error.empty(), "ambiguous grouped quantifiers rejected before execution");
        UartRecorder recorder;
        RecordingOptions recording; recording.base = fixture / L"session"; recording.rotate_bytes = 1024;
        check(recorder.start(recording), "recording starts asynchronously");
        check(!recorder.start(recording), "duplicate start rejected");
        std::string raw("A\0\xFF\r\n", 5); recorder.append(raw, 1700000000000); recorder.append("tail", 1700000001000);
        recorder.event("disconnect", 1700000001500); recorder.stop();
        check(recorder.wait_idle(5000), "stop drains queue and flushes files");
        check(read(fixture / L"session_1.bin") == raw + "tail", "binary recording preserves exact bytes, excludes synthetic markers");
        check(read(fixture / L"session_1.idx").find("0,5,1700000000000,RX") != std::string::npos && read(fixture / L"session_1.idx").find("disconnect") != std::string::npos, "index contains receipt times and separate events");
        check(read(fixture / L"session_1.log").find("A\\x00\\xFF\r\n") != std::string::npos, "readable recording escapes binary and normalizes CRLF");
        check(recorder.status().log_filename==L"session_1.log"&&recorder.status().log_bytes==read(fixture/L"session_1.log").size(),"current readable log filename and actual byte size remain available after stop");
        check(recorder.start(recording), "restart request accepted"); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().failed && read(fixture / L"session_1.bin") == raw + "tail", "existing recording never overwritten");
        recording.base = fixture / L"rotation"; check(recorder.start(recording), "rotation fixture starts");
        recorder.append(std::string(800, 'a'), 1700000000000); recorder.append(std::string(800, 'b'), 1700000000001); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().segments == 2 && read(fixture / L"rotation_1.bin") == std::string(800, 'a') && read(fixture / L"rotation_2.bin") == std::string(800, 'b'), "size rotation and raw concatenation");
        check(recorder.status().log_filename==L"rotation_2.log"&&recorder.status().log_bytes==read(fixture/L"rotation_2.log").size()&&recorder.status().log_bytes<read(fixture/L"rotation_1.log").size()+read(fixture/L"rotation_2.log").size(),"rotated log UI size belongs to the named segment, not the aggregate");
        recording.base = fixture / L"daily"; check(recorder.start(recording), "daily fixture starts");
        recorder.append("a", 1700000000000); recorder.append("b", 1700086400000); recorder.stop();
        check(recorder.wait_idle(5000) && recorder.status().segments == 2, "PC local date rotation");
        recording.base = fixture / L"missing" / L"中文"; check(recorder.start(recording), "bad path request is asynchronous");
        recorder.stop(); check(recorder.wait_idle(5000) && recorder.status().failed && recorder.status().detail.find(L"中文") != std::wstring::npos, "filesystem failure includes Unicode path");
        recording.base = fixture / L"overflow"; check(recorder.start(recording), "overflow fixture starts");
        recorder.append(std::string(8 * 1024 * 1024 + 1, 'x'), 1700000000000);
        check(recorder.wait_idle(5000) && recorder.status().lost == 8 * 1024 * 1024 + 1 && read(fixture / L"overflow_1.idx").find("INPUT GAP") != std::string::npos, "bounded recorder stops visibly on overflow");
        recording.base = fixture / L"duplex"; check(recorder.start(recording), "RX TX recording fixture starts");
        recorder.append("before",1700000000000); recorder.append(std::string("A\0B",3),1700000000001,true); recorder.append("after\n",1700000000002); recorder.stop();
        check(recorder.wait_idle(5000) && read(fixture / L"duplex_1.bin") == "beforeafter\n" && recorder.status().bytes == 12 && recorder.status().tx_bytes == 3, "raw recording excludes TX and keeps independent byte totals");
        const auto duplex_text = read(fixture / L"duplex_1.log");
        check(duplex_text.find("[TX]") != std::string::npos && duplex_text.find("A\\x00B") != std::string::npos && duplex_text.find("[RX] after") != std::string::npos, "readable log separates interleaved TX RX directions");
        recording.base=split_raw/L"separate";recording.readable_base=split_text/L"separate";recording.metadata="metadata";
        check(recorder.start(recording),"separate readable-log destination starts");recorder.append("raw",1700000000003);recorder.stop();
        check(recorder.wait_idle(5000)&&read(split_raw/L"separate_1.bin")=="raw"&&std::filesystem::exists(split_raw/L"separate_1.idx")&&read(split_raw/L"separate.sccmeta")=="metadata"&&read(split_text/L"separate_1.log").find("raw")!=std::string::npos&&!std::filesystem::exists(split_text/L"separate_1.bin"),"raw/index/metadata stay together while readable log uses its selected folder");
        owned.finish();std::cout << "PASS: UART receive core and exact owned fixture cleanup\n"; return 0;
    } catch (const std::exception& ex) { std::cerr << "FAIL: " << ex.what() << '\n'; return 1; }
}
