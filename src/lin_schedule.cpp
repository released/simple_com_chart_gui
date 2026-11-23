#include "lin_protocol.h"

bool validate_lin_schedule(const LinSchedule& schedule,unsigned cycles,std::string& error) {
    if(schedule.name.empty()||schedule.name.size()>64||schedule.slots.empty()||schedule.slots.size()>256||cycles<1||cycles>10000) {
        error="LIN schedule requires a name, 1..256 slots and 1..10000 cycles.";return false;
    }
    uint64_t duration=0;
    for(const auto& slot:schedule.slots) {
        if(slot.id>59||slot.delay_ms<1||slot.delay_ms>60000||slot.data.size()>8||
           (slot.publish&&slot.data.empty())||(!slot.publish&&!slot.data.empty())) {
            error="Only unconditional LIN slots with ID 00..3B, 1..60000 ms and explicit publisher data are supported.";return false;
        }
        duration+=slot.delay_ms;
    }
    if(duration*cycles>3600000){error="LIN schedule exceeds one hour.";return false;}
    return true;
}
bool LinScheduler::start(const LinSchedule& schedule,unsigned cycles,LinTransport* transport,uint64_t now,std::string& error) {
    if(active_||ticket_){error="Wait for the previous LIN operation to drain.";return false;}
    if(!validate_lin_schedule(schedule,cycles,error))return false;
    if(!transport){error="No LIN hardware adapter is configured.";return false;}
    const auto capabilities=transport->capabilities();
    if(!capabilities.hardware_schedule||!capabilities.header){error="The LIN adapter does not provide hardware scheduling and headers.";return false;}
    for(const auto& slot:schedule.slots)if(slot.publish&&!capabilities.publish){error="The LIN adapter cannot publish responses.";return false;}
    const auto generation=transport->generation();uint64_t ticket=0;
    if(!transport->schedule(schedule,cycles,ticket,error)||!ticket)return false;
    transport_=transport;generation_=generation;ticket_=ticket;schedule_=schedule;cycles_=cycles;
    uint64_t duration=0;for(const auto& slot:schedule.slots)duration+=slot.delay_ms;
    started_=now;deadline_=now+duration*cycles+1000;active_=true;status_="Hardware schedule running";
    if(transport_->generation()!=generation_){stop();status_="LIN connection changed; cancelled, waiting for completion";return false;}
    return true;
}
void LinScheduler::poll(uint64_t now) {
    if(!ticket_||!transport_)return;
    LinCompletion result;
    if(transport_->take(ticket_,result)) {
        const bool was_active=active_;ticket_=0;active_=false;
        status_=!was_active?"LIN operation drained; no automatic resume":
            result.generation!=generation_?"Stale LIN completion discarded":
            result.cancelled?"LIN operation cancelled":
            !result.success||result.partial?"LIN operation failed or partial; no retry":"Hardware schedule completed";
        return;
    }
    if(active_&&(transport_->generation()!=generation_||now>=deadline_)) {
        stop();status_="LIN timeout or connection change; cancelling, outcome may be unknown";
    }
}
void LinScheduler::stop() {
    if(!active_)return;active_=false;
    if(ticket_&&transport_)transport_->cancel(ticket_);
    status_=ticket_?"LIN cancellation pending; accepted effects cannot be revoked":"LIN stopped";
}
