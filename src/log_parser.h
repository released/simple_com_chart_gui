#pragma once

#include <string>
#include <unordered_map>

namespace log_parser {
bool parse_numeric_value(const std::string& text, double& value);
std::unordered_map<std::string, double> parse_kv_log(const std::string& line);
}
