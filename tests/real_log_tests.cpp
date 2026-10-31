#include "real_log_cases.h"
#include "log_parser.h"
#include "line_filter.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

void test_real_logs(int argc,char** argv) {
    for(const auto& sample:real_log_cases()){
        if(log_parser::parse_kv_log(sample.input)!=sample.expected)throw std::runtime_error(std::string("Golden numeric fields differ: ")+sample.file);
        LineFilter filter;std::string error;
        if(!filter.configure(sample.include,sample.exclude,false,error)||filter.matches(sample.input)!=sample.accepted)
            throw std::runtime_error(std::string("Golden line filter differs: ")+sample.file);
    }
    for(int argument=2;argument<argc;++argument){
        const auto name=std::filesystem::path(argv[argument]).filename().string();const RealLogCase* oracle=nullptr;
        for(const auto& sample:real_log_cases())if(name==sample.file)oracle=&sample;
        if(!oracle)throw std::runtime_error("No reviewed oracle for requested input: "+name);
        std::ifstream log(argv[argument],std::ios::binary);if(!log)throw std::runtime_error("Cannot open real-log input: "+name);
        LineFilter filter;std::string error;if(!filter.configure(oracle->include,oracle->exclude,false,error))throw std::runtime_error(error);
        size_t lines=0,parsed=0,accepted=0,excluded=0;bool exact=false;std::set<std::string> keys;std::string line;
        while(std::getline(log,line)){
            if(!line.empty()&&line.back()=='\r')line.pop_back();++lines;
            const bool expected=(std::string(oracle->include).empty()||line.find(oracle->include)!=line.npos)&&
                (std::string(oracle->exclude).empty()||line.find(oracle->exclude)==line.npos);
            if(filter.matches(line)!=expected)throw std::runtime_error("Raw-line filter mismatch in "+name);
            const auto values=log_parser::parse_kv_log(line);if(!values.empty())++parsed;
            for(const auto& value:values)keys.insert(value.first);
            if(expected&&!values.empty())++accepted;else if(!expected)++excluded;
            if(line==oracle->input){exact=true;if(values!=oracle->expected)throw std::runtime_error("Actual file values differ from reviewed map: "+name);}
        }
        if(!log.eof()||!exact||!parsed)throw std::runtime_error("Real-log audit incomplete or reviewed sample missing: "+name);
        std::cout<<"AUDIT: "<<name<<" lines="<<lines<<" parsed="<<parsed<<" filtered-numeric="<<accepted<<" excluded="<<excluded<<" keys="<<keys.size()<<" exact-oracle=PASS\n";
    }
}
