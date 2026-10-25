#pragma once
#include "binary_frame.h"
#include <map>
struct BitField {BinaryField field;unsigned first=0,width=1;std::map<uint32_t,std::string> names;};
struct InspectionRow {std::string name,type,value;size_t offset=0,count=0;bool valid=false;};
bool parse_bit_fields(std::string_view,std::vector<BitField>&,std::string&);
std::vector<InspectionRow> inspect_binary(std::string_view,const BinaryFrameConfig&,const std::vector<BitField>&);
std::string inspect_checksum(std::string_view,const BinaryFrameConfig&);
