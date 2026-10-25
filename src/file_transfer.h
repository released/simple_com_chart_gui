#pragma once
#include "replay_storage.h"
struct FileTransferOptions {bool text=false;int newline=0;size_t chunk=256;unsigned delay_ms=20;};
struct FileTransferChunk {std::string bytes;uint64_t source_read=0;bool end=false;};
class FileTransfer {
public:
    static std::shared_ptr<FileTransfer> prepare(const std::filesystem::path&,FileTransferOptions,const std::atomic<bool>& cancel);
    FileTransferChunk next(const std::atomic<bool>& cancel);
    uint64_t size() const {return file_->size();}
    unsigned delay() const {return options_.delay_ms;}
private:
    std::shared_ptr<ReplayFile> file_;
    FileTransferOptions options_;
    uint64_t offset_=0;
    std::string pending_;
    bool after_cr_=false;
};
