#pragma once
#include "capture_store.h"
#include "field_extractor.h"
#include "binary_frame.h"
#include <atomic>
#include <filesystem>
struct RunOptions {ExtractionConfig extraction;bool binary=false;BinaryFrameConfig schema;unsigned interval_ms=100;std::string include,exclude;bool regex=false;};
struct RecordedRun {CaptureSnapshot data;std::string name,provenance;};
RecordedRun load_recorded_run(const std::filesystem::path&,const RunOptions&,const std::atomic<bool>& cancel);
std::vector<std::pair<double,double>> xy_pairs(const CaptureSnapshot&,const std::string& x,const std::string& y,size_t limit=2000);
struct RunMatch {double time_a=0,time_b=0,a=0,b=0;bool pass=false;};
struct RunComparison {std::vector<RunMatch> matches;size_t unmatched_a=0,unmatched_b=0,passed=0;};
RunComparison compare_runs(const RecordedRun&,const RecordedRun&,const std::string&,double shift,double max_age,double tolerance);
std::string comparison_csv(const RunComparison&);
