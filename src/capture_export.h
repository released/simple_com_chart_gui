#pragma once

#include "capture_store.h"
#include <filesystem>

enum class CaptureFormat { Text, Csv, Xls };
CaptureFormat capture_format(const std::filesystem::path& path);
// Writes a sibling temporary file, then replaces the destination only on success.
// A failed export leaves an existing destination intact. Throws std::exception.
void export_capture(const CaptureSnapshot& data, const std::filesystem::path& path, CaptureFormat format);
