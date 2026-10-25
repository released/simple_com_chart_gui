#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include <cmath>

void CMainDialog::reset_offline_view() {
    reset_binary_replay();
    model_.reset(); capture_.reset(static_cast<size_t>(model_.get_max_channels())); data_page_.reset(); channel_panel_.reset();
    snapshot_=false; plot_view_.set_frozen(false); plot_view_.reset_visual(); message_page_.reset();
    ++replay_epoch_;replay_io_.cancel();source_lines_.clear_partial();
    extracted_clock_=false;
    ::SetWindowTextW(btn_snapshot_,L"Snapshot"); ::SendMessageW(btn_snapshot_,BM_SETCHECK,BST_UNCHECKED,0);
    { std::lock_guard<std::mutex> lock(pending_mutex_); pending_=PendingData{}; pending_dropped_=0; }
}
void CMainDialog::replay_request(const AdvancedTools::Request& request) {
    const auto tell=[&](const std::wstring& value) { advanced_tools_.notice(AdvancedTools::Replay,value); };
    const auto now=GetTickCount64();
    if (request.action==5) {
        if (!replay_.data() && !replay_load_.busy()) { tell(L"No offline data to unload; live capture unchanged."); return; }
        replay_load_.cancel();replay_io_.cancel();
        if (!advanced_idle()) { tell(L"Loader cancellation requested; live capture unchanged. Disconnect before unloading an offline view."); return; }
        replay_.load({}); reset_offline_view(); message_page_.set_offline_source(false,false); message_page_.refresh(); advanced_tools_.state(AdvancedTools::Replay,L"Unloaded; no offline data"); tell(L"Offline data unloaded; view cleared. No port opened."); set_left_status(L"COM: Disconnected"); return;
    }
    if (request.action==3) { replay_.pause(now); return; }
    if (request.fields.size()!=7) return;
    if (!advanced_idle() || recorder_.status().active || exporting_ || profile_job_.busy() || (workbench_&&workbench_->any_binary())) { tell(L"Disconnect both ports, disable Workbench binary charts and stop recording/export/profile work before offline operations."); return; }
    double number=0;
    if (request.action==1) {
        if (replay_load_.busy()) { tell(L"Load already pending; use Unload / cancel."); return; }
        if (!uart_number(request.fields[1],1,60000,number) || std::floor(number)!=number) { tell(L"Synthetic interval must be an integer 1..60000 ms."); return; }
        const auto path=std::filesystem::path(request.fields[0]); const auto interval=static_cast<unsigned>(number);
        if (!capture_.rows().empty() || !message_page_.buffer().bytes().empty()) {
            const auto confirm=uart_notice_;
            if (confirm(advanced_tools_.GetSafeHwnd(),L"Load offline data and replace the current capture / Messages on success?\n\nUse Save As first if the existing view must be kept. A failed/cancelled load retains it. No bytes will be sent to UART.",L"Load offline capture",MB_OKCANCEL|MB_ICONWARNING)!=IDOK) return;
            if (!::IsWindow(m_hWnd) || closing_serial_ || !advanced_idle() || recorder_.status().active || exporting_ || profile_job_.busy()) return;
        }
        replay_.pause(now);
        if (!replay_load_.start([path,interval](const std::atomic<bool>& cancel) { return load_replay(path,interval,cancel); })) tell(L"Cannot start replay loader");
        else tell(L"Building paged index in background; up to 64 GiB / 50 million records / 4096 ordered segments. Prior view retained until success.");
    } else if (request.action==2) {
        if (replay_load_.busy() || replay_io_.busy() || !replay_.data()) { tell(L"Load a file first; wait for pending file work."); return; }
        if (!uart_number(request.fields[2],0.1,100,number)) { tell(L"Speed must be 0.1..100 x."); return; } replay_.play(now,number);
    } else if(request.action==6) {
        if(!replay_.data() || replay_io_.busy() || replay_load_.busy()) {tell(L"Load a file and wait for pending reads/search.");return;}
        TxEntry pattern;pattern.text=request.fields[4];pattern.hex=request.fields[5]==L"HEX";std::string bytes;std::wstring error;
        if(!make_tx_payload(pattern,bytes,error)) {tell(error);return;}
        replay_.pause(now);const auto data=replay_.data();const auto first=replay_.position();replay_io_epoch_=replay_epoch_;
        replay_io_.start([data,first,bytes](const std::atomic<bool>& cancel){ReplayBatch batch;batch.search=true;batch.found=replay_find(*data,first,bytes,cancel);return batch;});tell(L"Searching raw bytes across ordered segments in background...");
    } else if(request.action==7) {
        if(!replay_.data() || replay_io_.busy() || !uart_number(request.fields[6],0,1e15,number)) {tell(L"Load data, wait for file work and enter finite non-negative file seconds.");return;}
        replay_.pause(now);const auto data=replay_.data();replay_io_epoch_=replay_epoch_;
        replay_io_.start([data,number](const std::atomic<bool>& cancel){if(cancel) throw std::runtime_error("Cancelled");ReplayBatch batch;batch.search=true;batch.time_seek=true;batch.found=replay_seek_time(*data,number);return batch;});tell(L"Seeking file time in background...");
    } else if (request.action==4) {
        if (replay_load_.busy() || !replay_.data() || !uart_number(request.fields[3],0,static_cast<double>(replay_.data()->records.size()),number) || std::floor(number)!=number) { tell(L"Seek must be a valid 0-based record index (or end)."); return; }
        replay_.pause(now); reset_offline_view(); replay_.seek(static_cast<size_t>(number),now); tell(L"Seek paused at selected record; parser/message/capture view reset. Click Play to continue.");
    }
}
void CMainDialog::feed_replay(size_t first,size_t count) {
    const auto data=replay_.data();if(!data || replay_io_.busy()) return;
    replay_io_epoch_=replay_epoch_;
    const auto binary=replay_binary_enabled_?replay_binary_state_:nullptr;
    replay_io_.start([data,first,count,binary](const std::atomic<bool>& cancel) {
        ReplayBatch batch;batch.first=first;
        for(size_t i=first;i<first+count;++i){batch.bytes.push_back(data->read_record(i,cancel));if(binary){auto frames=binary->feed(data->records[i],batch.bytes.back());for(auto& frame:frames)batch.frames.push_back(std::move(frame));}}
        if(binary){batch.crc_errors=binary->decoder.checksum_errors();batch.discarded=binary->decoder.discarded();}
        return batch;
    });
}
void CMainDialog::poll_replay_io() {
    drain_binary_replay();
    ReplayBatch batch;std::wstring error;
    if(!replay_io_.take(batch,error)) return;
    if(replay_io_epoch_!=replay_epoch_) return;
    const auto data=replay_.data();if(!data) return;
    if(!error.empty()) {replay_.pause(GetTickCount64());advanced_tools_.notice(AdvancedTools::Replay,error+L" No automatic retry; Seek/Load to recover.");return;}
    if(batch.search) {
        if(batch.found==data->records.size() && !batch.time_seek) {advanced_tools_.notice(AdvancedTools::Replay,L"No match from the current record to end; seek to 0 to search again.");return;}
        replay_.pause(GetTickCount64());reset_offline_view();replay_.seek(batch.found,GetTickCount64());
        advanced_tools_.set_field(AdvancedTools::Replay,3,std::to_wstring(batch.found));
        advanced_tools_.notice(AdvancedTools::Replay,std::wstring(batch.time_seek?L"Time seek: record ":L"Found in record ")+std::to_wstring(batch.found)+L"; paused. Click Play to inspect.");return;
    }
    std::lock_guard<std::mutex> lock(pending_mutex_);
    for(size_t n=0;n<batch.bytes.size();++n) {
        const auto index=batch.first+n;const auto record=data->records[index];const auto& bytes=batch.bytes[n];
        pending_.raw.append(bytes,0,record.unix_ms);
        if(replay_binary_enabled_)source_lines_.skip_bytes(bytes.size());
        else {
            for(auto& line:source_lines_.feed(bytes)) pending_.lines.push_back({std::move(line.text),record.seconds+1,line.offset,line.count});
            if(index+1==data->records.size()) for(auto& line:source_lines_.finish()) pending_.lines.push_back({std::move(line.text),record.seconds+1,line.offset,line.count});
        }
        oversized_lines_=source_lines_.skipped();
        pending_.source_end=source_lines_.end();
    }
    replay_crc_errors_=batch.crc_errors;replay_discarded_=batch.discarded;
    for(auto& frame:batch.frames)replay_frames_.push_back(std::move(frame));
}
