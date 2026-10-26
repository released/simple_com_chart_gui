#include "ui_language.h"
#include <fstream>
#include <iterator>
#include <sstream>
#include <set>
#include <stdexcept>
#include <cstdint>
namespace ui {
namespace {
Catalog current;std::string active="en";std::wstring notice;
std::wstring wide(std::string_view s){if(s.empty())return {};const auto n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!n)throw std::runtime_error("Invalid UTF-8 language catalog.");std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),out.data(),n);return out;}
std::string utf8(std::wstring_view s){if(s.empty())return {};const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);if(!n)throw std::runtime_error("Invalid language text.");std::string out(n,0);WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),out.data(),n,nullptr,nullptr);return out;}
std::string trim(std::string s){const auto first=s.find_first_not_of(" \t");return first==s.npos?"":s.substr(first,s.find_last_not_of(" \t")-first+1);}
std::wstring unescape(const std::string& s){std::string out;for(size_t i=0;i<s.size();++i){if(s[i]!='\\'){out+=s[i];continue;}if(++i==s.size())throw std::runtime_error("Incomplete language escape.");switch(s[i]){case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;case '\\':out+='\\';break;default:throw std::runtime_error("Unknown language escape.");}}return wide(out);}
}
std::string text_id(std::wstring_view s){uint64_t hash=14695981039346656037ULL;for(unsigned char c:utf8(s)){hash^=c;hash*=1099511628211ULL;}constexpr char digits[]="0123456789ABCDEF";std::string out="S_";for(int i=15;i>=0;--i)out+=digits[(hash>>(4*i))&15];return out;}
Catalog parse_catalog(std::string_view input,const std::string& code){
    if((code!="en"&&code!="tw")||input.size()>2*1024*1024||input.find('\0')!=input.npos)throw std::runtime_error("Invalid language catalog size/code.");
    if(input.substr(0,3)=="\xEF\xBB\xBF")input.remove_prefix(3);wide(input);Catalog out;std::string section,line;std::set<std::string> seen;bool version=false;
    std::istringstream stream{std::string(input)};while(std::getline(stream,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.size()>32768)throw std::runtime_error("Language line too long.");const auto clean=trim(line);if(clean.empty()||clean[0]==';'||clean[0]=='#')continue;
        if(clean.front()=='['&&clean.back()==']'){section=clean.substr(1,clean.size()-2);if(section!="Language"&&section!="Strings")throw std::runtime_error("Unknown language section.");continue;}
        const auto eq=line.find('=');if(eq==line.npos)throw std::runtime_error("Malformed language entry.");const auto key=trim(line.substr(0,eq)),value=line.substr(eq+1);if(!seen.insert(section+"/"+key).second)throw std::runtime_error("Duplicate language key.");
        if(section=="Language"){if(key=="Code")out.code=trim(value);else if(key=="Version"&&trim(value)=="1")version=true;else if(key!="Name")throw std::runtime_error("Unsupported language metadata.");}
        else if(section=="Strings"){if(key.size()!=18||key.substr(0,2)!="S_"||key.find_first_not_of("0123456789ABCDEF",2)!=key.npos||out.texts.size()>=8192)throw std::runtime_error("Invalid language text ID/count.");out.texts.emplace(key,unescape(value));}
        else throw std::runtime_error("Language entry without a section.");}
    if(out.code!=code||!version||out.texts.empty())throw std::runtime_error("Incomplete/mismatched language catalog.");return out;
}
std::wstring translate(const Catalog& c,std::wstring_view source){const auto it=c.texts.find(text_id(source));return it==c.texts.end()||it->second.empty()?std::wstring(source):it->second;}
std::string saved_language(const std::filesystem::path& ini){wchar_t value[16]{};const auto n=GetPrivateProfileStringW(L"UI",L"Language",L"tw",value,16,ini.c_str());return n<15&&std::wstring(value)==L"en"?"en":"tw";}
bool save_language(const std::filesystem::path& ini,const std::string& code){if(code!="en"&&code!="tw")return false;return WritePrivateProfileStringW(L"UI",L"Language",code=="en"?L"en":L"tw",ini.c_str())!=FALSE;}
void initialize_language(const std::filesystem::path& executable){
    current={};notice.clear();active=saved_language(std::filesystem::path(executable).replace_extension(L".ini"));const auto path=executable.parent_path()/(active=="en"?L"lang_en.ini":L"lang_tw.ini");
    try{const auto size=std::filesystem::file_size(path);if(size>2*1024*1024)throw std::runtime_error("Language file too large.");std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("Cannot read language file.");std::string bytes(static_cast<size_t>(size),'\0');in.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));if(static_cast<size_t>(in.gcount())!=bytes.size()||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Language file changed during read.");current=parse_catalog(bytes,active);}
    catch(...){current={};notice=L"Language file is missing or invalid. English fallback is active; check lang_en.ini / lang_tw.ini beside the EXE.";active="en";}
}
const std::string& active_language(){return active;}
const std::wstring& language_notice(){return notice;}
std::wstring tr(std::wstring_view source){return translate(current,source);}
std::wstring display_caption(std::wstring_view source){
    const auto exact=tr(source);if(exact!=source)return exact;
    for(const auto prefix:{L"Detected: ",L"Samples: ",L"Rows: ",L"Bytes: ",L"Font: ",L"Startup tab: ",L"Advanced UART tools | ",L"UART Console - "}){
        const std::wstring_view p(prefix);if(source.substr(0,p.size())==p)return tr(p)+std::wstring(source.substr(p.size()));}
    return exact;
}
const wchar_t* caption(const wchar_t* source){if(!source||IS_INTRESOURCE(source))return source;try{const auto it=current.texts.find(text_id(source));return it==current.texts.end()||it->second.empty()?source:it->second.c_str();}catch(...){return source;}}
}
