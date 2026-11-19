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
    UartToolForm form(L"Compare retained frames",L"Two immutable snapshots, at most4096 bytes each. Frame numbers are 1-based; offsets are decimal. Explicit byte alignment only; no insertion/deletion inference, TX or source edits. Decoded codec snapshots use decoded byte offsets.",
        {{L"Frame A",std::to_wstring(current+1),choices},{L"Frame B",L"1",choices},{L"A byte offset",L"0"},{L"B byte offset",L"0"}},{},[this](const auto& f,std::wstring& e){double a=0,b=0,x=0,y=0;if(!uart_number(f[0],1,static_cast<double>(frames_.size()),a)||!uart_number(f[1],1,static_cast<double>(frames_.size()),b)||!uart_number(f[2],0,4096,x)||!uart_number(f[3],0,4096,y)||a!=std::floor(a)||b!=std::floor(b)||x!=std::floor(x)||y!=std::floor(y)){e=ui::tr(L"Enter valid frame numbers and whole byte offsets.");return false;}try{compare_bytes(frames_[static_cast<size_t>(a)-1].bytes,frames_[static_cast<size_t>(b)-1].bytes,static_cast<size_t>(x),static_cast<size_t>(y));return true;}catch(const std::exception& ex){e=uart_wide(ex.what());return false;}},this);
    form.accept_caption(L"Compare snapshots");if(form.DoModal()!=IDOK)return;const auto f=form.values();const size_t a=std::stoull(f[0])-1,b=std::stoull(f[1])-1,x=std::stoull(f[2]),y=std::stoull(f[3]);const auto differences=compare_bytes(frames_[a].bytes,frames_[b].bytes,x,y);
    const auto fields_a=inspect_binary(frames_[a].bytes,config_,bits_),fields_b=inspect_binary(frames_[b].bytes,config_,bits_);DiscoveryListDialog::Rows rows;
    for(const auto& d:differences){const auto hex=[](int byte){return byte<0?L"--":uart_wide(uart_hex(std::string(1,static_cast<char>(byte))));};std::wstring fields;
        for(const auto& row:fields_a)if(x+d.offset>=row.offset&&x+d.offset-row.offset<row.count)fields+=L"A:"+uart_wide(row.name)+L" ";
        for(const auto& row:fields_b)if(y+d.offset>=row.offset&&y+d.offset-row.offset<row.count)fields+=L"B:"+uart_wide(row.name)+L" ";
        rows.push_back({std::to_wstring(x+d.offset),hex(d.a),std::to_wstring(y+d.offset),hex(d.b),fields});}
    DiscoveryListDialog result(this,L"Frame byte differences",{L"A offset",L"A HEX",L"B offset",L"B HEX",L"Affected fields"},std::move(rows),std::to_wstring(differences.size())+L" "+ui::tr(L"different byte positions; -- means absent. Complete bounded snapshots; no source changes."),L"Close");result.DoModal();
}
