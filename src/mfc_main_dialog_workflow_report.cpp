#include "mfc_main_dialog.h"
#include "workflow_dialogs.h"
#include "workflow_report.h"
#include <sstream>
#include <cmath>
void CMainDialog::lab_reports_request(const UartLabWindow::Request& r){if(r.fields.size()!=4)return;
    if(r.action==0){std::string keys;for(const auto& key:model_.get_keys())if(model_.is_enabled(key)){if(!keys.empty())keys+='\n';keys+=key;}lab_.field(r.page,1,uart_wide(keys));return;}
    auto snapshot=capture_.snapshot();if(snapshot.rows.empty())throw std::runtime_error("No retained capture to report.");std::vector<std::string> fields;std::istringstream lines(message_utf8(r.fields[1]));std::string key;while(std::getline(lines,key)){if(!key.empty()&&key.back()=='\r')key.pop_back();if(!key.empty())fields.push_back(key);if(fields.size()>8)throw std::runtime_error("Select at most eight fields.");}if(fields.empty())throw std::runtime_error("Select at least one exact field.");double first=0,last=snapshot.rows.back()->seconds;if(!uart_number(r.fields[2],0,1e15,first)||(!r.fields[3].empty()&&!uart_number(r.fields[3],0,1e15,last))||first>last)throw std::runtime_error("Enter an increasing capture-time interval.");
    const auto path=pick_workflow_file(L"html",L"Static HTML analysis report (*.html)|*.html||");if(!path)return;const auto title=message_utf8(r.fields[0]);start_lab_file(r.page,r.action,[snapshot=std::move(snapshot),fields,title,first,last,path=*path](const std::atomic<bool>& cancel){const auto bytes=workflow_html(snapshot,fields,title,first,last,cancel);uart_write_file(path,bytes,true,cancel);return LabFileResult{"Static HTML/SVG report saved. Selected retained raw numeric snapshot only; no scripts or complete raw archive.",{}};});
}
