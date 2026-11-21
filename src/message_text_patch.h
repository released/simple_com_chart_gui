#pragma once
#include <algorithm>
#include <optional>
#include <string_view>

struct MessageTextPatch { size_t erase_head=0, shared=0; };

// UTF-16 presentation positions only, never RX identities or navigation offsets.
// A candidate is usable only when its entire retained suffix equals the target
// prefix. Repeated content can choose any such edit: the result is still exact.
inline std::optional<MessageTextPatch> message_text_patch(std::wstring_view before,std::wstring_view after) {
    constexpr size_t limit=1024*1024,head_limit=65536,probe_size=32,attempt_limit=8;
    if(before.size()>limit||after.size()>limit)return {};
    if(before==after)return MessageTextPatch{0,before.size()};
    if(before.size()<probe_size||after.size()<probe_size)return {};
    const auto first=std::max<size_t>(1,before.size()>after.size()?before.size()-after.size():0);
    if(first>head_limit)return {};
    const auto searchable=before.substr(0,std::min(before.size(),head_limit+probe_size));
    const auto probe=after.substr(0,probe_size);
    auto at=searchable.find(probe,first);
    const auto low=[](wchar_t c){return c>=0xDC00&&c<=0xDFFF;};
    for(size_t attempt=0;at!=searchable.npos&&at<=head_limit&&attempt<attempt_limit;++attempt){
        const auto shared=before.size()-at;
        if(shared<=after.size()&&!low(before[at])&&!(shared<after.size()&&low(after[shared]))&&
            before.substr(at)==after.substr(0,shared))return MessageTextPatch{at,shared};
        at=searchable.find(probe,at+1);
    }
    return {};
}
