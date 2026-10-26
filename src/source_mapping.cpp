#include "source_mapping.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace {
void valid(const TextFraming& c){if(c.mode<TextFraming::LF||c.mode>TextFraming::Idle||c.idle_ms<1||c.idle_ms>60000||c.delimiter.size()>16||(c.mode==TextFraming::Delimiter&&c.delimiter.empty()))throw std::runtime_error("Text framing: LF/CR/CRLF, 1..16 delimiter bytes, or 1..60000 ms receipt idle gap.");}
std::string hex(std::string_view s){constexpr char d[]="0123456789ABCDEF";std::string out;for(unsigned char c:s){out+=d[c>>4];out+=d[c&15];}return out;}
}
std::string encode_text_framing(const TextFraming& c){valid(c);return "v1:"+std::to_string(c.mode)+":"+std::to_string(c.idle_ms)+":"+hex(c.delimiter);}
TextFraming decode_text_framing(const std::string& s){TextFraming c;if(s.empty())return c;if(s.size()>64||s.substr(0,3)!="v1:")throw std::runtime_error("Invalid text framing schema.");const auto a=s.find(':',3),b=a==s.npos?s.npos:s.find(':',a+1);if(a==s.npos||b==s.npos)throw std::runtime_error("Invalid framing fields.");unsigned m=0,t=0;const auto r=std::from_chars(s.data()+3,s.data()+a,m),q=std::from_chars(s.data()+a+1,s.data()+b,t);if(r.ec!=std::errc{}||r.ptr!=s.data()+a||q.ec!=std::errc{}||q.ptr!=s.data()+b||m>4||(s.size()-b-1)%2)throw std::runtime_error("Invalid framing numbers.");
    c.mode=static_cast<TextFraming::Mode>(m);c.idle_ms=t;for(size_t p=b+1;p<s.size();p+=2){unsigned byte=0;const auto n=std::from_chars(s.data()+p,s.data()+p+2,byte,16);if(n.ec!=std::errc{}||n.ptr!=s.data()+p+2)throw std::runtime_error("Invalid delimiter HEX.");c.delimiter+=static_cast<char>(byte);}valid(c);return c;}
void SourceLineTracker::configure(const TextFraming& c){valid(c);framing_=c;delimiter_=c.mode==TextFraming::LF?"\n":c.mode==TextFraming::CR?"\r":c.mode==TextFraming::CRLF?"\r\n":c.delimiter;prefix_.assign(delimiter_.size(),0);for(size_t i=1,j=0;i<delimiter_.size();++i){while(j&&delimiter_[i]!=delimiter_[j])j=prefix_[j-1];if(delimiter_[i]==delimiter_[j])++j;prefix_[i]=j;}clear_partial();}
std::vector<SourceLine> SourceLineTracker::feed(std::string_view bytes,double seconds){
    auto result=drain(seconds);if(!std::isfinite(seconds))throw std::runtime_error("Invalid text receipt time.");
    for(const char c:bytes){const auto offset=end_++;if(skip_lf_&&c=='\n'){skip_lf_=false;start_=end_;continue;}skip_lf_=false;if(line_.empty()&&!discard_)start_=offset;last_=seconds;
        if(!discard_)line_+=c;
        if(framing_.mode!=TextFraming::Idle){while(match_&&c!=delimiter_[match_])match_=prefix_[match_-1];if(c==delimiter_[match_])++match_;
            if(match_==delimiter_.size()){
                if(!discard_){line_.resize(line_.size()-delimiter_.size());if(framing_.mode==TextFraming::LF)while(!line_.empty()&&line_.back()=='\r')line_.pop_back();if(!line_.empty())result.push_back({std::move(line_),start_,end_-start_,last_});}
                line_.clear();discard_=false;match_=0;start_=end_;skip_lf_=framing_.mode==TextFraming::CR;continue;
            }
        }
        if(!discard_&&line_.size()-match_>65536){line_.clear();discard_=true;++skipped_;}
    }return result;
}
std::vector<SourceLine> SourceLineTracker::drain(double now){if(framing_.mode!=TextFraming::Idle||!std::isfinite(now)||now<last_||now-last_<framing_.idle_ms/1000.0)return {};return finish();}
std::vector<SourceLine> SourceLineTracker::finish(){std::vector<SourceLine> out;if(!discard_&&!line_.empty()){if(framing_.mode==TextFraming::LF)while(!line_.empty()&&line_.back()=='\r')line_.pop_back();if(!line_.empty())out.push_back({std::move(line_),start_,end_-start_,last_});}clear_partial();return out;}
