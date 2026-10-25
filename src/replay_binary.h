#pragma once
#include "binary_frame.h"
#include "replay_storage.h"
// Owned by one replay file job. A new object is used after every seek/settings epoch.
class ReplayBinary {
public:
    BinaryFrameDecoder decoder;
    std::vector<BinaryFrame> feed(const ReplayRecord&,std::string_view);
private:uint64_t generation_=UINT64_MAX;
};
