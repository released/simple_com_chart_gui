#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <cstdint>
struct TxPacing {unsigned byte_ms=0,line_ms=0;};
bool valid_tx_pacing(TxPacing);
// Runs on the serial writer only. Callbacks preserve owned transport/cancellation.
size_t paced_write(std::string_view,TxPacing,const std::function<size_t(std::string_view,std::wstring&)>&,
    const std::function<bool()>& current,std::wstring& error);
