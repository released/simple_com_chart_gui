#include <windows.h>
#include <gdiplus.h>
#include "plot_envelope.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
namespace {
std::vector<ChannelSample> legacy(const std::vector<ChannelSample>& raw){std::vector<ChannelSample> out;out.reserve(raw.size());int last=-1;for(const auto& p:raw){const auto px=static_cast<int>(std::lround(p.t/30*1000));if(px==last)out.back()=p;else{out.push_back(p);last=px;}}return out;}
double render(Gdiplus::Graphics& graphics,const std::vector<ChannelSample>& raw,bool peak){
    using namespace Gdiplus;const auto begin=std::chrono::steady_clock::now();
    for(int channel=0;channel<4;++channel){Pen pen(Color(255,0,180,100),5);pen.SetLineJoin(LineJoinRound);
        if(peak){const auto spans=plot_pixel_spans(raw,0,30,1000);std::vector<PointF> points;std::vector<BYTE> types;const auto line=[&](float x0,float y0,float x1,float y1){points.emplace_back(x0,y0);types.push_back(PathPointTypeStart);points.emplace_back(x1,y1);types.push_back(PathPointTypeLine);};float previous_x=0,previous_y=0;bool first=true;
            for(const auto& span:spans){const float x=static_cast<float>(80+span.x),last=static_cast<float>(550-(span.last.v+300)/1600*500);float low=static_cast<float>(550-(span.high.v+300)/1600*500),high=static_cast<float>(550-(span.low.v+300)/1600*500);if(!first){line(previous_x,previous_y,x,previous_y);low=std::min(low,previous_y);high=std::max(high,previous_y);}if(low!=high)line(x,low,x,high);else if(first)line(x-0.5f,last,x+0.5f,last);previous_x=x;previous_y=last;first=false;}
            GraphicsPath path(points.data(),types.data(),static_cast<INT>(points.size()));if(graphics.DrawPath(&pen,&path)!=Ok)throw std::runtime_error("DrawPath failed");}
        else {const auto data=legacy(raw);for(size_t i=0;i+1<data.size();++i){const float x0=static_cast<float>(80+data[i].t/30*1000),x1=static_cast<float>(80+data[i+1].t/30*1000),y0=static_cast<float>(550-(data[i].v+300)/1600*500),y1=static_cast<float>(550-(data[i+1].v+300)/1600*500);graphics.DrawLine(&pen,x0,y0,x1,y0);graphics.DrawLine(&pen,x1,y0,x1,y1);}}
    }graphics.Flush(FlushIntentionSync);return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
}
}
int main(){Gdiplus::GdiplusStartupInput input;ULONG_PTR token=0;if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok)return 1;int code=0;try{{Gdiplus::Bitmap bitmap(1200,600,PixelFormat32bppARGB);Gdiplus::Graphics graphics(&bitmap);graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);graphics.SetClip(Gdiplus::Rect(80,50,1000,500));std::vector<ChannelSample> raw;for(int n=0;n<20000;++n)raw.push_back({n*30.0/20000,201+100*std::sin(n*0.01)});raw[102].v=1000;raw[104].v=-201;render(graphics,raw,false);render(graphics,raw,true);std::vector<double> before,after;for(int n=0;n<5;++n){before.push_back(render(graphics,raw,false));after.push_back(render(graphics,raw,true));std::cout<<"run "<<n+1<<" legacy_ms="<<before.back()<<" peak_batch_ms="<<after.back()<<'\n';}std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());std::cout<<"median legacy_ms="<<before[2]<<" peak_batch_ms="<<after[2]<<"; 4 x 20000 points, same memory bitmap/quality/width, no physical display claim\n";}}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';code=1;}Gdiplus::GdiplusShutdown(token);return code;
}
