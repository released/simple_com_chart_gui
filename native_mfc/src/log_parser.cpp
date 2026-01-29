#include "log_parser.h"

#include <cctype>
namespace {
bool is_valid_key(const std::string& key) {
    if (key.size() < 2 || key.size() > 16) {
        return false;
    }
    if (!std::isalpha(static_cast<unsigned char>(key[0]))) {
        return false;
    }
    bool all_digits = true;
    for (size_t i = 0; i < key.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(key[i]);
        if (!(std::isalnum(ch) || ch == '_' || ch == '/')) {
            return false;
        }
        if (!std::isdigit(ch)) {
            all_digits = false;
        }
    }
    if (all_digits) {
        return false;
    }
    return true;
}

bool extract_int(const std::string& text, int* out) {
    if (!out) {
        return false;
    }
    bool found = false;
    int sign = 1;
    long value = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];
        if ((ch == '-' || ch == '+') && i + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[i + 1]))) {
            sign = (ch == '-') ? -1 : 1;
            i++;
            value = 0;
            while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                value = value * 10 + (text[i] - '0');
                i++;
            }
            found = true;
            break;
        }
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            sign = 1;
            value = 0;
            size_t j = i;
            while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j]))) {
                value = value * 10 + (text[j] - '0');
                j++;
            }
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }
    *out = static_cast<int>(sign * value);
    return true;
}
} // namespace

namespace log_parser {
std::unordered_map<std::string, int> parse_kv_log(const std::string& line) {
    std::unordered_map<std::string, int> result;
    if (line.empty()) {
        return result;
    }

    size_t start = 0;
    while (start < line.size()) {
        size_t end = line.find(',', start);
        if (end == std::string::npos) {
            end = line.size();
        }
        std::string token = line.substr(start, end - start);
        start = end + 1;

        size_t colon = token.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = token.substr(0, colon);
        std::string val = token.substr(colon + 1);

        auto trim = [](std::string& s) {
            size_t left = s.find_first_not_of(" \t\r\n");
            size_t right = s.find_last_not_of(" \t\r\n");
            if (left == std::string::npos) {
                s.clear();
                return;
            }
            s = s.substr(left, right - left + 1);
        };

        trim(key);
        trim(val);

        if (!is_valid_key(key)) {
            continue;
        }

        int value = 0;
        if (!extract_int(val, &value)) {
            continue;
        }
        result[key] = value;
    }

    return result;
}
} // namespace log_parser
