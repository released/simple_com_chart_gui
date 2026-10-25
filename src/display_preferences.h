#pragma once
#include <string>
#include <unordered_map>

struct ChannelDisplay {
    std::string alias,unit;
    int decimals=-1;
    double scale=1,offset=0;
};
class DisplayPreferences {
public:
    bool set(const std::string& key,const ChannelDisplay&,std::string& error);
    void erase(const std::string& key) {entries_.erase(key);}
    ChannelDisplay get(const std::string& key) const;
    double project(const std::string& key,double raw) const;
    std::string name(const std::string& key) const;
    std::string format(const std::string& key,double displayed) const;
    std::string encode() const;
    static DisplayPreferences decode(const std::string&);
private:
    std::unordered_map<std::string,ChannelDisplay> entries_;
};
