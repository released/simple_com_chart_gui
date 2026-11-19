#include "data_page.h"
#include "linked_cursor.h"
#include "ui_language.h"
#include <sstream>
#include <cmath>
void DataPage::show_linked_cursor(bool enabled,double time,double age){
    linked_enabled_=enabled;linked_time_=time;linked_age_=age;const auto context=enabled?selected_row():std::shared_ptr<const CaptureRow>{};linked_context_=context;linked_sequence_=context?context->sequence:0;linked_expired_=enabled&&!context;render_linked_cursor();
}
void DataPage::render_linked_cursor(){
    const auto data=linked_enabled_?capture_->snapshot():CaptureSnapshot{};
    const auto context=linked_context_.lock();
    for(size_t i=0;i<4;++i){const auto value=linked_enabled_&&!linked_expired_&&context?linked_cursor_value(data,selected_[i],linked_time_,linked_age_,context.get()):std::optional<double>{};std::wostringstream text;text<<ui::tr(L"Linked time")<<L" "<<(data.rows.empty()?0:linked_time_-(data.rows.front()->source_seconds-data.rows.front()->seconds))<<L" s | ";const double shown=value?(display_?display_->project(selected_[i],*value):*value):0;if(value&&std::isfinite(shown))text<<shown;else text<<L"n/a";plots_[i].set_linked_cursor(linked_enabled_,linked_time_,text.str());}
}
