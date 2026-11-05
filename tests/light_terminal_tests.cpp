#include "light_terminal.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
// Retained pre-optimization algorithm, used only as a CPU/output reference.
std::wstring reference_terminal(std::string_view bytes,const MessageDisplayOptions& options) {
    std::deque<std::wstring> lines(1);size_t column=0;std::string run;
    auto display=options;display.clock=MessageClock::None;display.ansi_sgr=false;
    const auto flush=[&]{const auto formatted=format_message(run,0,{},display);run.clear();auto& line=lines.back();for(const wchar_t c:formatted.text){if(column>=4096)continue;if(column>=line.size())line.resize(column+1,L' ');line[column++]=c;}};
    for(size_t i=0;i<bytes.size();++i){const auto c=static_cast<unsigned char>(bytes[i]);
        if(c=='\r'||c=='\n'||c=='\b'){flush();if(c=='\r')column=0;else if(c=='\b'){if(column)--column;}else{lines.emplace_back();column=0;if(lines.size()>8192)lines.pop_front();}continue;}
        if(c==27&&i+2<bytes.size()&&bytes[i+1]=='['){size_t end=i+2;char mode='0';if(bytes[end]>='0'&&bytes[end]<='2')mode=bytes[end++];
            if(end<bytes.size()&&bytes[end]=='K'){flush();auto& line=lines.back();if(mode=='0')line.resize(std::min(column,line.size()));else if(mode=='1')std::fill(line.begin(),line.begin()+std::min(column+1,line.size()),L' ');else line.clear();i=end;continue;}}
        run+=static_cast<char>(c);
    }
    flush();std::wstring text;for(size_t i=0;i<lines.size();++i){if(i)text+=L"\r\n";text+=lines[i];}return text;
}
void check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
}
int main(){
    try{
        MessageDisplayOptions options;
        auto quiet=options;quiet.suppress_nul=true;
        const std::string mixed("A\0B\x01",4);
        check(format_message(mixed,0,{},quiet).text==L"AB\\x01","NUL suppression preserves printable text and other escaped controls");
        check(format_message("\\x00",0,{},quiet).text==L"\\x00","literal ASCII escape text is not mistaken for a NUL byte");
        check(format_message(mixed,0,{},options).text==L"A\\x00B\\x01","raw formatter defaults still expose NUL bytes");
        auto clock=quiet;clock.clock=MessageClock::Elapsed;
        check(format_message(std::string(32,'\0'),0,{},clock,1000).text.empty(),"NUL-only input creates no text or clock labels");
        check(format_message(std::string(3,'\0')+"A",0,{{0,1000},{3,2500}},clock,1000).text==L"[1.500s] A","suppressed NUL preserves source receipt offsets and clock");
        auto hex=quiet;hex.encoding=MessageEncoding::Hex;auto raw_hex=hex;raw_hex.suppress_nul=false;
        check(format_message(mixed,32,{},hex).text==format_message(mixed,32,{},raw_hex).text,"HEX ignores text suppression and keeps raw offsets/bytes");
        auto utf8=quiet;utf8.encoding=MessageEncoding::Utf8;
        check(format_message(std::string("\xC2\0\xA0",3),0,{},utf8).text==L"\\xC2\\xA0","suppression never joins UTF-8 across a corrupt NUL boundary");
        check(format_light_terminal(std::string("abc\0\rZ\r\r\n",9),quiet).text==L"Zbc\r\n","terminal cursor semantics survive NUL suppression");
        auto colored=quiet;colored.ansi_sgr=true;
        const auto spans=format_message(std::string("\0\x1b[31mA\0\x1b[0mB",13),0,{},colored);
        check(spans.text==L"AB"&&spans.colors.size()==2&&spans.colors[0].start==0&&spans.colors[0].count==1&&spans.colors[1].start==1,"NUL suppression preserves SGR text/color span positions");
        auto terminal_clock=quiet;terminal_clock.clock=MessageClock::Elapsed;
        check(format_light_terminal("A\r\nB",terminal_clock,100,{{90,1000},{103,2500}},1000).text==L"[0.000s] A\r\n[1.500s] B","terminal clocks use absolute retained source offsets");
        check(format_light_terminal("A\nB",terminal_clock,0,{{0,1000}},1000).text==L"[0.000s] A\r\n[0.000s] B","one receipt chunk may legitimately timestamp several rows equally");
        const std::vector<std::pair<std::string,std::wstring>> cases{
            {"",L""},{"first\r\r\nsecond\r\n",L"first\r\nsecond\r\n"},
            {"12345\rAB",L"AB345"},{"ab\bZ",L"aZ"},{"\bZ",L"Z"},
            {"123\r\x1b[KX",L"X"},{"abc\b\x1b[1KZ",L"  Z"},
            {"abc\x1b[2KZ",L"   Z"},{"pre\x1b[3Jpost",L"pre\\x1B[3Jpost"},
            {std::string(4100,'x')+"\rZ",L"Z"+std::wstring(4095,L'x')}
        };
        for(const auto& sample:cases){const auto actual=format_light_terminal(sample.first,options).text;
            check(actual==sample.second,"terminal golden output");check(actual==reference_terminal(sample.first,options),"bulk output equals retained reference");}
        auto unicode=options;unicode.encoding=MessageEncoding::Utf8;
        check(format_light_terminal(u8"中文\rA",unicode).text==L"A文","UTF-8 overwrite retains the existing tail");
        check(format_light_terminal("\xF0\x9F\x98\x80\r\xF0\x9F\x98\x81",unicode).text==L"\U0001F601","supplementary UTF-8 characters survive overwrite");
        const auto many=std::string(30000,'\n');const auto limited=format_light_terminal(many,options).text;
        check(limited.size()==8191*2&&limited==reference_terminal(many,options),"line retention stays bounded and exact");
        std::string row="a:1,b:2,c:3";row.resize(255,' ');row+='\n';std::string input;
        for(unsigned i=0;i<1024;++i)input+=row;
        check(format_light_terminal(input,options).text==reference_terminal(input,options),"full retained soak window matches reference");
        std::vector<double> before,after;volatile size_t checksum=0;
        for(unsigned run=0;run<5;++run){auto start=std::chrono::steady_clock::now();
            for(unsigned i=0;i<20;++i)checksum+=reference_terminal(input,options).size();
            before.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());start=std::chrono::steady_clock::now();
            for(unsigned i=0;i<20;++i)checksum+=format_light_terminal(input,options).text.size();
            after.push_back(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
        std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
        std::cout<<"BENCH: 20 renders x 262144 raw bytes; median reference_ms="<<before[2]<<" bulk_ms="<<after[2]<<" checksum="<<checksum<<" (CPU only, host-dependent)\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
