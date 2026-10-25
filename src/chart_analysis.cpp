#include "chart_analysis.h"
#include <algorithm>
#include <cmath>
#include <limits>
ChartAnalysis analyze_capture(const CaptureSnapshot& data, size_t column, double a, double b) {
    ChartAnalysis result; double nearest_a = std::numeric_limits<double>::infinity(), nearest_b = nearest_a;
    std::vector<double> values; double scale=0;
    for (const auto& row : data.rows) {
        double value = 0; if (!CaptureStore::value(*row,column,value)) continue;
        ++result.retained_count;
        if (std::abs(row->seconds-a) < nearest_a) { nearest_a = std::abs(row->seconds-a); result.a = value; }
        if (std::abs(row->seconds-b) < nearest_b) { nearest_b = std::abs(row->seconds-b); result.b = value; }
        if (row->seconds < std::min(a,b) || row->seconds > std::max(a,b)) continue;
        if (!result.count) result.minimum = result.maximum = value;
        result.minimum = std::min(result.minimum,value); result.maximum = std::max(result.maximum,value);
        ++result.count; values.push_back(value); scale=std::max(scale,std::abs(value));
    }
    if (result.count && scale) {
        double mean=0,m2=0,squares=0; size_t n=0;
        for (const auto value:values) { const double normalized=value/scale, change=normalized-mean; mean+=change/++n; m2+=change*(normalized-mean); squares+=normalized*normalized; }
        result.mean=mean*scale; result.stddev=std::sqrt(std::max(0.0,m2/result.count))*scale; result.rms=std::sqrt(std::min(1.0,squares/result.count))*scale;
    }
    result.delta = result.b-result.a; return result;
}
