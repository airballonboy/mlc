#pragma once 
#include <unordered_set>
#include "codegen/instruction.h"
#include "codegen/base.h"
#include "type_system/variable.h"
#include "type_system/func.h"
#include "program.h"

class gnu_asm : public BaseCodegenerator {
public:

    gnu_asm(Program* prog);

    void call_func(Func func_name, VariableStorage args);
    void call_func_windows(Func func_name, VariableStorage args);
    void call_func_linux(Func func_name, VariableStorage args);
    void compileProgram() override;
    void compileFunction(Func func) override;
    void compileConstant(Variable var);

    std::vector<Register> arg_register;
    std::vector<Register> arg_register_float;

    AsmInstruction movabs = AsmInstruction("movabs", output);
    AsmInstruction lea    = AsmInstruction("lea", output);
    AsmInstruction cmp    = AsmInstruction("cmp", output);
    AsmInstruction mov    = AsmInstruction("mov", output);
    AsmInstruction add    = AsmInstruction("add", output);
    AsmInstruction sub    = AsmInstruction("sub", output);
    AsmInstruction imul   = AsmInstruction("imul", output);
    AsmInstruction idiv   = AsmInstruction("idiv", output);
    AsmInstruction adds   = AsmInstruction("adds", output, {"d", "s", "s", "s"});
    AsmInstruction movs   = AsmInstruction("movs", output, {"d", "s", "s", "s"});
    AsmInstruction subs   = AsmInstruction("subs", output, {"d", "s", "s", "s"});
    AsmInstruction muls   = AsmInstruction("muls", output, {"d", "s", "s", "s"});
    AsmInstruction divs   = AsmInstruction("divs", output, {"d", "s", "s", "s"});

    void mov_var(Variable src   , Register dest);
    void mov_var(Register src   , Variable dest);
    void mov_var(Variable src   , Variable dest);
    void mov_member(Variable src, Register dest);
    void mov_member(Register src, Variable dest);
    void deref(Register, int64_t deref_count);
    void cast_float_size(Register reg, size_t orig_size, size_t new_size);

    Struct& get_struct_from_name(std::string& name);
private:
    void function_prologue();
    void function_epilogue();
};

const Register Rip   = {"%rip"  , "%rip"  , "%rip"  , "%rip"};
const Register Rax   = {"%rax"  , "%eax"  , "%ax"   , "%al"};
const Register Rbx   = {"%rbx"  , "%ebx"  , "%bx"   , "%bl"};
const Register Rcx   = {"%rcx"  , "%ecx"  , "%cx"   , "%cl"};
const Register Rdx   = {"%rdx"  , "%edx"  , "%dx"   , "%dl"};
const Register Rsi   = {"%rsi"  , "%esi"  , "%si"   , "%sil"};
const Register Rdi   = {"%rdi"  , "%edi"  , "%di"   , "%dil"};
const Register Rbp   = {"%rbp"  , "%ebp"  , "%bp"   , "%bpl"};
const Register Rsp   = {"%rsp"  , "%esp"  , "%sp"   , "%spl"};
const Register R8    = {"%r8"   , "%r8d"  , "%r8w"  , "%r8b"};
const Register R9    = {"%r9"   , "%r9d"  , "%r9w"  , "%r9b"};
const Register R10   = {"%r10"  , "%r10d" , "%r10w" , "%r10b"};
const Register R11   = {"%r11"  , "%r11d" , "%r11w" , "%r11b"};
const Register R12   = {"%r12"  , "%r12d" , "%r12w" , "%r12b"};
const Register R13   = {"%r13"  , "%r13d" , "%r13w" , "%r13b"};
const Register R14   = {"%r14"  , "%r14d" , "%r14w" , "%r14b"};
const Register R15   = {"%r15"  , "%r15d" , "%r15w" , "%r15b"};
const Register Xmm0  = {"%xmm0" , "%xmm0" , "%xmm0" , "%xmm0"};
const Register Xmm1  = {"%xmm1" , "%xmm1" , "%xmm1" , "%xmm1"};
const Register Xmm2  = {"%xmm2" , "%xmm2" , "%xmm2" , "%xmm2"};
const Register Xmm3  = {"%xmm3" , "%xmm3" , "%xmm3" , "%xmm3"};
const Register Xmm4  = {"%xmm4" , "%xmm4" , "%xmm4" , "%xmm4"};
const Register Xmm5  = {"%xmm5" , "%xmm5" , "%xmm5" , "%xmm5"};
const Register Xmm6  = {"%xmm6" , "%xmm6" , "%xmm6" , "%xmm6"};
const Register Xmm7  = {"%xmm7" , "%xmm7" , "%xmm7" , "%xmm7"};
const Register Xmm8  = {"%xmm8" , "%xmm8" , "%xmm8" , "%xmm8"};
const Register Xmm9  = {"%xmm9" , "%xmm9" , "%xmm9" , "%xmm9"};
const Register Xmm10 = {"%xmm10", "%xmm10", "%xmm10", "%xmm10"};
const Register Xmm11 = {"%xmm11", "%xmm11", "%xmm11", "%xmm11"};
const Register Xmm12 = {"%xmm12", "%xmm12", "%xmm12", "%xmm12"};
const Register Xmm13 = {"%xmm13", "%xmm13", "%xmm13", "%xmm13"};
const Register Xmm14 = {"%xmm14", "%xmm14", "%xmm14", "%xmm14"};
const Register Xmm15 = {"%xmm15", "%xmm15", "%xmm15", "%xmm15"};
const static std::unordered_set<std::string_view> r64 = {
    Rip._64, Rax._64, Rbx._64, Rcx._64, Rdx._64, Rsi._64, Rdi._64, Rbp._64, Rsp._64, R8._64,
    R9._64 , R10._64, R11._64, R12._64, R13._64, R14._64, R15._64,};
const static std::unordered_set<std::string_view> xmm = {
    Xmm0._64, Xmm1._64, Xmm2._64, Xmm3._64, Xmm4._64, Xmm5._64, Xmm6._64, Xmm7._64, Xmm8._64,
    Xmm9._64, Xmm10._64, Xmm11._64, Xmm12._64, Xmm13._64, Xmm14._64, Xmm15._64
};
