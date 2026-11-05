#pragma once
#include "message_format.h"
// Deliberately not VT emulation: CR, LF, BS and CSI 0/1/2 K only.
FormattedMessage format_light_terminal(std::string_view bytes,const MessageDisplayOptions& options,
    uint64_t offset=0,const std::vector<MessageReceipt>& receipts={},int64_t origin_ms=0);
