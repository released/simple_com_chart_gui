#include "run_analysis.h"
#include "uart_offline.h"
#include "replay_binary.h"
#include "command_panel.h"
#include "source_mapping.h"
#include "message_format.h"
#include "line_filter.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <set>
namespace {
size_t column(const CaptureSnapshot& data,const std::string& key){const auto it=std::find(data.keys.begin(),data.keys.end(),key);return static_cast<size_t>(it-data.keys.begin());}
double scalar(const std::string& text){double value=0;if(!uart_number(uart_wide(text),-std::numeric_limits<double>::max(),std::numeric_limits<double>::max(),value))throw std::runtime_error("Invalid finite CSV number.");return value;}
std::vector<std::pair<double,double>> series(const RecordedRun& run,const std::string& key){std::vector<std::pair<double,double>> out;const auto index=column(run.data,key);if(index==run.data.keys.size())return out;for(const auto& row:run.data.rows){double value=0;if(CaptureStore::value(*row,index,value)&&std::isfinite(value))out.push_back({row->seconds,value});}return out;}
}
RecordedRun load_recorded_run(const std::filesystem::path& path,const RunOptions& options,const std::atomic<bool>& cancel){
    CaptureStore capture;capture.reset(1024);auto extension=path.extension().wstring();std::transform(extension.begin(),extension.end(),extension.begin(),towlower);RecordedRun run;run.name=path.filename().u8string();
    const auto append=[&](const std::string& raw,const std::unordered_map<std::string,double>& values,double seconds){if(values.empty())return;capture.append(raw,values,seconds);if(capture.dropped())throw std::runtime_error("Comparison import exceeds 10000 numeric rows / 32 MiB; choose a smaller recording.");};
    if(extension==L".csv"||extension==L".cvs"){
        auto text=uart_read_file(path,32*1024*1024,cancel);if(text.rfind("\xEF\xBB\xBF",0)==0)text.erase(0,3);std::vector<std::vector<std::string>> rows;std::string error;
        if(!uart_csv_table(text,rows,error,32*1024*1024,10001,1026)||rows.empty()||rows[0].size()<3||rows[0][0]!="Row"||rows[0][1]!="Time (s)")throw std::runtime_error("Use SimpleComChart CSV export (Row, Time (s), numeric fields). "+error);
        const auto& keys=rows[0];std::set<std::string> unique;for(size_t n=2;n<keys.size();++n){if(keys[n].empty()||keys[n].size()>160||!unique.insert(keys[n]).second)throw std::runtime_error("Invalid/duplicate CSV field name.");uart_wide(keys[n]);}
        double previous=-1;for(size_t i=1;i<rows.size();++i){if(cancel)throw std::runtime_error("Cancelled");if(rows[i].size()!=keys.size())throw std::runtime_error("CSV column count mismatch.");const auto seconds=scalar(rows[i][1]);if(seconds<0||seconds<previous||seconds>1e15)throw std::runtime_error("CSV time must be nonnegative/nondecreasing.");previous=seconds;std::unordered_map<std::string,double> values;for(size_t n=2;n<keys.size();++n)if(!rows[i][n].empty())values[keys[n]]=scalar(rows[i][n]);append("",values,seconds+1);}
        run.provenance="CSV export clock, normalized to first numeric row; raw numeric values";
    }else{
        const auto data=load_replay(path,options.interval_ms,cancel,64*1024*1024);
        FieldExtractor extractor;LineFilter filter;std::string error;if(!extractor.configure(options.extraction,error)||!filter.configure(options.include,options.exclude,options.regex,error))throw std::runtime_error(error);SourceLineTracker lines;ReplayBinary binary;if(options.binary&&!binary.decoder.configure(options.schema,error))throw std::runtime_error(error);double previous=-1;uint64_t generation=UINT64_MAX;
        const auto accept=[&](const SourceLine& line,double receipt){if(!filter.matches(line.text))return;const auto fields=extractor.extract(line.text);if(fields.limited||!fields.error.empty())throw std::runtime_error("Extraction failed/limited during comparison import.");const auto time=fields.seconds?*fields.seconds:receipt;if(time<previous)throw std::runtime_error("Backward extracted time in comparison input.");if(!fields.values.empty()){append(line.text,fields.values,time);previous=time;}};
        for(size_t i=0;i<data->records.size();++i){if(cancel)throw std::runtime_error("Cancelled");const auto record=data->records[i];const auto bytes=data->read_record(i,cancel);
            if(options.binary){const auto frames=binary.feed(record,bytes);for(const auto& frame:frames)append(uart_hex(frame.bytes),{frame.values.begin(),frame.values.end()},frame.sample_seconds);}
            else {if(generation!=record.generation){lines.clear_partial();generation=record.generation;}for(const auto& line:lines.feed(bytes))accept(line,record.seconds+1);if(i+1==data->records.size())for(const auto& line:lines.finish())accept(line,record.seconds+1);if(lines.skipped())throw std::runtime_error("Oversized text line; import stopped.");}
        }
        run.provenance=message_utf8(data->provenance)+(options.binary?"; binary schema snapshot":"; text extraction snapshot");
    }
    if(capture.rows().empty())throw std::runtime_error("No numeric rows; review extraction / binary schema.");run.data=capture.snapshot();return run;
}
std::vector<std::pair<double,double>> xy_pairs(const CaptureSnapshot& data,const std::string& x,const std::string& y,size_t limit){
    std::vector<std::pair<double,double>> out;const auto a=column(data,x),b=column(data,y);if(a==data.keys.size()||b==data.keys.size())return out;
    limit=std::min<size_t>(2000,limit);for(auto row=data.rows.rbegin();row!=data.rows.rend()&&out.size()<limit;++row){double xv=0,yv=0;if(CaptureStore::value(**row,a,xv)&&CaptureStore::value(**row,b,yv)&&std::isfinite(xv)&&std::isfinite(yv))out.push_back({xv,yv});}std::reverse(out.begin(),out.end());return out;
}
RunComparison compare_runs(const RecordedRun& a,const RecordedRun& b,const std::string& key,double shift,double max_age,double tolerance){
    if(!std::isfinite(shift)||!std::isfinite(max_age)||!std::isfinite(tolerance)||max_age<0||tolerance<0)throw std::runtime_error("Invalid comparison alignment/tolerance.");
    const auto av=series(a,key),bv=series(b,key);RunComparison result;size_t i=0,j=0;
    while(i<av.size()&&j<bv.size()){const long double target=av[i].first,difference=static_cast<long double>(bv[j].first)+shift-target;
        if(std::abs(difference)<=max_age){while(j+1<bv.size()&&std::abs(static_cast<long double>(bv[j+1].first)+shift-target)<std::abs(static_cast<long double>(bv[j].first)+shift-target)){++j;++result.unmatched_b;}
            const bool pass=std::abs(static_cast<long double>(bv[j].second)-av[i].second)<=tolerance;result.matches.push_back({av[i].first,bv[j].first,av[i].second,bv[j].second,pass});if(pass)++result.passed;++i;++j;
        }else if(difference<0){++result.unmatched_b;++j;}else{++result.unmatched_a;++i;}
    }result.unmatched_a+=av.size()-i;result.unmatched_b+=bv.size()-j;return result;
}
std::string comparison_csv(const RunComparison& result){std::ostringstream out;out<<std::setprecision(17)<<"Time_A_s,Time_B_s,A,B,B_minus_A,Result\r\n";for(const auto& match:result.matches)out<<match.time_a<<','<<match.time_b<<','<<match.a<<','<<match.b<<','<<(static_cast<long double>(match.b)-match.a)<<','<<(match.pass?"PASS":"FAIL")<<"\r\n";out<<"\r\nUnmatched_A,"<<result.unmatched_a<<"\r\nUnmatched_B,"<<result.unmatched_b<<"\r\n";return out.str();}
