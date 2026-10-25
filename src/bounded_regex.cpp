#include "bounded_regex.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>

namespace {
struct Node {
    enum Kind { Byte, Seq, Alt, Repeat, Group, Begin, End, Empty } kind=Empty;
    std::bitset<256> bytes;
    std::vector<std::shared_ptr<Node>> children;
    int low=0,high=0,group=0;
};
using Ptr=std::shared_ptr<Node>;
Ptr node(Node::Kind kind) { auto n=std::make_shared<Node>(); n->kind=kind; return n; }
class Parser {
public:
    explicit Parser(const std::string& p):p_(p){}
    Ptr parse() { auto result=expression(); if(pos_!=p_.size()) fail("Unexpected closing parenthesis."); return result; }
    int groups=0;
private:
    [[noreturn]] void fail(const char* error) { throw std::runtime_error(error); }
    bool eat(char c) { if(pos_<p_.size() && p_[pos_]==c) { ++pos_; return true; } return false; }
    std::bitset<256> character() {
        if(pos_==p_.size()) fail("Incomplete character escape.");
        unsigned char c=static_cast<unsigned char>(p_[pos_++]); std::bitset<256> result;
        if(c!='\\') { result.set(c); return result; }
        if(pos_==p_.size()) fail("Incomplete escape."); c=static_cast<unsigned char>(p_[pos_++]);
        if(c>='0' && c<='9') fail("Backreferences are not supported.");
        if(c=='x') {unsigned value=0;for(int i=0;i<2;++i) {if(pos_==p_.size()) fail("Incomplete hex escape.");const auto digit=p_[pos_++];const int n=digit>='0'&&digit<='9'?digit-'0':digit>='a'&&digit<='f'?digit-'a'+10:digit>='A'&&digit<='F'?digit-'A'+10:-1;if(n<0) fail("Invalid hex escape.");value=value*16+n;}result.set(value);return result;}
        if(c=='d' || c=='D' || c=='w' || c=='W' || c=='s' || c=='S') {
            for(unsigned i=0;i<256;++i) {
                const bool digit=i>='0' && i<='9';
                const bool word=digit || (i>='a' && i<='z') || (i>='A' && i<='Z') || i=='_';
                const bool space=i==' ' || i=='\t' || i=='\r' || i=='\n' || i=='\f' || i=='\v';
                const bool yes=c=='d'||c=='D'?digit:c=='w'||c=='W'?word:space;
                if((c=='D'||c=='W'||c=='S')?!yes:yes) result.set(i);
            }
        } else {
            if(((c>='A'&&c<='Z')||(c>='a'&&c<='z')) && c!='n' && c!='r' && c!='t' && c!='f' && c!='v') fail("Unsupported escape; use explicit character classes.");
            result.set(c=='n'?'\n':c=='r'?'\r':c=='t'?'\t':c=='f'?'\f':c=='v'?'\v':c);
        }
        return result;
    }
    Ptr expression() {
        auto result=node(Node::Alt); result->children.push_back(sequence());
        while(eat('|')) result->children.push_back(sequence());
        return result->children.size()==1?result->children.front():result;
    }
    Ptr sequence() {
        auto result=node(Node::Seq);
        while(pos_<p_.size() && p_[pos_]!=')' && p_[pos_]!='|') {
            auto item=atom();
            if(pos_<p_.size() && (p_[pos_]=='*'||p_[pos_]=='+'||p_[pos_]=='?'||p_[pos_]=='{')) {
                auto repeat=node(Node::Repeat); repeat->children.push_back(item);
                if(eat('*')) {repeat->low=0;repeat->high=-1;} else if(eat('+')) {repeat->low=1;repeat->high=-1;}
                else if(eat('?')) {repeat->low=0;repeat->high=1;} else {
                    ++pos_; repeat->low=count(); repeat->high=repeat->low;
                    if(eat(',')) repeat->high=pos_<p_.size() && p_[pos_]=='}'?-1:count();
                    if(!eat('}') || (repeat->high>=0 && repeat->high<repeat->low)) fail("Invalid repeat range.");
                }
                if(pos_<p_.size() && (p_[pos_]=='?'||p_[pos_]=='+'||p_[pos_]=='*'||p_[pos_]=='{')) fail("Lazy/possessive or repeated quantifiers are not supported.");
                item=repeat;
            }
            result->children.push_back(item);
        }
        return result->children.empty()?node(Node::Empty):result->children.size()==1?result->children.front():result;
    }
    int count() { int result=0, digits=0; while(pos_<p_.size() && p_[pos_]>='0' && p_[pos_]<='9') { result=result*10+p_[pos_++]-'0'; ++digits; if(result>64) fail("Repeat count exceeds 64."); } if(!digits) fail("Missing repeat count."); return result; }
    Ptr atom() {
        if(eat('(')) {
            if(++depth_>16) fail("Group nesting exceeds 16.");
            bool capture=true;
            if(eat('?')) { if(!eat(':')) fail("Lookaround and inline flags are not supported."); capture=false; }
            const int group=capture?++groups:0; if(groups>8) fail("At most 8 capture groups are supported.");
            auto content=expression(); if(!eat(')')) fail("Unclosed group."); --depth_;
            if(!capture) return content; auto result=node(Node::Group); result->group=group; result->children.push_back(content); return result;
        }
        if(eat('^')) return node(Node::Begin); if(eat('$')) return node(Node::End);
        auto result=node(Node::Byte);
        if(eat('.')) { result->bytes.set(); result->bytes.reset('\n'); result->bytes.reset('\r'); return result; }
        if(eat('[')) {
            const bool negate=eat('^'); bool any=false;
            while(pos_<p_.size() && p_[pos_]!=']') {
                const auto first=character(); any=true;
                if(pos_+1<p_.size() && p_[pos_]=='-' && p_[pos_+1]!=']') {
                    ++pos_; const auto last=character(); if(first.count()!=1 || last.count()!=1) fail("Invalid character range.");
                    unsigned lo=0,hi=0; while(!first[lo]) ++lo; while(!last[hi]) ++hi;
                    if(lo>hi) fail("Reversed character range."); for(unsigned i=lo;i<=hi;++i) result->bytes.set(i);
                } else result->bytes|=first;
            }
            if(!eat(']') || !any) fail("Unclosed or empty character class."); if(negate) result->bytes.flip(); return result;
        }
        if(pos_==p_.size() || p_[pos_]=='*'||p_[pos_]=='+'||p_[pos_]=='?'||p_[pos_]=='{'||p_[pos_]=='}') fail("Unexpected quantifier.");
        result->bytes=character(); return result;
    }
    const std::string& p_; size_t pos_=0; int depth_=0;
};
}
bool BoundedRegex::compile(const std::string& pattern,std::string& error) {
    error.clear();
    if(pattern.size()>1024) { error="Regex exceeds 1024 UTF-8 bytes."; return false; }
    try {
        Parser parser(pattern); const auto tree=parser.parse(); BoundedRegex result;
        const auto emit=[&](State state) { if(result.states_.size()>=1024) throw std::runtime_error("Regex expands beyond 1024 states."); result.states_.push_back(std::move(state)); return static_cast<int>(result.states_.size()-1); };
        std::function<int(const Ptr&,int)> build;
        build=[&](const Ptr& n,int next)->int {
            if(n->kind==Node::Empty) return next;
            if(n->kind==Node::Byte) { State s; s.kind=State::Byte;s.a=next;s.bytes=n->bytes; return emit(s); }
            if(n->kind==Node::Begin || n->kind==Node::End) { State s; s.kind=n->kind==Node::Begin?State::Begin:State::End;s.a=next;return emit(s); }
            if(n->kind==Node::Seq) { for(auto i=n->children.rbegin();i!=n->children.rend();++i) next=build(*i,next); return next; }
            if(n->kind==Node::Alt) { int target=build(n->children.back(),next); for(size_t i=n->children.size()-1;i>0;--i) {State s; s.kind=State::Split;s.a=build(n->children[i-1],next);s.b=target;target=emit(s);} return target; }
            if(n->kind==Node::Group) {State close; close.kind=State::Save;close.slot=n->group*2+1;close.a=next;int end=emit(close); State begin;begin.kind=State::Save;begin.slot=n->group*2;begin.a=build(n->children[0],end);return emit(begin);}
            if(n->high<0) {State split;split.kind=State::Split;split.b=next;int loop=emit(split); int body=build(n->children[0],loop);result.states_[loop].a=body;next=loop;}
            else for(int i=n->low;i<n->high;++i) {State s;s.kind=State::Split;s.a=build(n->children[0],next);s.b=next;next=emit(s);}
            for(int i=0;i<n->low;++i) next=build(n->children[0],next); return next;
        };
        State accept; accept.kind=State::Accept; result.start_=build(tree,emit(accept));result.groups_=parser.groups; *this=std::move(result); return true;
    } catch(const std::exception& failure) {error=failure.what();return false;}
}
BoundedRegex::Match BoundedRegex::search(std::string_view input,size_t budget) const {
    Match result;result.bounds.fill(-1); if(start_<0) return result;
    if(input.size()>65536) {result.limited=true;return result;}
    struct Thread {int state;std::array<int,18> captures;};
    std::vector<Thread> current,next,stack; current.reserve(states_.size()); next.reserve(states_.size());
    std::vector<bool> visited(states_.size());
    const auto add=[&](Thread thread,size_t pos,std::vector<Thread>& list) {
        stack.clear();stack.push_back(std::move(thread));
        while(!stack.empty() && !result.limited) {
            auto item=stack.back();stack.pop_back();
            if(!budget--) {result.limited=true;break;}
            if(visited[item.state]) continue;visited[item.state]=true;
            const auto& s=states_[item.state];
            if(s.kind==State::Split) {auto alternative=item;alternative.state=s.b;stack.push_back(alternative);item.state=s.a;stack.push_back(item);}
            else if(s.kind==State::Save) {item.captures[s.slot]=static_cast<int>(pos);item.state=s.a;stack.push_back(item);}
            else if(s.kind==State::Begin || s.kind==State::End) {if((s.kind==State::Begin && pos==0)||(s.kind==State::End && pos==input.size())) {item.state=s.a;stack.push_back(item);}}
            else if(s.kind==State::Accept) {if(!result.found || item.captures[0]<result.bounds[0] || (item.captures[0]==result.bounds[0] && static_cast<int>(pos)>result.bounds[1])) {result.found=true;result.bounds=item.captures;result.bounds[1]=static_cast<int>(pos);}}
            else list.push_back(item);
        }
    };
    std::array<int,18> initial;initial.fill(-1); initial[0]=0; add({start_,initial},0,current);
    for(size_t pos=0;pos<input.size() && !result.limited;++pos) {
        next.clear();std::fill(visited.begin(),visited.end(),false);
        for(auto thread:current) {
            if(!budget--) {result.limited=true;break;}
            if(result.found && thread.captures[0]>result.bounds[0]) continue;
            if(states_[thread.state].bytes[static_cast<unsigned char>(input[pos])]) {thread.state=states_[thread.state].a;add(thread,pos+1,next);}
        }
        if(!result.found) {initial[0]=static_cast<int>(pos+1);add({start_,initial},pos+1,next);}
        current.swap(next);
        if(result.found && current.empty()) break;
    }
    if(result.limited) {result.found=false;result.bounds.fill(-1);} return result;
}
