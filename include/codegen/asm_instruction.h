#pragma once
#include <string>
#include "type_system/variable.h"

struct Register {
    const char* _64;
    const char* _32;
    const char* _16;
    const char* _8;
};

class Memory;
class AsmInstruction {
public:
    AsmInstruction(const char* inst_name, std::string& output, Register suffixs = {"q", "l", "w", "b"})
        : m_instName(inst_name), 
          m_output(&output),
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

private:
    std::string m_instName = "";
    std::string* m_output;
    Register m_instSuffixs = {"q", "l", "w", "b"};
};
