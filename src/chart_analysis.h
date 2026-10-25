#pragma once
#include "capture_store.h"
struct ChartAnalysis {
    size_t count = 0;
    size_t retained_count = 0;
    double minimum = 0, maximum = 0, mean = 0, rms = 0, stddev = 0, a = 0, b = 0, delta = 0;
};
ChartAnalysis analyze_capture(const CaptureSnapshot& data, size_t column, double a, double b);
