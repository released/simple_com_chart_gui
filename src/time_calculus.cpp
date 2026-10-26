#include "time_calculus.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
CalculusResult time_calculus(const CaptureSnapshot& data,const std::string& key,unsigned source,bool integral,double gap,const std::atomic<bool>& cancel){
    if(source>1||!std::isfinite(gap)||gap<=0||gap>86400||data.rows.size()>10000)throw std::runtime_error("Calculus source/gap/retained-row limit is invalid.");
    const auto found=std::find(data.keys.begin(),data.keys.end(),key);if(found==data.keys.end())throw std::runtime_error("Selected key is not retained.");const auto col=static_cast<size_t>(found-data.keys.begin());
    CalculusResult out;std::ostringstream csv;csv<<std::setprecision(17)<<"Row,Time_s,Result,Segment,Source\r\n";
    bool previous=false;double before=0,time=0;uint64_t generation=0;size_t segment=0,valid=0,breaks=0;long double sum=0;
    for(const auto& row:data.rows){if(cancel)throw std::runtime_error("Calculus cancelled.");if(row->numeric_source!=source)continue;
        CalculusPoint p;p.seconds=row->seconds;p.row=row->sequence;double v=0;const bool has=CaptureStore::value(*row,col,v)&&std::isfinite(v)&&std::isfinite(row->source_seconds)&&std::isfinite(row->seconds);
        const double dt=row->source_seconds-time;
        if(!has){if(previous)++breaks;previous=false;}
        else if(!previous||row->generation!=generation||dt<=0||dt>gap){if(previous)++breaks;++segment;sum=0;p.value=0;p.valid=integral;previous=true;}
        else {const long double delta=static_cast<long double>(v)-before;
            const long double result=integral?sum+(static_cast<long double>(before)*.5L+static_cast<long double>(v)*.5L)*dt:
                std::isfinite(delta)?delta/dt:static_cast<long double>(v)/dt-static_cast<long double>(before)/dt;
            if(std::isfinite(result)&&std::abs(result)<=std::numeric_limits<double>::max()){p.value=static_cast<double>(result);p.valid=true;if(integral)sum=result;}
            else {previous=false;++breaks;}}
        if(has&&previous){before=v;time=row->source_seconds;generation=row->generation;}p.segment=segment;if(p.valid)++valid;
        out.points.push_back(p);csv<<p.row<<','<<p.seconds<<',';if(p.valid)csv<<p.value;csv<<','<<p.segment<<','<<(source?'B':'A')<<"\r\n";
    }
    out.csv=csv.str();out.report=std::string(integral?"Integral (raw value * seconds)":"Derivative (raw value / second)")+" | key "+key+" | "+std::to_string(valid)+" valid points / "+std::to_string(segment)+" segments / "+std::to_string(breaks)+" gaps.\r\nClock: retained source_seconds (PC receipt, configured device time, or explicitly synthetic replay). Not wire timing. Other-source rows ignored; missing/nonfinite values, generation changes and nonpositive/excessive time steps break segments.\r\n"+(integral?"Each valid segment starts at zero by definition; trapezoidal integration assumes a piecewise-linear signal only within that segment.":"First point of every segment is n/a; differences use adjacent valid samples only.");return out;
}
