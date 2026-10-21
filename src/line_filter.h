#pragma once

#include <regex>
#include <string>

class LineFilter {
public:
    bool configure(const std::string& include, const std::string& exclude,
                   bool regex_mode, std::string& error);
    bool matches(const std::string& line) const;
private:
    std::string include_, exclude_;
    bool regex_mode_ = false;
    std::regex include_regex_, exclude_regex_;
};
