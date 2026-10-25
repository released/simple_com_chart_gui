#include "derived_channels.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>

class DerivedChannels::Parser {
public:
    Parser(std::string text,const std::set<std::string>& known):text_(std::move(text)),known_(known){}
    std::vector<Node> parse(){expression(0);space();if(at_!=text_.size())fail("Unexpected formula suffix.");return std::move(nodes_);}
private:
    void fail(const char* why){throw std::runtime_error(why);}
    void space(){while(at_<text_.size()&&std::isspace(static_cast<unsigned char>(text_[at_])))++at_;}
    bool take(char c){space();if(at_<text_.size()&&text_[at_]==c){++at_;return true;}return false;}
    size_t add(Node node){if(nodes_.size()>=128)fail("Formula exceeds 128 nodes.");nodes_.push_back(std::move(node));return nodes_.size()-1;}
    size_t expression(unsigned depth){auto left=term(depth+1);for(;;){space();if(at_>=text_.size()||(text_[at_]!='+'&&text_[at_]!='-'))return left;const auto op=text_[at_++];const auto right=term(depth+1);Node n;n.op=op;n.left=left;n.right=right;left=add(std::move(n));}}
    size_t term(unsigned depth){auto left=primary(depth+1);for(;;){space();if(at_>=text_.size()||(text_[at_]!='*'&&text_[at_]!='/'))return left;const auto op=text_[at_++];const auto right=primary(depth+1);Node n;n.op=op;n.left=left;n.right=right;left=add(std::move(n));}}
    size_t primary(unsigned depth){if(depth>32)fail("Formula nesting exceeds 32.");space();if(at_==text_.size())fail("Missing formula operand.");
        if(take('+'))return primary(depth+1);if(take('-')){Node n;n.op='n';n.left=primary(depth+1);return add(std::move(n));}
        if(take('(')){const auto value=expression(depth+1);if(!take(')'))fail("Missing closing parenthesis.");return value;}
        if(std::isdigit(static_cast<unsigned char>(text_[at_]))||text_[at_]=='.'){Node n;const auto first=text_.data()+at_;const auto parsed=std::from_chars(first,text_.data()+text_.size(),n.number);if(parsed.ec!=std::errc{}||parsed.ptr==first||!std::isfinite(n.number))fail("Invalid finite number.");at_=static_cast<size_t>(parsed.ptr-text_.data());return add(std::move(n));}
        std::string name;if(take('`')){const auto end=text_.find('`',at_);if(end==text_.npos)fail("Unclosed quoted channel key.");name=text_.substr(at_,end-at_);at_=end+1;}else{const auto first=at_;while(at_<text_.size()){const auto c=static_cast<unsigned char>(text_[at_]);if(!std::isalnum(c)&&c!='_'&&c!='.'&&c!='['&&c!=']')break;++at_;}name=text_.substr(first,at_-first);}
        if(name.empty()||name.size()>160)fail("Invalid channel key; use backticks for special names.");
        if(take('(')){Node n;if(name=="abs")n.op='a';else if(name=="sqrt")n.op='s';else if(name=="min")n.op='<';else if(name=="max")n.op='>';else if(name=="ema")n.op='e';else fail("Supported functions: abs, sqrt, min, max, ema.");n.left=expression(depth+1);if(n.op=='<'||n.op=='>'||n.op=='e'){if(!take(','))fail("Missing function argument.");n.right=expression(depth+1);}if(!take(')'))fail("Missing function closing parenthesis.");return add(std::move(n));}
        if(name.rfind("calc.",0)==0&&!known_.count(name))fail("Computed channels may reference earlier definitions only (no cycles/forward references).");
        Node n;n.op='k';n.key=std::move(name);return add(std::move(n));
    }
    std::string text_;const std::set<std::string>& known_;size_t at_=0;std::vector<Node> nodes_;
};
bool DerivedChannels::configure(const std::string& text,std::string& error){
    try{if(text.size()>8192)throw std::runtime_error("Derived definitions exceed 8 KiB.");std::vector<Formula> next;std::set<std::string> names;std::istringstream input(text);std::string line;
        while(std::getline(input,line)){const auto start=line.find_first_not_of(" \t\r");if(start==line.npos||line[start]=='#')continue;const auto equals=line.find('=',start);if(equals==line.npos)throw std::runtime_error("Use name = expression, one per line.");auto name=line.substr(start,equals-start);const auto end=name.find_last_not_of(" \t");name.resize(end+1);
            if(name.empty()||name.size()>48||!std::all_of(name.begin(),name.end(),[](unsigned char c){return std::isalnum(c)||c=='_';}))throw std::runtime_error("Derived names use 1..48 ASCII letters/digits/underscore.");name="calc."+name;if(names.count(name))throw std::runtime_error("Duplicate derived name.");if(next.size()>=32)throw std::runtime_error("At most 32 derived channels.");
            auto nodes=Parser(line.substr(equals+1),names).parse();next.push_back({name,std::move(nodes)});names.insert(name);
        }formulas_=std::move(next);definitions_=text;skipped_=0;return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
void DerivedChannels::reset(){for(auto& formula:formulas_)for(auto& node:formula.nodes)node.state.reset();skipped_=0;}
std::unordered_map<std::string,double> DerivedChannels::evaluate(const std::unordered_map<std::string,double>& raw){
    if(formulas_.empty())return {};
    auto available=raw;std::unordered_map<std::string,double> result;
    for(auto& formula:formulas_){if(raw.count(formula.key)){++skipped_;continue;}std::vector<std::optional<double>> values;values.reserve(formula.nodes.size());
        for(auto& node:formula.nodes){std::optional<double> value;const auto op=node.op;
            if(!op)value=node.number;
            else if(op=='k'){const auto it=available.find(node.key);if(it!=available.end()&&std::isfinite(it->second))value=it->second;}
            else if(values[node.left]){const double a=*values[node.left];if(op=='n')value=-a;else if(op=='a')value=std::abs(a);else if(op=='s'){if(a>=0)value=std::sqrt(a);}else if(values[node.right]){const double b=*values[node.right];switch(op){case '+':value=a+b;break;case '-':value=a-b;break;case '*':value=a*b;break;case '/':if(b!=0)value=a/b;break;case '<':value=std::min(a,b);break;case '>':value=std::max(a,b);break;case 'e':if(b>0&&b<=1){value=node.state?b*a+(1-b)*(*node.state):a;if(value&&std::isfinite(*value))node.state=value;}break;default:break;}}}
            if(value&&!std::isfinite(*value))value.reset();values.push_back(value);
        }
        if(!values.empty()&&values.back()){available[formula.key]=*values.back();result[formula.key]=*values.back();}else ++skipped_;
    }return result;
}
