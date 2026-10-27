#pragma once
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace chart_time {
constexpr double minimum = 0.25;
constexpr double input_maximum = 300.0;

inline bool parse(const std::wstring& text, double& seconds) {
    if (text.empty() || text.size() > 48) return false;
    std::wistringstream input(text);
    input.imbue(std::locale::classic());
    double number = 0;
    if (!(input >> number) || !std::isfinite(number)) return false;
    std::wstring unit, extra;
    input >> unit;
    if (input >> extra) return false;
    for (auto& c : unit) if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
    if (unit == L"ms") number /= 1000;
    else if (!unit.empty() && unit != L"s") return false;
    if (number < minimum || number > input_maximum) return false;
    seconds = number;
    return true;
}

inline std::wstring label(double seconds) {
    if (seconds == 0.25) return L"250 ms";
    if (seconds == 0.5) return L"500 ms";
    std::wostringstream text;
    text.imbue(std::locale::classic());
    text << std::setprecision(12) << seconds << L" s";
    return text.str();
}

inline double tick_step(double seconds, unsigned labels) {
    const double raw = seconds / std::max(1U, labels);
    if (seconds >= 1) return std::max(1.0, std::ceil(raw));
    const double power = std::pow(10.0, std::floor(std::log10(raw)));
    const double fraction = raw / power;
    return (fraction <= 1 ? 1 : fraction <= 2 ? 2 : fraction <= 5 ? 5 : 10) * power;
}
}
