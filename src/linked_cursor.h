#pragma once
#include "capture_store.h"
#include <optional>
std::optional<double> linked_cursor_value(const CaptureSnapshot&,const std::string&,double time,double maximum_age,const CaptureRow* context=nullptr);
