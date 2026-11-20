#pragma once
#include "message_buffer.h"
#include "line_filter.h"
struct MessageLensConfig {std::string include,exclude;bool regex=false;unsigned before=0,after=0;};
struct MessageLensRow {uint64_t offset=0;size_t count=0,start=0,end=0;int64_t receipt=0;bool match=false;};
struct MessageLensView {FormattedMessage formatted;std::vector<MessageLensRow> rows;size_t total=0,limited=0;};
class MessageLens {
public:
    bool configure(const MessageLensConfig&,std::string&);
    const MessageLensConfig& config()const{return config_;}
    const MessageLensView& update(const MessageBuffer&,MessageDisplayOptions,bool rich=false);
    void reset(){cached_.clear();offset_=UINT64_MAX;}
private:
    struct Cached {uint64_t offset=0;size_t count=0;LineFilter::Decision decision=LineFilter::IncludeMiss;int64_t receipt=0;FormattedMessage text;};
    MessageLensConfig config_;LineFilter filter_,exclusion_;uint64_t offset_=UINT64_MAX;size_t size_=0;
    MessageDisplayOptions options_;bool options_valid_=false,rich_=false;
    std::vector<Cached> cached_;MessageLensView view_;
};
struct MessageSearchRow {uint64_t offset=0;size_t count=0,hits=0;int64_t receipt=0;std::wstring text;};
struct MessageSearchResults {std::vector<MessageSearchRow> rows;std::vector<size_t> buckets;size_t matches=0,skipped_rows=0;int64_t begin_ms=0,end_ms=0;bool limited=false,timed=false;std::wstring error,frequency;};
MessageSearchResults search_retained_messages(const MessageBuffer&,MessageDisplayOptions,const std::wstring&,bool regex,bool match_case);
