#pragma once
#include <string>
#include "types.h"

struct Register {
    std::string _64;
    std::string _32;
    std::string _16;
    std::string _8;
};

class AsmInstruction {
public:
    AsmInstruction(std::string inst_name, std::string& output, Register suffixs = {"q", "l", "w", "b"})
        : m_instName(inst_name), 
          m_output(output),
          m_instSuffixs(suffixs) {}
    ~AsmInstruction() = default;

    // appends instruction
    void append();
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

private:
    std::string& m_output;
    std::string m_instName = "";
    Register m_instSuffixs;
};
