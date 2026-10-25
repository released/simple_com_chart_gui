#pragma once
#include <array>
#include <bitset>
#include <string>
#include <string_view>
#include <vector>

// Thompson NFA: no recursive backtracking, detached thread, or thread killing.
// Byte-oriented ASCII syntax over UTF-8 bytes; captures use byte offsets.
class BoundedRegex {
public:
    struct Match { bool found=false, limited=false; std::array<int,18> bounds{}; };
    bool compile(const std::string& pattern, std::string& error);
    Match search(std::string_view input, size_t budget=2000000) const;
    size_t groups() const { return groups_; }
private:
    struct State { enum Kind { Byte, Split, Save, Begin, End, Accept } kind=Accept; int a=-1,b=-1,slot=0; std::bitset<256> bytes; };
    std::vector<State> states_;
    int start_=-1;
    size_t groups_=0;
};
