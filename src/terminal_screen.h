#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
// Bounded, receive-only, monochrome VT100-style ASCII screen. Never replies/TX.
class TerminalScreen {
public:
    TerminalScreen(){resize(80,24);}
    bool resize(unsigned columns,unsigned rows);
    void reset();
    void feed(std::string_view);
    std::wstring text()const;
    unsigned columns()const{return columns_;}unsigned rows()const{return rows_;}
    unsigned row()const{return row_;}unsigned column()const{return column_;}
    uint64_t ignored()const{return ignored_;}
private:
    void print(unsigned char);
    void index(bool reverse=false);
    void scroll(unsigned top,unsigned bottom,unsigned count,bool reverse);
    void csi(char);
    enum Parser{Normal,Escape,Csi,Charset,Ignore,IgnoreEscape,Hash}parser_=Normal;
    std::vector<std::wstring> lines_;
    std::vector<bool> tabs_;
    unsigned columns_=80,rows_=24,row_=0,column_=0,top_=0,bottom_=23,saved_row_=0,saved_column_=0,charset_slot_=0;
    bool wrap_=true,wrap_pending_=false,origin_=false,g0_=false,g1_=false,use_g1_=false,insert_=false;
    uint64_t ignored_=0;
    std::string parameters_;
};
