#include "session_manifest.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <stdexcept>
namespace {void cancelled(const std::atomic<bool>& c){if(c)throw std::runtime_error("Session file work cancelled.");}}
std::string file_sha256(const ReplayFile& file,const std::atomic<bool>& cancel){
    struct Hash {BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::vector<unsigned char> object;~Hash(){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}} h;
    if(BCryptOpenAlgorithmProvider(&h.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 provider unavailable.");ULONG size=0,got=0;if(BCryptGetProperty(h.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)<0||size>1048576)throw std::runtime_error("Invalid SHA256 provider state.");h.object.resize(size);
    if(BCryptCreateHash(h.algorithm,&h.hash,h.object.data(),size,nullptr,0,0)<0)throw std::runtime_error("Cannot create SHA256 state.");
    for(uint64_t offset=0;offset<file.size();){cancelled(cancel);const auto block=file.read(offset,static_cast<size_t>(std::min<uint64_t>(65536,file.size()-offset)),cancel);if(BCryptHashData(h.hash,reinterpret_cast<PUCHAR>(const_cast<char*>(block.data())),static_cast<ULONG>(block.size()),0)<0)throw std::runtime_error("SHA256 update failed.");offset+=block.size();}
    cancelled(cancel);unsigned char digest[32]{};if(BCryptFinishHash(h.hash,digest,sizeof(digest),0)<0)throw std::runtime_error("SHA256 finish failed.");return uart_hex(std::string_view(reinterpret_cast<char*>(digest),sizeof(digest)));
}
