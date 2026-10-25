#include "mfc_main_dialog.h"
void bound_pending_lines(PendingData& pending,int& dropped) {
    size_t bytes=0;for(const auto& line:pending.lines) bytes+=line.line.size();size_t first=0;
    while(first<pending.lines.size() && (pending.lines.size()-first>2000 || bytes>4*1024*1024)) {bytes-=pending.lines[first++].line.size();}
    if(first) {dropped+=static_cast<int>(first);pending.lines.erase(pending.lines.begin(),pending.lines.begin()+first);}
}
