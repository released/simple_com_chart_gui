#pragma once
#include "plot_view.h"
#include <cstdint>
#include <functional>
#include <atomic>
struct PlotRaster {int width=0,height=0;std::vector<uint32_t> pixels;};
PlotRaster raster_plots(const std::vector<PlotView*>&,int,int);
std::string encode_plot_png(const PlotRaster&,const std::atomic<bool>&);
