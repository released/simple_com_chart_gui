#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
using JsonSelections=std::vector<std::pair<std::string,std::string>>;
JsonSelections json_selections(const std::string& definition);
struct JsonNumericNode {std::string pointer,type;size_t parent=SIZE_MAX;bool numeric=false;double value=0;};
std::vector<JsonNumericNode> json_numeric_tree(const std::string&);
std::string json_tree_selection(const std::vector<JsonNumericNode>&,const std::vector<size_t>&);
std::unordered_map<std::string,double> json_numbers(const std::string&,const JsonSelections&);
