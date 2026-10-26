#include "line_filter.h"

bool LineFilter::configure(const std::string& include, const std::string& exclude,
                           bool regex_mode, std::string& error) {
    error.clear();
    if (include.size() > 1024 || exclude.size() > 1024) {
        error = "Filter patterns are too long (max 256 characters).";
        return false;
    }
    BoundedRegex include_regex, exclude_regex;
    if(regex_mode && ((!include.empty() && !include_regex.compile(include,error)) || (!exclude.empty() && !exclude_regex.compile(exclude,error)))) return false;
    include_ = include;
    exclude_ = exclude;
    regex_mode_ = regex_mode;
    include_regex_ = std::move(include_regex);
    exclude_regex_ = std::move(exclude_regex);
    limited_=0;
    return true;
}

bool LineFilter::matches(const std::string& line) const {
    return evaluate(line)==Accept;
}
LineFilter::Decision LineFilter::evaluate(const std::string& line)const {
    if (!regex_mode_) {
        if(!include_.empty()&&line.find(include_)==std::string::npos)return IncludeMiss;
        if(!exclude_.empty()&&line.find(exclude_)!=std::string::npos)return Excluded;
        return Accept;
    }
    const auto included=include_.empty()?BoundedRegex::Match{}:include_regex_.search(line,65536);
    const auto excluded=exclude_.empty()?BoundedRegex::Match{}:exclude_regex_.search(line,65536);
    if(included.limited || excluded.limited) {++limited_;return WorkLimit;}
    if(!exclude_.empty()&&excluded.found)return Excluded;
    return include_.empty()||included.found?Accept:IncludeMiss;
}
