#pragma once
#include "uart_operations.h"
#include <array>

std::wstring encode_tx_entry(const TxEntry& value);
bool decode_tx_entry(const std::wstring& encoded, TxEntry& value);
SerialControls load_uart_controls(const std::wstring& ini, std::wstring& error);
bool save_uart_controls(const std::wstring& ini, const SerialControls& controls);
std::vector<TxEntry> load_tx_history(const std::wstring& ini);
bool save_tx_history(const std::wstring& ini, const std::vector<TxEntry>& history);
std::array<TxEntry, 8> load_tx_presets(const std::wstring& ini);
bool save_tx_preset(const std::wstring& ini, size_t slot, const TxEntry* value);
