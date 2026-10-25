#include "session_instance.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
namespace {
void check(bool ok,const char* name){if(!ok) throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
std::string read(const std::filesystem::path& path){std::ifstream input(path);return std::string(std::istreambuf_iterator<char>(input),{});}
struct Child {
    HANDLE process=nullptr;
    bool finish() noexcept {
        if(!process)return true;
        if(WaitForSingleObject(process,1000)!=WAIT_OBJECT_0)TerminateProcess(process,9);
        const bool exited=WaitForSingleObject(process,5000)==WAIT_OBJECT_0;
        CloseHandle(process);process=nullptr;return exited;
    }
    ~Child(){if(!finish())std::cerr<<"CLEANUP FAIL: owned helper did not exit\n";}
    Child()=default;Child(const Child&)=delete;
};
void plain_path(const std::filesystem::path& path){
    std::filesystem::path part;
    for(const auto& component:path){part/=component;const auto attributes=GetFileAttributesW(part.c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Helper path contains a reparse point");}
}
struct HelperCopy {
    std::filesystem::path path;bool removed=false;
    explicit HelperCopy(std::filesystem::path value):path(std::move(value)){if(std::filesystem::exists(path))throw std::runtime_error("Helper copy already exists");}
    ~HelperCopy() noexcept {if(!removed)try{plain_path(path);std::error_code error;std::filesystem::remove(path,error);if(error)std::cerr<<"CLEANUP FAIL: helper copy: "<<error.message()<<'\n';}catch(...){std::cerr<<"CLEANUP FAIL: helper path validation\n";}}
    void remove_checked(){plain_path(path);std::filesystem::remove(path);check(!std::filesystem::exists(path),"test-owned renamed helper removed after children exit");removed=true;}
};
void start(Child& child,const std::wstring& exe,const std::filesystem::path& report,const std::wstring& id,const wchar_t* mode=L"ready",unsigned hold=1600) {
    std::wstring command=L"\""+exe+L"\" --child \""+report.wstring()+L"\" "+id+L" "+mode+L" "+std::to_wstring(hold);
    STARTUPINFOW startup{sizeof(startup)};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
    if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,report.parent_path().c_str(),&startup,&process)) throw std::runtime_error("Cannot launch private instance helper");
    CloseHandle(process.hThread);child.process=process.hProcess;
}
void wait(const Child& child){check(WaitForSingleObject(child.process,10000)==WAIT_OBJECT_0,"helper exits within bounded deadline");}
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc==6 && std::wstring(argv[1])==L"--child") {
            SessionInstance instance;std::wstring error;const auto result=instance.acquire(argv[3],error);
            if(result!=SessionInstance::Result::Owner){std::ofstream out(argv[2]);out<<(result==SessionInstance::Result::Activated?"activated":"failed");return 0;}
            HWND main=CreateWindowW(L"STATIC",L"unchanged-data",WS_POPUP,0,0,100,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            HWND modal=CreateWindowW(L"STATIC",L"modal-data",WS_POPUP,0,0,80,80,main,nullptr,GetModuleHandleW(nullptr),nullptr);
            EnableWindow(main,FALSE);
            if(std::wstring(argv[4])!=L"not-ready") instance.ready(main);
            {std::ofstream out(argv[2]);out<<"owner";}
            const auto end=GetTickCount64()+wcstoul(argv[5],nullptr,10);
            while(GetTickCount64()<end){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(2);}
            wchar_t text[64]{};GetWindowTextW(main,text,64);const bool preserved=IsWindow(modal) && !IsWindowEnabled(main) && std::wstring(text)==L"unchanged-data";
            DestroyWindow(modal);DestroyWindow(main);return preserved?0:1;
        }
        if(argc!=2) throw std::runtime_error("Expected private fixture folder");
        const auto root=std::filesystem::absolute(argv[1]).lexically_normal();plain_path(root);std::filesystem::create_directories(root);plain_path(root);
        const auto folder=root/(std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));check(std::filesystem::create_directory(folder),"fresh helper fixture directory");
        const auto failed_copy=folder/L"failed-helper.exe";
        try{HelperCopy owned(failed_copy);std::ofstream(failed_copy,std::ios::binary)<<"owned fixture";throw std::runtime_error("intentional helper fixture failure");}catch(const std::exception&){}
        check(!std::filesystem::exists(failed_copy),"helper copy cleanup during exception unwinding");
        std::ofstream(folder/L"fixture-recipe.txt")<<"Copy session_instance_tests.exe as renamed-helper.exe; private no-data children only. Remove copy after all owned children exit.\n";
        wchar_t path[MAX_PATH]{};GetModuleFileNameW(nullptr,path,MAX_PATH);const std::wstring exe=path;
        const auto copy=folder/L"renamed-helper.exe";HelperCopy owned_copy(copy);check(CopyFileW(exe.c_str(),copy.c_str(),TRUE)!=FALSE,"renamed private helper copy");
        const auto identity=L"PrivateTest."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64());
        Child first,second;start(first,exe,folder/L"first.txt",identity);start(second,copy.wstring(),folder/L"second.txt",identity);wait(first);wait(second);
        const auto a=read(folder/L"first.txt"),b=read(folder/L"second.txt");check((a=="owner"&&b=="activated")||(b=="owner"&&a=="activated"),"simultaneous renamed launches have exactly one owner");
        DWORD exit_a=1,exit_b=1;GetExitCodeProcess(first.process,&exit_a);GetExitCodeProcess(second.process,&exit_b);check(!exit_a&&!exit_b,"activation preserves main/modal data and busy state");
        Child relaunch;start(relaunch,exe,folder/L"relaunch.txt",identity,L"ready",100);wait(relaunch);check(read(folder/L"relaunch.txt")=="owner","normal exit permits relaunch");
        Child abandoned;const auto crash_id=identity+L".Abandoned";start(abandoned,exe,folder/L"abandoned.txt",crash_id,L"ready",8000);
        const auto ready_deadline=GetTickCount64()+4000;while(!std::filesystem::exists(folder/L"abandoned.txt")&&GetTickCount64()<ready_deadline) Sleep(10);
        check(read(folder/L"abandoned.txt")=="owner","test-owned crash helper acquired guard");
        HANDLE kept_mutex=OpenMutexW(SYNCHRONIZE,FALSE,(L"Local\\Released.SimpleComChart."+crash_id+L".Owner").c_str());check(kept_mutex!=nullptr,"keep abandoned mutex object alive across helper crash");
        wchar_t actual[32768]{};DWORD count=32768;check(QueryFullProcessImageNameW(abandoned.process,0,actual,&count)&&_wcsicmp(actual,exe.c_str())==0,"exact helper image revalidated before simulated crash");
        check(TerminateProcess(abandoned.process,7)!=FALSE,"only private no-data helper terminated for abandonment test");wait(abandoned);
        Child recovered;start(recovered,copy.wstring(),folder/L"recovered.txt",crash_id,L"ready",100);wait(recovered);check(read(folder/L"recovered.txt")=="owner","abandoned-owner recovery");CloseHandle(kept_mutex);
        Child unready,timeout;const auto timeout_id=identity+L".Unready";start(unready,exe,folder/L"unready.txt",timeout_id,L"not-ready",1600);
        const auto until=GetTickCount64()+4000;while(!std::filesystem::exists(folder/L"unready.txt")&&GetTickCount64()<until) Sleep(10);
        start(timeout,copy.wstring(),folder/L"timeout.txt",timeout_id,L"ready",100);wait(unready);wait(timeout);check(read(folder/L"timeout.txt")=="failed","readiness timeout never becomes a second owner/UI");
        SessionInstance invalid;std::wstring error;check(invalid.acquire(std::wstring(1000,L'x'),error)==SessionInstance::Result::Failed && !error.empty(),"guard failure is fail-closed");
        for(auto* child:{&first,&second,&relaunch,&abandoned,&recovered,&unready,&timeout})check(child->finish(),"owned child closed before artifact cleanup");
        owned_copy.remove_checked();return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
