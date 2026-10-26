#pragma once
#include "traffic_framer.h"
#include <atomic>
#include <optional>
struct TransactionOptions {
    TrafficFraming framing;
    std::string request_prefix,reply_prefix;
    size_t request_id=1,reply_id=1,width=1;
    bool big=false;double timeout=1;
    unsigned source=0;
};
struct TransactionRow {uint32_t id=0;std::optional<TrafficRecord> request,reply;std::string status;double milliseconds=0;};
struct TransactionResult {std::vector<TransactionRow> rows;std::string report,csv;bool truncated=false;};
TransactionResult analyze_transactions(const TrafficBatch&,const TransactionOptions&,double now,const std::atomic<bool>&);
std::string transaction_detail(const TransactionResult&,size_t row,bool reply);
