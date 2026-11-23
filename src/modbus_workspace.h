#pragma once
#include "modbus_rtu.h"

struct ModbusPoint {
    std::string name,unit,type="u16",order="ABCD";
    RtuConfig request;
    double scale=1,offset=0;
};
bool validate_modbus_points(const std::vector<ModbusPoint>&,std::string&);
std::vector<RtuConfig> modbus_plan(const std::vector<ModbusPoint>&);
std::string encode_modbus_points(const std::vector<ModbusPoint>&);
std::vector<ModbusPoint> decode_modbus_points(std::string_view);
bool modbus_address(unsigned function,std::string_view,bool reference,unsigned&,std::string&);
unsigned modbus_reference(unsigned function,unsigned offset);
const char* rtu_outcome_name(RtuOutcome);
std::string modbus_transactions_csv(const std::deque<RtuTransaction>&);
