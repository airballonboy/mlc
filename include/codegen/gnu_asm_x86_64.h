#pragma once 
#include "codegen/base.h"
#include "types.h"

struct Register {
    std::string_view _64;
    std::string_view _32;
    std::string_view _16;
    std::string_view _8;
};

class gnu_asm : public BaseCodegenerator {
public:

    gnu_asm(Program* prog) : BaseCodegenerator(prog) {}

    void move_var_to_reg(Variable arg, Register reg);
    void move_reg_to_var(Register reg, Variable arg2);
    void move_var_to_var(Variable arg1, Variable arg2);
    void move_reg_to_reg(Register reg1, Register reg2);
    void deref_var_to_reg(Variable arg, Register reg);

    // moves `src` into `dest` with the size of `size`
    void mov(Register src      , Register dest  , size_t   size);
    // moves `src+offset` into `dest` with the size of `size`
    void mov(int64_t  offset   , Register src   , Register dest, size_t size);
    // moves `src` into `dest+offset` with the size of `size`
    void mov(Register src      , int64_t  offset, Register dest, size_t size);
    // moves `global_label+src` into `dest` with the size of `size`
    void mov(std::string global_label, Register src, Register dest, size_t size);
    // moves `src` into `global_label+dest` with the size of `size`
    void mov(Register src, std::string global_label, Register dest, size_t size);
    // moves `global_label+src` into `dest` with the size of 8
    void mov(std::string global_label, Register src, Register dest);
    // moves `src` into `global_label+dest` with the size of 8
    void mov(Register src, std::string global_label, Register dest);
    // moves `src` into `dest` with the size of 8
    void mov(Register src      , Register dest);
    // moves `src+offset` into `dest` with the size of 8
    void mov(int64_t  offset   , Register src   , Register dest);
    // moves `src` into `dest+offset` with the size of 8
    void mov(Register src      , int64_t  offset, Register dest);
    // moves `int_value` into `dest+offset` of size 8
    void mov(int64_t  int_value, int64_t  offset, Register dest);
    // moves `int_value` into `dest` of size 8
    void mov(int64_t  int_value, Register dest);
    // moves `int_value` into `dest+offset` of size `size`
    void mov(int64_t  int_value, int64_t  offset, Register dest, size_t size);
    // moves `int_value` into `dest` of size `size`
    void mov(int64_t  int_value, Register dest, size_t size);
    void mov(Variable src      , Register dest);
    void mov(Register src      , Variable dest);
    void mov(Variable src      , Variable dest);
    void lea(Register src      , Register dest);
    void lea(std::string label, Register src      , Register dest);
    void lea(int64_t offset   , Register src      , Register dest);
    void mov_member(Variable src, Register dest);
    void mov_to_deref  (Register src, Register dest);
    void mov_from_deref(Register src, Register dest);

    void call_func(std::string func_name, VariableStorage args);
    void compileProgram() override;
    void compileFunction(Func func) override;
};

const Register Rip = {"%rip", "%rip" , "%rip" , "%rip"};
const Register Rax = {"%rax", "%eax" , "%ax"  , "%al"};
const Register Rbx = {"%rbx", "%ebx" , "%bx"  , "%bl"};
const Register Rcx = {"%rcx", "%ecx" , "%cx"  , "%cl"};
const Register Rdx = {"%rdx", "%edx" , "%dx"  , "%dl"};
const Register Rsi = {"%rsi", "%esi" , "%si"  , "%sil"};
const Register Rdi = {"%rdi", "%edi" , "%di"  , "%dil"};
const Register Rbp = {"%rbp", "%ebp" , "%bp"  , "%bpl"};
const Register Rsp = {"%rsp", "%esp" , "%sp"  , "%spl"};
const Register R8  = {"%r8" , "%r8d" , "%r8w" , "%r8b"};
const Register R9  = {"%r9" , "%r9d" , "%r9w" , "%r9b"};
const Register R10 = {"%r10", "%r10d", "%r10w", "%r10b"};
const Register R11 = {"%r11", "%r11d", "%r11w", "%r11b"};
const Register R12 = {"%r12", "%r12d", "%r12w", "%r12b"};
const Register R13 = {"%r13", "%r13d", "%r13w", "%r13b"};
const Register R14 = {"%r14", "%r14d", "%r14w", "%r14b"};
const Register R15 = {"%r15", "%r15d", "%r15w", "%r15b"};
