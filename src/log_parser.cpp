#include "log_parser.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <regex>
#include <vector>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

bool parse_number(const std::string& text, double& value) {
    static const std::regex number(
        R"(^([+-]?0[xX][0-9a-fA-F]+|[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)\s*([A-Za-z%]*)$)");
    std::smatch match;
    const auto clean = trim(text);
    if (!std::regex_match(clean, match, number)) return false;
    const auto token = match[1].str();
    const auto prefix = clean[0] == '+' || clean[0] == '-' ? 1u : 0u;
    if (clean.size() >= prefix + 2 && clean[prefix] == '0' &&
        (clean[prefix + 1] == 'x' || clean[prefix + 1] == 'X') &&
        token.find_first_of("xX") == std::string::npos) return false;
    try {
        if (token.find("0x") != std::string::npos || token.find("0X") != std::string::npos) {
            if (!match[2].str().empty()) return false;
            const bool negative = token[0] == '-';
            const auto offset = token[0] == '-' || token[0] == '+' ? 3u : 2u;
            value = static_cast<double>(std::stoull(token.substr(offset), nullptr, 16));
            if (negative) value = -value;
        } else {
            value = std::strtod(token.c_str(), nullptr);
        }
    } catch (const std::exception&) {
        return false;
    }
    return std::isfinite(value);
}
}

namespace log_parser {
std::unordered_map<std::string, double> parse_kv_log(const std::string& line) {
    std::unordered_map<std::string, double> result;
    size_t start = 0;
    while (start < line.size()) {
        auto end = line.find(',', start);
        if (end == std::string::npos) end = line.size();
        const auto token = line.substr(start, end - start);
        start = end + 1;
        const auto separator = token.find_first_of(":=");
        if (separator == std::string::npos) continue;
        const auto key = trim(token.substr(0, separator));
        if (key.empty() || key.size() > 128 || std::any_of(key.begin(), key.end(),
            [](unsigned char ch) { return ch < 32; })) continue;
        const auto raw = token.substr(separator + 1);
        std::vector<double> values;
        size_t part_start = 0;
        bool valid = true;
        do {
            auto part_end = raw.find('/', part_start);
            if (part_end == std::string::npos) part_end = raw.size();
            double value = 0;
            if (!parse_number(raw.substr(part_start, part_end - part_start), value)) {
                valid = false;
                break;
            }
            values.push_back(value);
            part_start = part_end + 1;
        } while (part_start <= raw.size());
        if (!valid) continue;
        for (size_t i = 0; i < values.size(); ++i) {
            result[values.size() == 1 ? key : key + "[" + std::to_string(i) + "]"] = values[i];
        }
    }
    return result;
}
}
