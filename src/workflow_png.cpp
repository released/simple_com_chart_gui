#include "workflow_png.h"
#include <gdiplus.h>
#include <objidl.h>
#include <stdexcept>
#include <memory>
namespace {
void dimensions(int w,int h){if(w<400||h<300||w>4096||h>4096||static_cast<uint64_t>(w)*h>16000000)throw std::runtime_error("PNG dimensions: 400..4096 by 300..4096, at most 16 million pixels.");}
struct Surface {
    HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ old=nullptr;void* pixels=nullptr;
    Surface(int w,int h){dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!dc||!bitmap){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);throw std::runtime_error("Cannot allocate bounded PNG canvas.");}old=SelectObject(dc,bitmap);}
    ~Surface(){if(dc){SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);}}
};
struct StreamRelease {void operator()(IStream* p)const{if(p)p->Release();}};
}
PlotRaster raster_plots(const std::vector<PlotView*>& plots,int width,int height){dimensions(width,height);if(plots.size()!=1&&plots.size()!=4)throw std::runtime_error("Choose one plot or four plots.");if(plots.size()==4&&(width<800||height<600))throw std::runtime_error("Four plots require at least 800 x 600.");Surface canvas(width,height);PatBlt(canvas.dc,0,0,width,height,BLACKNESS);
    const int columns=plots.size()==4?2:1;for(size_t i=0;i<plots.size();++i){if(!plots[i])throw std::runtime_error("Plot is unavailable.");const int left=static_cast<int>(i%columns)*width/columns,top=static_cast<int>(i/columns)*height/columns,right=static_cast<int>(i%columns+1)*width/columns,bottom=static_cast<int>(i/columns+1)*height/columns;Surface tile(right-left,bottom-top);plots[i]->render_to(tile.dc,right-left,bottom-top);if(!BitBlt(canvas.dc,left,top,right-left,bottom-top,tile.dc,0,0,SRCCOPY))throw std::runtime_error("Cannot render chart image.");}
    PlotRaster out;out.width=width;out.height=height;const auto pixels=static_cast<const uint32_t*>(canvas.pixels);out.pixels.assign(pixels,pixels+static_cast<size_t>(width)*height);return out;
}
std::string encode_plot_png(const PlotRaster& r,const std::atomic<bool>& cancel){dimensions(r.width,r.height);if(cancel||r.pixels.size()!=static_cast<size_t>(r.width)*r.height)throw std::runtime_error("Cancelled/invalid PNG raster.");UINT count=0,size=0;if(Gdiplus::GetImageEncodersSize(&count,&size)!=Gdiplus::Ok||size>65536)throw std::runtime_error("PNG encoder unavailable.");std::vector<unsigned char> storage(size);const auto codecs=reinterpret_cast<Gdiplus::ImageCodecInfo*>(storage.data());if(Gdiplus::GetImageEncoders(count,size,codecs)!=Gdiplus::Ok)throw std::runtime_error("Cannot query PNG encoder.");const CLSID* encoder=nullptr;for(UINT i=0;i<count;++i)if(wcscmp(codecs[i].MimeType,L"image/png")==0){encoder=&codecs[i].Clsid;break;}if(!encoder)throw std::runtime_error("PNG encoder unavailable.");
    IStream* raw=nullptr;if(FAILED(CreateStreamOnHGlobal(nullptr,TRUE,&raw)))throw std::runtime_error("Cannot allocate PNG stream.");std::unique_ptr<IStream,StreamRelease> stream(raw);Gdiplus::Bitmap bitmap(r.width,r.height,r.width*4,PixelFormat32bppRGB,reinterpret_cast<BYTE*>(const_cast<uint32_t*>(r.pixels.data())));if(bitmap.GetLastStatus()!=Gdiplus::Ok||bitmap.Save(stream.get(),encoder)!=Gdiplus::Ok||cancel)throw std::runtime_error("PNG encoding failed/cancelled.");STATSTG stat{};if(FAILED(stream->Stat(&stat,STATFLAG_NONAME))||stat.cbSize.QuadPart>128ULL*1024*1024)throw std::runtime_error("PNG exceeds bounded output.");LARGE_INTEGER zero{};if(FAILED(stream->Seek(zero,STREAM_SEEK_SET,nullptr)))throw std::runtime_error("Cannot read PNG stream.");std::string out(static_cast<size_t>(stat.cbSize.QuadPart),'\0');ULONG read=0;if(FAILED(stream->Read(out.data(),static_cast<ULONG>(out.size()),&read))||read!=out.size()||cancel)throw std::runtime_error("PNG stream incomplete/cancelled.");return out;
}
