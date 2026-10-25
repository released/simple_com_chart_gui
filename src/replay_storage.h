#pragma once
#include "uart_files.h"
#include <memory>
#include <vector>
#include <cstdint>
struct ReplayRecord {size_t offset=0,count=0;double seconds=0;int64_t unix_ms=0;uint32_t file=0;uint64_t generation=0;};
// At most 4096 records in RAM; larger tables use an OS-paged temporary index.
class ReplayRecords {
public:
    ReplayRecords();~ReplayRecords();
    void push_back(ReplayRecord);
    void finish();
    size_t size() const;
    bool empty() const {return size()==0;}
    ReplayRecord operator[](size_t index) const;
    bool paged() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
class ReplayFile {
public:
    explicit ReplayFile(const std::filesystem::path&);
    ~ReplayFile();
    uint64_t size() const;
    std::string read(uint64_t offset,size_t count,const std::atomic<bool>& cancel) const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
