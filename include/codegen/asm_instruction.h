#pragma once
#include <string>
#include "type_system/variable.h"

struct Register {
    const char* _64;
    const char* _32;
    const char* _16;
    const char* _8;
};
inline std::string default_output;
inline std::string* m_output;

class Memory;
class AsmInstruction {
public:
    AsmInstruction(const char* inst_name, Register suffixs = {"q", "l", "w", "b"})
        : m_instName(inst_name), 
          m_instSuffixs(suffixs) {}

    // appends instruction
    void append();
    void append(Memory src, size_t size = 8);
    void append(Memory src, Memory dest, size_t size = 8);
    // appends instruction with `reg`
    void append(Register reg, size_t size = 8);
    // appends instruction with `src` into `dest` with the size of `size`
    void append(Register src      , Register dest  , size_t   size = 8);
    // appends instruction with `src+offset` into `dest` with the size of `size`
    void append(int64_t  offset   , Register src   , Register dest, size_t size = 8);
    // appends instruction with `src` into `dest+offset` with the size of `size`
    void append(Register src      , int64_t  offset, Register dest, size_t size = 8);
    // appends instruction with `global_label+src` into `dest` with the size of `size`
    void append(std::string global_label, Register src, Register dest, size_t size = 8);
    // appends instruction with `src` into `global_label+dest` with the size of `size`
    void append(Register src, std::string global_label, Register dest, size_t size = 8);
    // appends instruction with `int_value` into `dest+offset` of size `size`
    void append(int64_t  int_value, int64_t  offset, Register dest, size_t size = 8);
    // appends instruction with `int_value` into `dest+offset` of size `size`
    void append(int64_t  int_value, std::string label, Register dest, size_t size = 8);
    // appends instruction with `int_value` into `dest` of size `size`
    void append(int64_t  int_value, Register dest, size_t size = 8);

    static void set_output(std::string* out) {
        m_output = out;
    }
private:
    std::string m_instName = "";
    Register m_instSuffixs = {"q", "l", "w", "b"};
};

inline AsmInstruction movabs = AsmInstruction("movabs");                   ;
inline AsmInstruction lea    = AsmInstruction("lea" , {"q", "q", "q", "q"});
inline AsmInstruction cmp    = AsmInstruction("cmp");
inline AsmInstruction mov    = AsmInstruction("mov");
inline AsmInstruction add    = AsmInstruction("add");
inline AsmInstruction sub    = AsmInstruction("sub");
inline AsmInstruction imul   = AsmInstruction("imul");
inline AsmInstruction idiv   = AsmInstruction("idiv");
inline AsmInstruction adds   = AsmInstruction("adds" , {"d", "s", "s", "s"});
inline AsmInstruction movs   = AsmInstruction("movs" , {"d", "s", "s", "s"});
inline AsmInstruction subs   = AsmInstruction("subs" , {"d", "s", "s", "s"});
inline AsmInstruction muls   = AsmInstruction("muls" , {"d", "s", "s", "s"});
inline AsmInstruction divs   = AsmInstruction("divs" , {"d", "s", "s", "s"});
inline AsmInstruction comis  = AsmInstruction("comis", {"d", "s", "s", "s"});
inline AsmInstruction sete   = AsmInstruction("sete" , {"", "", "", ""});
inline AsmInstruction setne  = AsmInstruction("setne", {"", "", "", ""});
inline AsmInstruction setl   = AsmInstruction("setl" , {"", "", "", ""});
inline AsmInstruction setle  = AsmInstruction("setle", {"", "", "", ""});
inline AsmInstruction setg   = AsmInstruction("setg" , {"", "", "", ""});
inline AsmInstruction setge  = AsmInstruction("setge", {"", "", "", ""});
inline AsmInstruction setb   = AsmInstruction("setb" , {"", "", "", ""});
inline AsmInstruction setbe  = AsmInstruction("setbe", {"", "", "", ""});
inline AsmInstruction seta   = AsmInstruction("seta" , {"", "", "", ""});
inline AsmInstruction setnb  = AsmInstruction("setnb", {"", "", "", ""});
inline AsmInstruction and_   = AsmInstruction("and");
inline AsmInstruction or_    = AsmInstruction("or");
