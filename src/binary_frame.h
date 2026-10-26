#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <deque>
#include "wire_codec.h"
#include "custom_crc.h"
#include <optional>

enum class FrameChecksum {None,Sum8,Xor8,Modbus16,Ccitt16,Crc32};
uint32_t frame_checksum(std::string_view bytes,FrameChecksum kind);
size_t frame_checksum_width(FrameChecksum kind);
struct BinaryField {std::string name,type;size_t offset=0;double scale=1,add=0;};
struct BinaryVariant {uint32_t id=0;std::vector<BinaryField> fields;};
size_t binary_field_width(const std::string& type);
bool read_binary_field(std::string_view,const BinaryField&,double& value);
struct BinaryFrameConfig {
    std::string header;
    size_t fixed_bytes=0,length_offset=0,length_width=1,length_add=0,checksum_from=0;
    bool length_big_endian=false,checksum_big_endian=false;
    FrameChecksum checksum=FrameChecksum::None;
    std::vector<BinaryField> fields;
    WireCodec codec=WireCodec::None;
    size_t id_offset=0,id_width=1;
    bool id_big_endian=false;
    std::vector<BinaryVariant> variants;
    std::optional<CrcSpec> custom_crc;
    size_t checksum_count=0;
};
size_t binary_checksum_width(const BinaryFrameConfig&);
uint32_t binary_checksum(std::string_view,const BinaryFrameConfig&);
bool parse_binary_layout(std::string_view,BinaryFrameConfig&,std::string&);
bool parse_binary_fields(std::string_view csv,std::vector<BinaryField>& fields,std::string& error);
bool validate_binary_config(const BinaryFrameConfig&,std::string& error);
struct BinaryFrame {
    std::string bytes;
    int64_t receipt_ms=0;
    std::vector<std::pair<std::string,double>> values;
    double sample_seconds=0;
    uint64_t generation=UINT64_MAX;
};
class BinaryFrameDecoder {
public:
    bool configure(BinaryFrameConfig config,std::string& error);
    std::vector<BinaryFrame> feed(std::string_view bytes,int64_t receipt_ms,double sample_seconds=0);
    void reset();
    void interrupt(){pending_.clear();receipts_.clear();wire_.reset();}
    size_t pending_bytes()const{return pending_.size()+wire_.pending_bytes();}
    const BinaryFrameConfig& config()const{return config_;}
    uint64_t discarded()const{return discarded_;}
    uint64_t checksum_errors()const{return checksum_errors_;}
    uint64_t layout_errors()const{return layout_errors_;}
    uint64_t codec_errors()const{return wire_.errors();}
    uint64_t unknown_types()const{return unknown_types_;}
private:
    std::vector<BinaryFrame> feed_plain(std::string_view,int64_t,double);
    WireDecoder wire_;
    std::optional<CustomCrc> custom_crc_;
    BinaryFrameConfig config_;
    std::string pending_;
    struct Receipt {size_t count;int64_t ms;double seconds;};
    std::deque<Receipt> receipts_;
    uint64_t discarded_=0,checksum_errors_=0,layout_errors_=0;
    uint64_t unknown_types_=0;
    bool configured_=false;
};
