#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
class DerivedChannels {
public:
    bool configure(const std::string& definitions,std::string& error);
    std::unordered_map<std::string,double> evaluate(const std::unordered_map<std::string,double>& raw);
    void reset();
    const std::string& definitions()const{return definitions_;}
    uint64_t skipped()const{return skipped_;}
private:
    struct Node {char op=0;double number=0;std::string key;size_t left=0,right=0;std::optional<double> state;};
    struct Formula {std::string key;std::vector<Node> nodes;};
    class Parser;
    std::string definitions_;std::vector<Formula> formulas_;uint64_t skipped_=0;
};
