#include "text_telemetry.h"
#include "settings_fields.h"
std::wstring pack_text_settings(const std::vector<std::wstring>& fields){
    if(fields.size()!=9)throw std::runtime_error("Invalid text settings field count.");
    auto encoded=pack_settings(fields);if(fields[4]==L"JSON"&&fields[5].rfind(L"ptr:",0)==0)encoded[1]=L'2';return encoded;
}
std::vector<std::wstring> unpack_text_settings(const std::wstring& input){
    auto encoded=input;const bool extended=encoded.rfind(L"v2:",0)==0;if(extended)encoded[1]=L'1';auto fields=unpack_settings(encoded,9);
    if(extended&&(fields[4]!=L"JSON"||fields[5].rfind(L"ptr:",0)!=0))throw std::runtime_error("Text settings v2 requires explicit JSON Pointer selections.");return fields;
}
bool TextTelemetry::configure(const TextTelemetryConfig& c,std::string& error){
    if(!c.extraction.time_field.empty()||c.extraction.time_group){error="Port B text charts use PC receipt time; clear device time fields/groups.";return false;}
    FieldExtractor extractor;LineFilter filter;
    if(!extractor.configure(c.extraction,error)||!filter.configure(c.include,c.exclude,c.regex,error))return false;
    config_=c;extractor_=std::move(extractor);filter_=std::move(filter);interrupt();skipped_=0;diagnostics_.reset();return true;
}
void TextTelemetry::interrupt(){lines_.clear_partial();pending_.clear();bytes_=0;generation_=UINT64_MAX;}
void TextTelemetry::receive(const TrafficRecord& record){
    if(!config_.enabled||record.tx)return;
    if(generation_!=record.generation){if(generation_!=UINT64_MAX){skipped_+=pending_.size();interrupt();}generation_=record.generation;}
    for(auto& line:lines_.feed(record.bytes,record.sample_seconds)){bytes_+=line.text.size();const auto time=line.seconds;pending_.push_back({std::move(line),time});}
    while(pending_.size()>2000||bytes_>4*1024*1024){bytes_-=pending_.front().first.text.size();pending_.pop_front();++skipped_;}
}
void TextTelemetry::drain(double now){for(auto& line:lines_.drain(now)){bytes_+=line.text.size();const auto time=line.seconds;pending_.push_back({std::move(line),time});}while(pending_.size()>2000||bytes_>4*1024*1024){bytes_-=pending_.front().first.text.size();pending_.pop_front();++skipped_;}}
std::vector<TextTelemetryFrame> TextTelemetry::take(size_t limit){
    std::vector<TextTelemetryFrame> out;limit=std::min<size_t>(64,limit);
    for(size_t i=0;i<limit&&!pending_.empty();++i){auto item=std::move(pending_.front());pending_.pop_front();bytes_-=item.first.text.size();
        const auto decision=filter_.evaluate(item.first.text);
        if(decision!=LineFilter::Accept){diagnostics_.observe(decision==LineFilter::Excluded?PipelineDiagnostics::Excluded:decision==LineFilter::WorkLimit?PipelineDiagnostics::FilterLimit:PipelineDiagnostics::IncludeMiss,item.first.text);continue;}
        auto fields=extractor_.extract(item.first.text);
        if(fields.limited||!fields.error.empty()){++skipped_;diagnostics_.observe(PipelineDiagnostics::ParserError,item.first.text);continue;}
        diagnostics_.observe(fields.values.empty()?PipelineDiagnostics::NoValues:PipelineDiagnostics::Accepted,item.first.text);
        if(!fields.values.empty())out.push_back({std::move(item.first.text),std::move(fields.values),item.second,generation_});
    }return out;
}
