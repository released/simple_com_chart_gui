#include "mfc_main_dialog.h"
#include "workflow_dialogs.h"
#include <set>
#include <cmath>
std::optional<std::filesystem::path> CMainDialog::pick_workflow_file(const wchar_t* extension,const wchar_t* filter,bool open){return workflow_picker_?workflow_picker_(extension,open):workflow_pick(&lab_,extension,filter,open);}
ViewPreset CMainDialog::current_view()const {
    ViewPreset view;for(const auto& key:model_.get_keys())if(model_.is_enabled(key))view.checked.push_back(key);
    for(const auto& pair:data_page_.column_checks())if(pair.second)view.columns.push_back(pair.first);
    std::sort(view.checked.begin(),view.checked.end());std::sort(view.columns.begin(),view.columns.end());
    const auto axis=[](const PlotView& plot){const auto range=plot.y_range();return ViewAxis{plot.view_window(),range.first,range.second,plot.fixed_y()};};view.axes[0]=axis(plot_view_);
    for(size_t i=0;i<4;++i){view.charts[i]=data_page_.selected_field(i);view.axes[i+1]=axis(data_page_.chart_plot(i));}return view;
}
void CMainDialog::apply_view(const ViewPreset& view){
    encode_view(view);if(snapshot_)throw std::runtime_error("Return Snapshot to Live before applying a view; capture retained.");
    const auto keys=model_.get_keys();const auto& table=capture_.keys();std::set<std::string> checked(view.checked.begin(),view.checked.end()),missing;
    std::unordered_map<std::string,bool> checks;for(const auto& k:keys){const bool enabled=checked.count(k)!=0;model_.set_enabled(k,enabled);checks[k]=enabled;}
    for(const auto& k:view.checked)if(std::find(keys.begin(),keys.end(),k)==keys.end())missing.insert(k);channel_panel_.set_checkbox_state_map(checks);std::unordered_map<std::string,double> latest;for(const auto& key:keys){double value=0;if(model_.latest_value(key,value))latest[key]=value;}channel_panel_.update_values(latest);
    checks.clear();for(const auto& k:view.columns){if(std::find(table.begin(),table.end(),k)!=table.end())checks[k]=true;else missing.insert(k);}data_page_.set_column_checks(checks);
    auto charts=view.charts;for(auto& k:charts)if(!k.empty()&&std::find(table.begin(),table.end(),k)==table.end()){missing.insert(k);k.clear();}data_page_.set_selected_fields(charts);
    const auto& main=view.axes[0];model_.set_time_window(main.window);plot_view_.set_time_window(main.window);plot_view_.set_fixed_y(main.fixed,main.low,main.high);
    wchar_t text[64]{};swprintf_s(text,L"%.6g",main.window);auto index=::SendMessageW(combo_time_,CB_FINDSTRINGEXACT,static_cast<WPARAM>(-1),reinterpret_cast<LPARAM>(text));if(index==CB_ERR)index=::SendMessageW(combo_time_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));::SendMessageW(combo_time_,CB_SETCURSEL,index,0);
    for(size_t i=0;i<4;++i){const auto& a=view.axes[i+1];data_page_.set_chart_time(i,a.window);data_page_.chart_plot(i).set_fixed_y(a.fixed,a.low,a.high);}plot_view_.update_from_model(now_seconds());OnChartSelectionChanged(0,0);
    std::string report="View applied to existing fields only. Capture, parser, ports and recording unchanged. Missing keys: "+std::to_string(missing.size());for(const auto& key:missing)report+="\r\n"+key;lab_.notice(UartLabWindow::Views,uart_wide(report));
}
void CMainDialog::lab_views_request(const UartLabWindow::Request& r){
    if(r.action==4){if(!staged_view_)throw std::runtime_error("Stage a view first.");apply_view(*staged_view_);return;}
    std::filesystem::path path;bool replace=false;
    if(r.action<2){if(r.fields.size()!=1||!valid_profile_name(r.fields[0]))throw std::runtime_error("View name: 1..48 ASCII letters/digits/_/-; no device names.");auto folder=std::filesystem::path(settings_path_).parent_path()/L"views";const auto attributes=GetFileAttributesW(folder.c_str());if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Named view folder cannot be a link.");if(r.action==0)std::filesystem::create_directories(folder);path=folder/(r.fields[0]+L".sccview");
        if(r.action==0&&std::filesystem::exists(path)){if(ui::message_box(lab_.m_hWnd,L"Replace this saved named view?",L"Save view",MB_OKCANCEL|MB_ICONQUESTION)!=IDOK)return;replace=true;}}
    else {if(r.action==3&&!staged_view_)throw std::runtime_error("Stage a view first.");const auto picked=pick_workflow_file(L"sccview",L"Private chart view (*.sccview)|*.sccview||",r.action==2);if(!picked)return;path=*picked;replace=r.action==3;}
    if(r.action==1||r.action==2)start_lab_file(r.page,r.action,[path](const std::atomic<bool>& cancel){LabFileResult result;result.view=std::make_shared<ViewPreset>(decode_view(uart_read_file(path,65536,cancel)));result.report="View staged; review checked keys and explicitly Apply. Missing keys never auto-enable.";for(const auto& k:result.view->checked)result.report+="\r\nChart: "+k;for(const auto& k:result.view->columns)result.report+="\r\nTable: "+k;for(size_t i=0;i<4;++i)result.report+="\r\nPlot "+std::to_string(i+1)+": "+(result.view->charts[i].empty()?"(none)":result.view->charts[i]);return result;});
    else {auto view=std::make_shared<ViewPreset>(r.action==0?current_view():*staged_view_);const auto encoded=encode_view(*view);start_lab_file(r.page,r.action,[path,replace,encoded,view](const std::atomic<bool>& cancel){uart_write_file(path,encoded,replace,cancel);LabFileResult result;result.view=view;result.report="View saved privately and staged. No capture/serial/parser changes.";return result;});}
}
void CMainDialog::lab_images_request(const UartLabWindow::Request& r){if(r.fields.size()!=3)return;double width=0,height=0;if(!uart_number(r.fields[1],400,4096,width)||!uart_number(r.fields[2],300,4096,height)||width!=std::floor(width)||height!=std::floor(height))throw std::runtime_error("PNG dimensions must be whole pixels within bounds.");const auto path=pick_workflow_file(L"png",L"PNG chart image (*.png)|*.png||");if(!path)return;std::vector<PlotView*> plots;if(r.fields[0]==L"4 Charts")for(size_t i=0;i<4;++i)plots.push_back(&data_page_.chart_plot(i));else plots.push_back(&plot_view_);const auto raster=std::make_shared<PlotRaster>(raster_plots(plots,static_cast<int>(width),static_cast<int>(height)));start_lab_file(r.page,r.action,[path=*path,raster](const std::atomic<bool>& cancel){const auto bytes=encode_plot_png(*raster,cancel);uart_write_file(path,bytes,true,cancel);return LabFileResult{"PNG saved atomically; chart, capture and raw recordings unchanged.",{}};});}
