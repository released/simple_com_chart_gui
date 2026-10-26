#include "uart_lab_window.h"
#include "ui_localized_controls.h"
void UartLabWindow::build_pages() {
    fields_[Recording]={{L"Source .bin / rotation segment",L""},{L"Synthetic interval (ms)",L"100"},{L"Bounds",L"Records",{L"Records",L"Seconds"}},{L"First (inclusive)",L"0"},{L"Past-last record / end time",L""},{L"NEW output .bin",L""}};
    fields_[Packet]={{L"Target",L"Port A",{L"Port A",L"Port B"}},{L"Format",L"HEX",{L"HEX",L"Text UTF-8"}},{L"Template",L"",{},3},{L"name,value,min,max",L"",{},4},{L"Ending",L"None",{L"None",L"CR",L"LF",L"CRLF"}},{L"Checksum",L"None",{L"None",L"Sum8",L"XOR8",L"CRC16 Modbus",L"CRC16 CCITT-FALSE",L"CRC32"}},{L"Checksum byte order",L"Little",{L"Little",L"Big"}},{L"Sequence (manual)",L"0"},{L"Wire codec",L"None",{L"None",L"COBS",L"SLIP"}},{L"Definition name",L"Packet"}};
    fields_[Quality]={{L"Source",L"Port A",{L"Port A",L"Port B"}},{L"Exact numeric key",L""},{L"Sequence modulus",L"65536"},{L"Expected step",L"1"}};
    fields_[Parser]={{L"Parser",L"Key/value",{L"Key/value",L"CSV",L"Tab",L"JSON",L"Regex"}},{L"Columns / JSON ptr:name=/path",L""},{L"Regex",L""},{L"Name group",L"1"},{L"Value group",L"2"},{L"Time group (0=receipt)",L"0"},{L"Time field",L""},{L"Cases CSV",L"",{},5},{L"Absolute tolerance",L"0"}};
    fields_[Schema]={{L"Target",L"Port A",{L"Port A",L"Port B",L"Offline replay"}}};
    fields_[Digital]={{L"name,key,first-bit,width",L"",{},5},{L"Window (s)",L"30"},{L"Maximum hold (s)",L"2"}};
    fields_[Sessions]={{L"Completed source .bin",L""},{L"Manifest .sccsession",L""},{L"Synthetic interval (ms)",L"100"},{L"Settings snapshot",L"Capture-start metadata",{L"Capture-start metadata",L"Current reviewed settings"}},{L"Include readable TX/RX .log",L"No",{L"No",L"Yes"}}};
    fields_[Devices]={{L"Search (COM / VID / name)",L""}};
    fields_[Views]={{L"View name",L"view_1"}};
    fields_[Images]={{L"Plots",L"Chart",{L"Chart",L"4 Charts"}},{L"PNG width",L"1280"},{L"PNG height",L"800"}};
    fields_[Notes]={{L"Completed source file",L""},{L"Byte offset",L"0"},{L"Byte count",L"0"},{L"Note text",L"",{},3},{L"Note index (0 based)",L"0"}};
    fields_[Reports]={{L"Title",L"UART analysis"},{L"Fields (one exact key per line)",L"",{},4},{L"First capture second",L"0"},{L"Last capture second (empty=latest)",L""}};
    fields_[Waterfall]={{L"Exact retained numeric key",L""},{L"FFT samples",L"256"},{L"Sample rate Hz (0=clocks)",L"0"},{L"History rows",L"128"},{L"Minimum dB",L"-100"},{L"Maximum dB",L"0"}};
    fields_[Input]={{L"Source",L"Port A",{L"Port A",L"Port B",L"Offline replay"}},{L"Framing",L"LF",{L"LF",L"CR",L"CRLF",L"Custom delimiter",L"Receipt idle gap"}},{L"Delimiter HEX",L""},{L"Idle gap (ms)",L"20"}};
    fields_[Checksums]={{L"CRC width:poly:init:xor:in:out",L"16:1021:FFFF:0:0:0"},{L"Input HEX (preview only)",L"",{},3},{L"First byte",L"0"},{L"Covered count (0 = rest)",L"0"},{L"Output byte order",L"Little",{L"Little",L"Big"}}};
    fields_[Calculus]={{L"Source",L"Port A",{L"Port A",L"Port B"}},{L"Exact retained numeric key",L""},{L"Operation",L"Derivative",{L"Derivative",L"Integral"}},{L"Maximum sample gap (s)",L"2"}};
    fields_[Transactions]={{L"Source",L"Port A",{L"Port A",L"Port B"}},{L"Framing",L"LF",{L"LF",L"CRLF",L"Fixed bytes",L"Receipt idle gap",L"Header + fixed"}},{L"Frame bytes",L"16"},{L"Idle gap (ms)",L"20"},{L"Header HEX",L""},{L"TX request prefix HEX",L""},{L"RX reply prefix HEX",L""},{L"Request ID byte offset",L"1"},{L"Reply ID byte offset",L"1"},{L"ID bytes",L"1",{L"1",L"2",L"4"}},{L"ID byte order",L"Little",{L"Little",L"Big"}},{L"Timeout (ms)",L"1000"},{L"Result row (0 based)",L"0"}};
    fields_[Packet].push_back({L"Custom CRC (empty = standard)",L""});fields_[Packet].push_back({L"Custom CRC first byte",L"0"});fields_[Packet].push_back({L"Custom CRC count (0 = rest)",L"0"});
    const std::array<std::vector<const wchar_t*>,PageCount> labels{{
        {L"Refresh",L"Reset A text counters",L"Reset B text counters",L"A extraction...",L"B text parser...",L"Binary schemas..."},
        {L"Browse source...",L"Check selected segment",L"Export range",L"Salvage valid prefix",L"Browse output...",L"Cancel file work"},
        {L"Preview",L"Send once...",L"Load definition...",L"Save definition...",L"Edit parameters..."},
        {L"Apply sequence settings",L"Reset sequence counters",L"Refresh",L"Save statistics..."},
        {L"Use current parser",L"Run cases",L"Import cases...",L"Export cases...",L"Use selected capture row..."},
        {L"Edit selected schema..."},
        {L"Apply lanes",L"Refresh",L"Reset to none"},
        {L"Browse source...",L"Open manifest...",L"New manifest...",L"Create manifest",L"Verify files",L"Verify and stage settings",L"Review staged...",L"Cancel file work"},
        {L"Refresh device list",L"Copy displayed information"},
        {L"Load saved source",L"Apply framing"},
        {L"Calculate preview",L"Use for packet",L"Show binary directive"},
        {L"Calculate retained snapshot",L"Save result CSV..."},
        {L"Analyze retained traffic",L"Inspect request",L"Inspect reply",L"Save transaction CSV..."},
        {L"Save current view",L"Stage named view",L"Import view...",L"Export staged view...",L"Apply staged view",L"Cancel file work"},
        {L"Save chart PNG...",L"Cancel file work"},
        {L"Browse source...",L"Add note",L"Load notes...",L"Save notes...",L"Locate selected note",L"Clear staged notes",L"Cancel file work"},
        {L"Use checked fields",L"Save HTML report...",L"Cancel file work"},
        {L"Start waterfall",L"Stop waterfall",L"Clear waterfall"}}};
    const auto make=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int id){return ui::create_window(cls,text,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,0,0,1,1,viewport_.GetSafeHwnd(),reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),nullptr,nullptr);};
    for(int p=0;p<PageCount;++p) {
        for(size_t i=0;i<fields_[p].size();++i) {
            auto& f=fields_[p][i];f.label=make(L"STATIC",f.name.c_str(),SS_NOPREFIX|SS_CENTERIMAGE,0);
            f.input=make(f.choices.empty()?L"EDIT":L"COMBOBOX",f.choices.empty()?f.initial.c_str():L"",f.choices.empty()?WS_BORDER|(f.rows>1?ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL|WS_VSCROLL:ES_AUTOHSCROLL):CBS_DROPDOWNLIST|WS_VSCROLL,FieldId+p*32+static_cast<int>(i));
            if(f.choices.empty())::SendMessageW(f.input,EM_SETLIMITTEXT,f.rows>1?65536:2048,0);
            else {ui::choices(f.input);for(const auto& choice:f.choices)::SendMessageW(f.input,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(choice.c_str()));::SendMessageW(f.input,CB_SETCURSEL,0,0);}
        }
        for(size_t a=0;a<labels[p].size();++a)actions_[p].push_back(make(L"BUTTON",labels[p][a],BS_PUSHBUTTON,ActionId+p*16+static_cast<int>(a)));
    }
}
