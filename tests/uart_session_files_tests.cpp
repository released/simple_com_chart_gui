#include "session_fixture.h"
#include "session_manifest.h"
#include "transaction_analysis.h"
#include <cmath>
void session_check(bool,const char*);
namespace {template<class F> bool refuses(F work){try{work();}catch(...){return true;}return false;}}
void session_file_tests(){
    SessionFixture f;f.write(L"input.bin","ABCD");f.write(L"input.idx","SCC-RX-INDEX,1\r\noffset,length,unix_ms,event\r\n0,2,1000,RX\r\n2,2,1100,RX\r\n");
    CaptureMetadata meta;meta.version="1.0.1.029";meta.source="Current";
    f.write(L"sha.bin","abc");ReplayFile sha(f.root/L"sha.bin");session_check(file_sha256(sha,f.cancel)=="BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD","Windows native SHA256 matches independent known vector");
    const auto manifest=f.file(L"capture.sccsession");auto built=create_session_manifest(f.root/L"input.bin",manifest,meta,false,false,100,f.cancel);
    auto verified=verify_session_manifest(manifest,f.cancel);session_check(verified.files.size()==2&&verified.source==L"input.bin"&&verified.metadata.source=="Current","manifest verifies relative bin/index and explicit current settings");
    const auto missing_log=f.file(L"missing-log.sccsession");std::string missing_detail;
    try{create_session_manifest(f.root/L"input.bin",missing_log,meta,false,true,100,f.cancel);}catch(const std::exception& e){missing_detail=e.what();}
    session_check(!std::filesystem::exists(missing_log)&&missing_detail.find("0 found, 1 missing")!=missing_detail.npos&&missing_detail.find("input.log")!=missing_detail.npos,"requested missing readable logs are listed before any manifest is created");
    f.write(L"input.log","readable RX\r\n");const auto with_log=f.file(L"with-log.sccsession");
    const auto logged=create_session_manifest(f.root/L"input.bin",with_log,meta,false,true,100,f.cancel);
    session_check(logged.files.size()==3&&verify_session_manifest(with_log,f.cancel).files.size()==3,"requested adjacent readable log is listed and hash-verified");
    session_check(std::filesystem::file_size(f.root/L"input.bin")==4&&refuses([&]{create_session_manifest(f.root/L"input.bin",manifest,meta,false,false,100,f.cancel);}),"manifest never copies raw bytes or overwrites an existing destination");
    uart_write_file(f.root/L"input.bin","ABCE",true,f.cancel);session_check(refuses([&]{verify_session_manifest(manifest,f.cancel);}),"same-size payload corruption is detected by SHA256");uart_write_file(f.root/L"input.bin","ABCD",true,f.cancel);
    auto unsafe=built;unsafe.files[0].leaf=L"..\\escape.bin";session_check(refuses([&]{decode_session_manifest(encode_session_manifest(unsafe));}),"manifest rejects parent traversal before referenced I/O");
    unsafe=built;unsafe.files.push_back(unsafe.files.front());session_check(refuses([&]{decode_session_manifest(encode_session_manifest(unsafe));}),"duplicate file references rejected");
    const std::atomic<bool> cancelled{true};session_check(refuses([&]{verify_session_manifest(manifest,cancelled);}),"cancelled verification stops without input changes");
    const auto no_manifest=f.file(L"cancel.sccsession");session_check(refuses([&]{create_session_manifest(f.root/L"input.bin",no_manifest,meta,false,false,100,cancelled);})&&!std::filesystem::exists(no_manifest),"cancelled creation leaves no manifest output");
    f.write(L"input.sccmeta",encode_capture_metadata(meta.profile,"1.0.1.029","A")+"Changed=1\r\n");const auto captured=f.file(L"captured.sccsession");built=create_session_manifest(f.root/L"input.bin",captured,meta,true,false,100,f.cancel);verified=verify_session_manifest(captured,f.cancel);
    session_check(verified.files.size()==3&&verified.metadata.source=="A"&&verified.metadata.changed,"recording-start sidecar preserved and later changes disclosed");
    session_check(refuses([&]{ReplayFile lock(manifest);uart_write_file(manifest,"OVERWRITE",true,f.cancel);}),"completed input reader holds files against modification");
    const auto source=uart_read_file(f.root/L"input.bin",16,f.cancel);session_check(source=="ABCD","all manifest operations preserve source bytes");
}
void transaction_tests(){
    std::atomic<bool> cancel{false};TrafficBatch b;TransactionOptions o;o.framing.mode=TrafficFraming::LF;o.request_prefix="Q";o.reply_prefix="R";o.timeout=1;
    b.records={{1,1000,true,"Q",10,1},{2,1010,true,std::string("\x01\n",2),10.01,1},{3,1020,false,std::string("R\x01\n",3),10.02,1}};
    auto r=analyze_transactions(b,o,10.03,cancel);session_check(r.rows.size()==1&&r.rows[0].status=="RULE MATCH"&&std::abs(r.rows[0].milliseconds-10)<1e-6,"passive split-request/reply matching uses actual completion clocks");
    session_check(r.rows[0].request->first_sequence==1&&r.rows[0].request->first_offset==0&&r.rows[0].request->sequence==2&&r.rows[0].request->last_offset==2&&transaction_detail(r,0,true).find("52010A")!=std::string::npos,"request/reply inspect retains exact cross-chunk traffic spans and bytes");
    b.records={{1,1000,true,std::string("Q\x01\n",3),10,1},{2,1010,true,std::string("Q\x01\n",3),10.01,1},{3,1020,false,std::string("R\x01\n",3),10.02,1}};r=analyze_transactions(b,o,10.03,cancel);
    session_check(r.rows.size()==3&&r.rows[0].status=="AMBIGUOUS ID OVERLAP"&&r.rows[2].status=="AMBIGUOUS REPLY","overlapping IDs do not invent a request/reply pairing");
    b.records.resize(1);r=analyze_transactions(b,o,12,cancel);session_check(r.rows[0].status=="TIMEOUT","quiet snapshot expiry produces observable timeout");
    b.records.push_back({2,1020,false,std::string("R\x01\n",3),10.02,2});r=analyze_transactions(b,o,10.03,cancel);session_check(r.rows.size()==2&&r.rows[0].status=="GENERATION ENDED"&&r.rows[1].status=="ORPHAN REPLY","reconnect never pairs different generations");
    b.records.back().bytes="R";r=analyze_transactions(b,o,10.03,cancel);session_check(r.rows.size()==1&&r.rows[0].status=="GENERATION ENDED","reconnect closes pending requests even before a new complete frame");
    b.records={{1,1000,true,std::string("Q\x01\n",3),10.1,1},{2,1020,false,std::string("R\x01\n",3),10,1}};r=analyze_transactions(b,o,10.2,cancel);session_check(r.rows[0].status=="PC CLOCK ORDER UNCERTAIN","uncertain host ordering does not fabricate nonnegative latency");
    b.expired=42;o.source=1;r=analyze_transactions(b,o,10.2,cancel);session_check(r.report.find("history expired 42")!=std::string::npos&&r.csv.find("B,0,")!=std::string::npos,"port ownership/history expiry remain visible in reports");
    TrafficStore tiny(3,8);tiny.append("ABCDE",1000,true,10,1);auto tail=tiny.snapshot();session_check(tail.records[0].first_offset==2&&tail.records[0].last_offset==5,"retained cropped chunk offsets refer to original driver bytes");
    const std::atomic<bool> cancelled{true};session_check(refuses([&]{analyze_transactions(b,o,10,cancelled);}),"passive analysis cancellation never acts on transport");
}
