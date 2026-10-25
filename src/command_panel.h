#pragma once
#include "uart_operations.h"
#include "binary_frame.h"
#include <vector>

struct PanelCommand {
    std::string group,name,body;
    bool hex=false,checksum_big=false;
    TxEnding ending=TxEnding::None;
    FrameChecksum checksum=FrameChecksum::None;
    double minimum=0,maximum=100,step=1,value=0;
};
bool panel_payload(const PanelCommand&,double value,std::string& bytes,std::string& error);
bool validate_panel(const std::vector<PanelCommand>&,std::string& error);
std::string encode_panel(const std::vector<PanelCommand>&);
bool decode_panel(std::string_view,std::vector<PanelCommand>&,std::string& error);
// Bounded RFC4180-style rows for versioned native command/step definitions.
bool uart_csv_rows(std::string_view,std::vector<std::vector<std::string>>&,std::string& error);
bool uart_csv_table(std::string_view,std::vector<std::vector<std::string>>&,std::string& error,size_t max_bytes,size_t max_rows,size_t max_columns);
std::string uart_csv_cell(std::string_view);
