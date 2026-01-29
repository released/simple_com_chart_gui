#pragma once

#include <string>
#include <unordered_map>

namespace log_parser {
std::unordered_map<std::string, int> parse_kv_log(const std::string& line);
}
