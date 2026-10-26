#pragma once
#include "command_panel.h"
#include "wire_codec.h"
#include <map>
#include <array>
using PacketParameterRow=std::array<std::string,4>;
std::vector<PacketParameterRow> packet_parameters(std::string_view);
std::string encode_packet_parameters(const std::vector<PacketParameterRow>&);

struct PacketDefinition {
    PanelCommand command;
    // CSV rows: name,value,minimum,maximum. Reserved: length and sequence.
    std::string parameters;
    uint32_t sequence=0;
    WireCodec codec=WireCodec::None;
    std::optional<CrcSpec> custom_crc;
    size_t checksum_from=0,checksum_count=0;
};
bool packet_payload(const PacketDefinition&,std::string&,std::string&);
std::string encode_packet(const PacketDefinition&);
bool decode_packet(std::string_view,PacketDefinition&,std::string&);
