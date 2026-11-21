#pragma once
#include "numeric_watch.h"
struct WatchAlarm {
    uint64_t id=0,tick=0,snooze_until=0;
    NumericWatchEvent event;
    bool active=true,acknowledged=false;
};
class WatchAlarms {
public:
    void observe(const NumericWatchEvent&,uint64_t);
    bool acknowledge(uint64_t);
    bool snooze(uint64_t,uint64_t);
    void resolve_all();
    std::vector<NumericWatchEvent> reminders(uint64_t);
    const std::vector<WatchAlarm>& rows()const{return rows_;}
private:
    std::vector<WatchAlarm> rows_;
    uint64_t next_=0;
};
