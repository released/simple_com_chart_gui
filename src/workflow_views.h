#pragma once
#include <array>
#include <string>
#include <vector>
struct ViewAxis {double window=30,low=0,high=50;bool fixed=false;};
struct ViewPreset {
    std::vector<std::string> checked,columns;
    std::array<std::string,4> charts{};
    std::array<ViewAxis,5> axes{};
};
std::string encode_view(const ViewPreset&);
ViewPreset decode_view(const std::string&);
