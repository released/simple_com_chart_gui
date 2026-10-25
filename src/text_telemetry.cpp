#include "text_telemetry.h"
bool TextTelemetry::configure(const TextTelemetryConfig& c,std::string& error){
    if(!c.extraction.time_field.empty()||c.extraction.time_group){error="Port B text charts use PC receipt time; clear device time fields/groups.";return false;}
    FieldExtractor extractor;LineFilter filter;
    if(!extractor.configure(c.extraction,error)||!filter.configure(c.include,c.exclude,c.regex,error))return false;
    config_=c;extractor_=std::move(extractor);filter_=std::move(filter);interrupt();skipped_=0;return true;
}
void TextTelemetry::interrupt(){lines_.clear_partial();pending_.clear();bytes_=0;generation_=UINT64_MAX;}
void TextTelemetry::receive(const TrafficRecord& record){
    if(!config_.enabled||record.tx)return;
    if(generation_!=record.generation){if(generation_!=UINT64_MAX){skipped_+=pending_.size();interrupt();}generation_=record.generation;}
    for(auto& line:lines_.feed(record.bytes)){bytes_+=line.text.size();pending_.push_back({std::move(line),record.sample_seconds});}
    while(pending_.size()>2000||bytes_>4*1024*1024){bytes_-=pending_.front().first.text.size();pending_.pop_front();++skipped_;}
}
std::vector<TextTelemetryFrame> TextTelemetry::take(size_t limit){
    std::vector<TextTelemetryFrame> out;limit=std::min<size_t>(64,limit);
    for(size_t i=0;i<limit&&!pending_.empty();++i){auto item=std::move(pending_.front());pending_.pop_front();bytes_-=item.first.text.size();
        if(!filter_.matches(item.first.text))continue;auto fields=extractor_.extract(item.first.text);
        if(fields.limited||!fields.error.empty()){++skipped_;continue;}
        if(!fields.values.empty())out.push_back({std::move(item.first.text),std::move(fields.values),item.second});
    }return out;
}
