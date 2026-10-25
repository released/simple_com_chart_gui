#include "mfc_main_dialog.h"
#include <cmath>

#ifndef CHART_RUNTIME_CHECKS
#error Synthetic telemetry must not be compiled into the delivery application.
#endif

// Deterministic input for isolated UI checks, never a delivery input source.
void CMainDialog::feed_demo() {
    if (++demo_tick_ % 4 != 0) return;
    const double phase = static_cast<double>(demo_tick_) / 12.0;
    const auto line = std::string("fan,duty:920,target:2340,pri_ntc:76,fg_hz:") +
        std::to_string(69.1 + 3 * std::sin(phase)) + ",fg_rpm:" + std::to_string(2074 + 80 * std::sin(phase)) +
        ",offset:" + std::to_string(-2.5 + std::sin(phase)) + ",err:11%,cap:105/76/29,tick:612/609/2453,cmd:0x3202";
    std::lock_guard<std::mutex> lock(pending_mutex_);
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    const double now = static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
    pending_.lines.push_back({line, now});
    const std::string ac = "ac_adc_mv:1169,calibrated:1169,ac_v:116,ocp_mv:377,ntc_mv:2439,ntc_c:76,cnt:554";
    pending_.lines.push_back({ac, now});
    pending_.raw.append(line + "\r\n" + ac + "\r\n");
}
