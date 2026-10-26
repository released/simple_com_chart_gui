#pragma once
#include <string>
#include <unordered_map>
#include <vector>
using JsonSelections=std::vector<std::pair<std::string,std::string>>;
JsonSelections json_selections(const std::string& definition);
std::unordered_map<std::string,double> json_numbers(const std::string&,const JsonSelections&);
