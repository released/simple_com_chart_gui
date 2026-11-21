#include "watch_alarms.h"
void WatchAlarms::observe(const NumericWatchEvent& event,uint64_t tick){
    const bool active=event.state=="Above maximum"||event.state=="Below minimum"||event.state=="Data unavailable";
    for(auto& row:rows_)if(row.active&&row.event.source==event.source&&row.event.key==event.key){
        if(active&&row.event.state==event.state){row.event=event;return;}
        row.active=false;row.snooze_until=0;
    }
    if(active){if(rows_.size()==128)rows_.erase(rows_.begin());WatchAlarm row;row.id=++next_;row.tick=tick;row.event=event;rows_.push_back(std::move(row));}
}
bool WatchAlarms::acknowledge(uint64_t id){for(auto& row:rows_)if(row.id==id){row.acknowledged=true;row.snooze_until=0;return true;}return false;}
bool WatchAlarms::snooze(uint64_t id,uint64_t tick){for(auto& row:rows_)if(row.id==id&&row.active&&!row.acknowledged&&tick<=UINT64_MAX-60000){row.snooze_until=tick+60000;return true;}return false;}
void WatchAlarms::resolve_all(){for(auto& row:rows_){row.active=false;row.snooze_until=0;}}
std::vector<NumericWatchEvent> WatchAlarms::reminders(uint64_t tick){std::vector<NumericWatchEvent> result;for(auto& row:rows_)if(row.active&&!row.acknowledged&&row.snooze_until&&tick>=row.snooze_until){row.snooze_until=0;result.push_back(row.event);}return result;}
