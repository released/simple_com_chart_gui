#include "binary_inspector.h"
#include "byte_comparison.h"
#include "discovery_list.h"
#include "uart_tool_form.h"
#include "uart_files.h"
#include "ui_language.h"
#include <cmath>
#include <algorithm>
void BinaryInspector::compare_frames(){
    if(frames_.empty())return;const auto current=static_cast<size_t>(std::max<LRESULT>(0,::SendMessageW(pick_,CB_GETCURSEL,0,0)));
    std::vector<std::wstring> choices;for(size_t i=0;i<frames_.size();++i)choices.push_back(std::to_wstring(i+1));
    ByteComparisonRequest request;
    UartToolForm form(L"Compare retained frames",L"Two immutable snapshots, at most4096 bytes each. Frame numbers are 1-based; offsets are decimal. Explicit byte alignment only; no insertion/deletion inference, TX or source edits. Decoded codec snapshots use decoded byte offsets.",
        {{L"Frame A",std::to_wstring(current+1),choices},{L"Frame B",L"1",choices},{L"A byte offset",L"0"},{L"B byte offset",L"0"}},{},[this,&request](const auto& f,std::wstring& e){ByteComparisonRequest next;if(!parse_byte_comparison_request({f[0],f[1],f[2],f[3]},frames_.size(),next)){e=ui::tr(L"Enter decimal whole frame numbers and byte offsets (digits only).");return false;}try{compare_bytes(frames_[next.frame_a].bytes,frames_[next.frame_b].bytes,next.offset_a,next.offset_b);request=next;return true;}catch(const std::exception& ex){e=uart_wide(ex.what());return false;}},this);
    form.accept_caption(L"Compare snapshots");if(form.DoModal()!=IDOK)return;const size_t a=request.frame_a,b=request.frame_b,x=request.offset_a,y=request.offset_b;const auto differences=compare_bytes(frames_[a].bytes,frames_[b].bytes,x,y);
    const auto fields_a=inspect_binary(frames_[a].bytes,config_,bits_),fields_b=inspect_binary(frames_[b].bytes,config_,bits_);DiscoveryListDialog::Rows rows;
    for(const auto& d:differences){const auto hex=[](int byte){return byte<0?L"--":uart_wide(uart_hex(std::string(1,static_cast<char>(byte))));};std::wstring fields;
        for(const auto& row:fields_a)if(x+d.offset>=row.offset&&x+d.offset-row.offset<row.count)fields+=L"A:"+uart_wide(row.name)+L" ";
        for(const auto& row:fields_b)if(y+d.offset>=row.offset&&y+d.offset-row.offset<row.count)fields+=L"B:"+uart_wide(row.name)+L" ";
        rows.push_back({std::to_wstring(x+d.offset),hex(d.a),std::to_wstring(y+d.offset),hex(d.b),fields});}
    DiscoveryListDialog result(this,L"Frame byte differences",{L"A offset",L"A HEX",L"B offset",L"B HEX",L"Affected fields"},std::move(rows),std::to_wstring(differences.size())+L" "+ui::tr(L"different byte positions; -- means absent. Complete bounded snapshots; no source changes."),L"Close");result.DoModal();
}
