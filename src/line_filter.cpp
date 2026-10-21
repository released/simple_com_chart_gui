#include "line_filter.h"

bool LineFilter::configure(const std::string& include, const std::string& exclude,
                           bool regex_mode, std::string& error) {
    error.clear();
    if (include.size() > 1024 || exclude.size() > 1024) {
        error = "Filter patterns are too long (max 256 characters).";
        return false;
    }
    std::regex include_regex, exclude_regex;
    try {
        if (regex_mode && !include.empty()) include_regex = std::regex(include);
        if (regex_mode && !exclude.empty()) exclude_regex = std::regex(exclude);
    } catch (const std::regex_error& failure) {
        error = std::string("Invalid regex: ") + failure.what();
        return false;
    }
    include_ = include;
    exclude_ = exclude;
    regex_mode_ = regex_mode;
    include_regex_ = std::move(include_regex);
    exclude_regex_ = std::move(exclude_regex);
    return true;
}

bool LineFilter::matches(const std::string& line) const {
    if (!regex_mode_) {
        return (include_.empty() || line.find(include_) != std::string::npos) &&
               (exclude_.empty() || line.find(exclude_) == std::string::npos);
    }
    return (include_.empty() || std::regex_search(line, include_regex_)) &&
           (exclude_.empty() || !std::regex_search(line, exclude_regex_));
}
