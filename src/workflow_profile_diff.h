#pragma once
#include "uart_profiles.h"
#include <array>
using ProfileDifference=std::array<std::string,3>;
std::vector<ProfileDifference> profile_differences(const UartProfile&,const UartProfile&);
