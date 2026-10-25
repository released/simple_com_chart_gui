#include "tx_pacing.h"
#include <algorithm>
#include <chrono>
#include <thread>
bool valid_tx_pacing(TxPacing p){return p.byte_ms<=1000&&p.line_ms<=60000;}
size_t paced_write(std::string_view bytes,TxPacing p,const std::function<size_t(std::string_view,std::wstring&)>& write,
    const std::function<bool()>& current,std::wstring& error){
    error.clear();if(!valid_tx_pacing(p)){error=L"Invalid TX pacing.";return 0;}
    const auto wait=[&](unsigned ms){const auto end=std::chrono::steady_clock::now()+std::chrono::milliseconds(ms);while(std::chrono::steady_clock::now()<end){if(!current())return false;std::this_thread::sleep_for(std::min(std::chrono::milliseconds(5),std::chrono::duration_cast<std::chrono::milliseconds>(end-std::chrono::steady_clock::now())+std::chrono::milliseconds(1)));}return current();};
    size_t accepted=0;
    while(accepted<bytes.size()){
        if(!current()){error=L"TX cancelled; accepted bytes cannot be revoked.";break;}
        size_t n=bytes.size()-accepted;if(p.byte_ms)n=1;else if(p.line_ms){const auto end=bytes.find_first_of("\r\n",accepted);if(end!=std::string_view::npos){n=end-accepted+1;if(bytes[end]=='\r'&&end+1<bytes.size()&&bytes[end+1]=='\n')++n;}}
        const auto chunk=bytes.substr(accepted,n);const auto count=std::min(n,write(chunk,error));accepted+=count;
        if(!error.empty()||count!=n){if(error.empty())error=L"Partial driver write; remaining bytes were not retried.";break;}
        const char last=chunk.back();const bool line_end=last=='\n'||(last=='\r'&&(accepted==bytes.size()||bytes[accepted]!='\n'));
        const unsigned pause=std::max(p.byte_ms,line_end?p.line_ms:0U);
        if(pause&&!wait(pause)){error=L"TX cancelled during pacing; accepted bytes cannot be revoked.";break;}
    }return accepted;
}
