#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
enum class BlockMode {XChecksum,XCrc,X1K,Ymodem};
struct BlockOptions {BlockMode mode=BlockMode::XCrc;unsigned timeout_ms=3000,retries=10;};
struct BlockLink {
    std::function<int(unsigned)> byte; // -1 timeout, otherwise 0..255
    std::function<void(std::string_view)> write; // complete driver acceptance, throws on partial/error
    std::function<void()> check; // throws on cancel/generation change/one-hour limit
    std::function<void(uint64_t,unsigned)> progress;
};
struct BlockSource {std::string name;uint64_t size=0;std::function<std::string(uint64_t,size_t)> read;};
struct BlockSink {
    std::function<void(const std::string&,std::optional<uint64_t>)> begin;
    std::function<void(std::string_view)> append;
    std::function<void()> commit;
};
uint16_t xmodem_crc(std::string_view);
bool safe_transfer_name(std::string_view);
std::string xmodem_packet(unsigned number,std::string_view data,size_t block_size,bool crc);
void block_send(const BlockOptions&,const std::vector<BlockSource>&,BlockLink&);
void block_receive(const BlockOptions&,BlockSink&,BlockLink&,std::optional<uint64_t> xmodem_size={});
