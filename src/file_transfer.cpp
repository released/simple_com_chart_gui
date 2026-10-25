#include "file_transfer.h"
#include <algorithm>
#include <stdexcept>
std::shared_ptr<FileTransfer> FileTransfer::prepare(const std::filesystem::path& path,FileTransferOptions options,const std::atomic<bool>& cancel) {
    if(options.chunk<1 || options.chunk>4096 || options.delay_ms>60000 || options.newline<0 || options.newline>3) throw std::runtime_error("Invalid file TX chunk/delay/newline settings.");
    auto result=std::make_shared<FileTransfer>();result->options_=options;result->file_=std::make_shared<ReplayFile>(path);
    if(!result->size() || result->size()>64ULL*1024*1024*1024) throw std::runtime_error("File TX size must be 1 byte..64 GiB.");
    if(options.text) {
        unsigned remaining=0,codepoint=0,minimum=0;
        for(uint64_t at=0;at<result->size();) {
            const auto bytes=result->file_->read(at,static_cast<size_t>(std::min<uint64_t>(65536,result->size()-at)),cancel);at+=bytes.size();
            for(const unsigned char c:bytes) {
                if(remaining) {if((c&0xc0)!=0x80) throw std::runtime_error("Text file is not valid UTF-8; choose Binary for unchanged bytes.");codepoint=(codepoint<<6)|(c&63);if(!--remaining && (codepoint<minimum || codepoint>0x10ffff || (codepoint>=0xd800&&codepoint<=0xdfff))) throw std::runtime_error("Invalid UTF-8 code point.");}
                else if(c<128) continue;else if(c>=0xc2 && c<=0xdf) {remaining=1;minimum=0x80;codepoint=c&31;}else if(c>=0xe0 && c<=0xef) {remaining=2;minimum=0x800;codepoint=c&15;}else if(c>=0xf0 && c<=0xf4) {remaining=3;minimum=0x10000;codepoint=c&7;}else throw std::runtime_error("Text file is not valid UTF-8.");
            }
        }
        if(remaining) throw std::runtime_error("Incomplete UTF-8 text suffix.");
    }
    return result;
}
FileTransferChunk FileTransfer::next(const std::atomic<bool>& cancel) {
    if(cancel) throw std::runtime_error("Cancelled");
    while(pending_.size()<options_.chunk && offset_<size()) {
        const auto bytes=file_->read(offset_,static_cast<size_t>(std::min<uint64_t>(4096,size()-offset_)),cancel);offset_+=bytes.size();
        if(!options_.text || !options_.newline) pending_+=bytes;
        else for(const char c:bytes) {
            if(c=='\n' && after_cr_) {after_cr_=false;continue;}after_cr_=c=='\r';
            if(c=='\r'||c=='\n') pending_+=options_.newline==1?"\n":options_.newline==2?"\r\n":"\r";else pending_+=c;
        }
    }
    FileTransferChunk result;const auto count=std::min(options_.chunk,pending_.size());result.bytes=pending_.substr(0,count);pending_.erase(0,count);result.source_read=offset_;result.end=offset_==size()&&pending_.empty();return result;
}
