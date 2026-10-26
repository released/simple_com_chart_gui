#include "mfc_main_dialog.h"
#include "uart_workbench.h"
#include "uart_tool_form.h"
void CMainDialog::accept_numeric(const std::string& line,const std::unordered_map<std::string,double>& values,double at,uint64_t offset,uint64_t count,unsigned source,uint64_t generation,uint16_t parser_prefix){
    const uint64_t sample_generation=generation!=UINT64_MAX?generation:source&&workbench_?workbench_->secondary_.generation():replay_.data()?replay_epoch_:uart_operations_.generation();
    if(source<2&&!sequence_keys_[source].empty()){const auto found=values.find(sequence_keys_[source]);if(found!=values.end())sequences_[source].receive(found->second,sample_generation);}
    if(capture_.keys().empty()){numeric_sources_.clear();source_collisions_=0;numeric_secondary_seen_=false;for(auto& engine:derived_)engine.reset();}
    if(!source&&!numeric_secondary_seen_&&derived_[0].definitions().empty()){
        capture_.append(line,values,at,offset,count,sample_generation,source,parser_prefix);
        if(capture_.keys().size()>numeric_sources_.size())for(const auto& key:capture_.keys())if(numeric_sources_.emplace(key,0).second)model_.ensure_channel(key,at);
        data_page_.append(values,at);model_.update_from_kv(values,at);return;
    }
    if(source)numeric_secondary_seen_=true;
    if(!derived_[source].definitions().empty()){
        if(derived_generations_[source]!=sample_generation){derived_[source].reset();derived_generations_[source]=sample_generation;}}
    std::unordered_map<std::string,double> accepted;
    const auto add=[&](const std::pair<const std::string,double>& value){const auto owner=numeric_sources_.find(value.first);
        if(owner!=numeric_sources_.end()&&owner->second!=source){
            ++source_collisions_;if(source_collisions_==1)log_line(L"Numeric source-name collision; conflicting samples excluded. Raw logs remain intact.");
            show_status_message(L"A/B field-name collision: conflicting numeric samples skipped; review parser keys.",5000);return false;
        }
        accepted.insert(value);return true;
    };
    size_t new_raw=0;
    for(const auto& value:values)if(add(value)&&!numeric_sources_.count(value.first))++new_raw;
    const auto limit=static_cast<size_t>(model_.get_max_channels());
    size_t room=limit-std::min(limit,numeric_sources_.size()+new_raw);
    auto generated=derived_[source].evaluate(values);
    if(source){std::unordered_map<std::string,double> qualified;for(const auto& value:generated)qualified.emplace("B."+value.first,value.second);generated=std::move(qualified);}
    std::vector<std::string> computed;
    for(const auto& value:generated)computed.push_back(value.first);std::sort(computed.begin(),computed.end());
    for(const auto& key:computed){const auto& value=*generated.find(key);
        if(numeric_sources_.count(key))add(value);else if(room&&add(value))--room;
    }
    capture_.append(line,accepted,at,offset,count,sample_generation,source,parser_prefix);
    if(capture_.keys().size()>numeric_sources_.size())for(const auto& key:capture_.keys())if(!numeric_sources_.count(key)&&accepted.count(key)){numeric_sources_[key]=source;model_.ensure_channel(key,at);}
    data_page_.append(accepted,at);model_.update_from_kv(accepted,at);
}
void CMainDialog::load_derived(){std::vector<wchar_t> text(32768);const auto n=GetPrivateProfileStringW(L"Derived",L"Configuration",L"",text.data(),static_cast<DWORD>(text.size()),settings_path_.c_str());std::string definitions,error;
    try{if(n>=text.size()-1||!uart_unhex(message_utf8(text.data()),definitions)||!derived_[0].configure(definitions,error)||!derived_[1].configure(definitions,error))throw std::runtime_error("Invalid formulas");}
    catch(...){derived_={};show_status_message(L"Invalid saved formulas; derived channels disabled. Review UART > Derived channels.",7000);}
}
void CMainDialog::derived_options(){
    if(profile_job_.busy()||replay_load_.busy()||exporting_){show_status_message(L"Finish profile/load/export work before editing derived channels.",5000);return;}
    UartToolForm form(L"Derived channels",L"One name = expression per line; output is calc.name (B.calc.name on Port B; use B.key inputs). Operators + - * /, abs(x), sqrt(x), min(x,y), max(x,y), ema(x,alpha). Backticks quote special keys. Same-frame finite inputs only; missing/stale inputs yield no sample. Earlier calc.names may be referenced; no forward references. Apply starts a new numeric capture; raw recording stays unchanged.",
        {{L"Definitions (empty=disabled)",uart_wide(derived_[0].definitions()),{},10},{L"Preview key:value line",L"voltage:12,current:2",{},2}},
        [](const auto& f){DerivedChannels engine;std::string error;if(!engine.configure(message_utf8(f[0]),error))return uart_wide(error);const auto raw=log_parser::parse_kv_log(message_utf8(f[1]));const auto result=engine.evaluate(raw);std::wstring out=L"Preview only; computed "+std::to_wstring(result.size());for(const auto& value:result)out+=L"\r\n"+uart_wide(value.first)+L" = "+std::to_wstring(value.second);return out;},
        [](const auto& f,std::wstring& error){DerivedChannels engine;std::string detail;const bool ok=engine.configure(message_utf8(f[0]),detail);error=uart_wide(detail);return ok;},this);
    if(form.DoModal()!=IDOK||profile_job_.busy())return;const auto definitions=message_utf8(form.values()[0]);std::array<DerivedChannels,2> next;std::string error;for(auto& engine:next)if(!engine.configure(definitions,error))return;
    const auto encoded=uart_wide(uart_hex(definitions));if(!WritePrivateProfileStringW(L"Derived",L"Configuration",encoded.c_str(),settings_path_.c_str())){show_status_message(L"Cannot save formulas; previous definitions retained.",6000);return;}
    derived_=std::move(next);snapshot_=false;plot_view_.set_frozen(false);::SetWindowTextW(btn_snapshot_,L"Snapshot");::SendMessageW(btn_snapshot_,BM_SETCHECK,BST_UNCHECKED,0);model_.reset();capture_.reset(static_cast<size_t>(model_.get_max_channels()));data_page_.reset();channel_panel_.reset();plot_view_.reset_visual();show_status_message(L"Derived channels saved; new numeric capture. Raw RX/recording unchanged.",6000);
}
