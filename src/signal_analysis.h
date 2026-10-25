#pragma once
#include <vector>
#include <utility>
#include <string>
struct Spectrum {std::vector<std::pair<double,double>> bins;double sample_rate=0,peak_hz=0,peak_amplitude=0;std::string provenance;};
// Hann-windowed one-sided amplitude spectrum; zero manual_rate validates source clocks.
bool signal_fft(const std::vector<std::pair<double,double>>& samples,size_t count,double manual_rate,Spectrum&,std::string& error);
