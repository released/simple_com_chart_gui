#pragma once
#include "capture_store.h"
#include <atomic>
std::string workflow_html(const CaptureSnapshot&,const std::vector<std::string>& fields,const std::string& title,double first,double last,const std::atomic<bool>&);
std::string capture_case_csv(const std::string& input,const std::string& expected,const std::string& seconds);
std::string capture_case_input(const CaptureRow&);
