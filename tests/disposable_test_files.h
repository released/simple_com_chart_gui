#pragma once
#include <windows.h>
#include <filesystem>
#include <vector>
#include <iostream>
#include <stdexcept>

class DisposableTestFiles {
public:
    static void plain_path(const std::filesystem::path& path) {
        std::filesystem::path part;
        for(const auto& component:std::filesystem::absolute(path).lexically_normal()) {
            part/=component;const auto attributes=GetFileAttributesW(part.c_str());
            if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Test path contains a reparse point");
        }
    }
    void own(const std::filesystem::path& path) {
        plain_path(path);
        if(std::filesystem::exists(path))throw std::runtime_error("Test output already exists");
        paths_.push_back(path);
    }
    void finish() {
        for(const auto& path:paths_) {
            plain_path(path);std::filesystem::remove(path);
            if(std::filesystem::exists(path))throw std::runtime_error("Disposable test file was not removed");
        }
        finished_=true;
    }
    ~DisposableTestFiles() noexcept {
        if(finished_)return;
        for(const auto& path:paths_)try {
            plain_path(path);std::error_code error;std::filesystem::remove(path,error);
            if(error)std::cerr<<"CLEANUP FAIL: "<<path.filename().string()<<": "<<error.message()<<'\n';
        }catch(...){std::cerr<<"CLEANUP FAIL: unsafe disposable path\n";}
    }
private:
    std::vector<std::filesystem::path> paths_;
    bool finished_=false;
};
