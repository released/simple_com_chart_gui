#pragma once
#include "uart_files.h"
#include "disposable_test_files.h"
#include <algorithm>
class SessionFixture {
public:
    SessionFixture() {
        const auto project=std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path()).lexically_normal();
        root=project/L"build"/L"verify"/(L"session-core-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
        DisposableTestFiles::plain_path(root);std::filesystem::create_directories(root.parent_path());
        if(!std::filesystem::create_directory(root))throw std::runtime_error("Fixture must be fresh.");
    }
    std::filesystem::path file(const std::wstring& name) {
        const std::filesystem::path leaf(name);if(leaf.has_parent_path()||name==L"."||name==L"..")throw std::runtime_error("Owned fixture leaf required.");
        const auto p=root/leaf;DisposableTestFiles::plain_path(p);
        if(std::find(files.begin(),files.end(),p)==files.end()){if(std::filesystem::exists(p))throw std::runtime_error("Unknown fixture file.");files.push_back(p);}return p;
    }
    void write(const std::wstring& name,std::string_view text){uart_write_file(file(name),text,false,cancel);}
    void cleanup() {
        DisposableTestFiles::plain_path(root);for(const auto& f:files){DisposableTestFiles::plain_path(f);std::filesystem::remove(f);}
        if(std::filesystem::exists(root)&&!std::filesystem::remove(root))throw std::runtime_error("Unknown fixture content retained.");
    }
    ~SessionFixture(){try{cleanup();}catch(const std::exception& e){std::cerr<<"CLEANUP FAIL: "<<e.what()<<'\n';}}
    std::filesystem::path root;std::atomic<bool> cancel{false};
private:std::vector<std::filesystem::path> files;
};
