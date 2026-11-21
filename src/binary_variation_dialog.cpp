#include "binary_inspector.h"
#include "monitor_analysis.h"
#include "byte_comparison.h"
#include "discovery_list.h"
#include "uart_tool_form.h"
#include "ui_localized_controls.h"
#include "uart_files.h"
#include <algorithm>

void BinaryInspector::variation_frames(){
    std::vector<ByteVariation> prepared;size_t first=0,last=0;
    UartToolForm form(L"Frame byte variation",L"Select 1-based consecutive snapshot records: 2..256, identical length/direction/generation, at most4096 bytes each and1 MiB total. Offset0 aligns with offset0; no automatic alignment. Read chunks are not necessarily protocol frames. Only the retained/decoded snapshot is analyzed; source bytes and schemas remain unchanged.",{{L"First frame",L"1"},{L"Last frame",std::to_wstring(std::min<size_t>(256,frames_.size()))}},{},
        [&](const auto& f,std::wstring& error){ByteComparisonRequest request;if(!parse_byte_comparison_request({f[0],f[1],L"0",L"0"},frames_.size(),request)){error=ui::tr(L"Enter valid whole frame numbers.");return false;}try{prepared=monitor_variation(frames_,request.frame_a,request.frame_b);first=request.frame_a;last=request.frame_b;return true;}catch(const std::exception& e){error=ui::tr(uart_wide(e.what()));return false;}},this);
    form.accept_caption(L"Analyze snapshot");if(form.DoModal()!=IDOK)return;
    DiscoveryListDialog::Rows rows;
    for(const auto& r:prepared){std::vector<std::wstring> row{std::to_wstring(r.offset),std::to_wstring(r.distinct),r.distinct>1?ui::tr(L"Changed"):ui::tr(L"Fixed")};for(auto n:r.transitions)row.push_back(std::to_wstring(n));rows.push_back(std::move(row));}
    DiscoveryListDialog list(this,L"Frame byte variation",{L"Byte offset",L"Distinct values",L"Variation",L"Bit0 flips",L"Bit1 flips",L"Bit2 flips",L"Bit3 flips",L"Bit4 flips",L"Bit5 flips",L"Bit6 flips",L"Bit7 flips"},std::move(rows),std::to_wstring(last-first+1)+L" "+ui::tr(L"snapshot records; transitions compare adjacent selected records. Statistical patterns do not identify protocol fields."),L"Close");list.DoModal();
}
