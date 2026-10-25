#pragma once

#include <string>
#include "bounded_regex.h"

class LineFilter {
public:
    bool configure(const std::string& include, const std::string& exclude,
                   bool regex_mode, std::string& error);
    bool matches(const std::string& line) const;
    uint64_t limited() const { return limited_; }
private:
    std::string include_, exclude_;
    bool regex_mode_ = false;
    BoundedRegex include_regex_, exclude_regex_;
    mutable uint64_t limited_=0;
};
