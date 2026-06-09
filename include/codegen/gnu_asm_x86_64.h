#pragma once 
#include <unordered_set>
#include "codegen/asm_instruction.h"
#include "codegen/base.h"
#include "operations.h"
#include "type_system/variable.h"
#include "type_system/func.h"
#include "program.h"

class gnu_asm : public BaseCodegen {
public:
    gnu_asm(Program* prog);

    void call_func(Func& func, std::vector<Node> args, Memory* ret_mem = nullptr);
    void call_func_windows(Func& func, std::vector<Node> args, Memory* ret_mem = nullptr);
    void call_func_linux(Func& func, std::vector<Node> args, Memory* ret_mem = nullptr);
    void compileProgram() override;
    void compileFunction(Func& func) override;
    void compileConstant(Variable var);

    void mov_var(Variable src   , Register dest);
    void mov_var(Register src   , Variable dest);
    void mov_var(Variable src   , Variable dest);
    void mov_member(Variable src, Register dest);
    Memory get_member_ptr(Variable var);
    void deref(Register, int64_t deref_count);
    void cast_float_size(Register reg, size_t orig_size, size_t new_size);
    void cast_int_size(Register reg, size_t orig_size, size_t new_size);

public:
    void   emitReturn   (Return_Ast*    nd) override;
    void   emitJump     (Jump_Ast*      nd) override;
    void   emitJumpIfNot(JumpIfNot_Ast* nd) override;
    void   emitLabel    (Label_Ast*     nd) override;
    void   emitIf       (If_Ast*        nd) override;
    Memory emitLoad     (Load_Ast*      nd) override;
    Memory emitRef      (Ref_Ast*       nd) override;
    Memory emitDeref    (Deref_Ast*     nd) override;
    Memory emitCall     (Call_Ast*      nd) override;
    Memory emitStore    (Store_Ast*     nd) override;
    Memory emitBinOp    (BinOp_Ast*     nd) override;
    Memory getVarPtr    (Variable var)      override;

public:
    std::vector<Register> arg_register;
    std::vector<Register> arg_register_float;
private:
    void function_prologue();
    void get_func_args_windows(Func& func);
    void get_func_args_linux(Func& func);
    void function_epilogue();
    AsmInstruction& get_binop(BinOp bin_op, bool is_float);
    AsmInstruction& get_compare_binop(BinOp bin_op, bool is_float);

public:
    AsmInstruction movabs = AsmInstruction("movabs");                   ;
    AsmInstruction lea    = AsmInstruction("lea" , {"q", "q", "q", "q"});
    AsmInstruction cmp    = AsmInstruction("cmp");
    AsmInstruction mov    = AsmInstruction("mov");
    AsmInstruction add    = AsmInstruction("add");
    AsmInstruction sub    = AsmInstruction("sub");
    AsmInstruction imul   = AsmInstruction("imul");
    AsmInstruction idiv   = AsmInstruction("idiv");
    AsmInstruction adds   = AsmInstruction("adds" , {"d", "s", "s", "s"});
    AsmInstruction movs   = AsmInstruction("movs" , {"d", "s", "s", "s"});
    AsmInstruction subs   = AsmInstruction("subs" , {"d", "s", "s", "s"});
    AsmInstruction muls   = AsmInstruction("muls" , {"d", "s", "s", "s"});
    AsmInstruction divs   = AsmInstruction("divs" , {"d", "s", "s", "s"});
    AsmInstruction comis  = AsmInstruction("comis", {"d", "s", "s", "s"});
    AsmInstruction sete   = AsmInstruction("sete" , {"", "", "", ""});
    AsmInstruction setne  = AsmInstruction("setne", {"", "", "", ""});
    AsmInstruction setl   = AsmInstruction("setl" , {"", "", "", ""});
    AsmInstruction setle  = AsmInstruction("setle", {"", "", "", ""});
    AsmInstruction setg   = AsmInstruction("setg" , {"", "", "", ""});
    AsmInstruction setge  = AsmInstruction("setge", {"", "", "", ""});
    AsmInstruction setb   = AsmInstruction("setb" , {"", "", "", ""});
    AsmInstruction setbe  = AsmInstruction("setbe", {"", "", "", ""});
    AsmInstruction seta   = AsmInstruction("seta" , {"", "", "", ""});
    AsmInstruction setnb  = AsmInstruction("setnb", {"", "", "", ""});

    AsmInstruction and_   = AsmInstruction("and");
    AsmInstruction or_    = AsmInstruction("or");
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


inline bool is_float_reg(Register reg) {return xmm.contains(reg._64);}

static std::vector<std::pair<Register, bool>> available_reg = {
    {Rax, true},
    {Rbx, true},
    {R13, true},
    {R14, true},
    {R15, true},
};
static std::vector<std::pair<Register, bool>> available_float_reg = {
    {Xmm12, true},
    {Xmm13, true},
    {Xmm14, true},
    {Xmm15, true},
};

inline Register get_available_int_reg() {
    if (available_reg.size() < 1) TODO("no available Registers");
    Register reg;
    for (auto& [reg_, avail] : available_reg) {
        if (avail) {
            reg = reg_;
            avail = false;
            break;
        }
    }
    return reg;
}
inline Register get_available_float_reg() {
    if (available_float_reg.size() < 1) TODO("no available Registers");
    Register reg;
    for (auto& [reg_, avail] : available_float_reg) {
        if (avail) {
            reg = reg_;
            avail = false;
            break;
        }
    }
    return reg;
}

inline void free_int_reg(Register reg) {
    if (r64.contains(reg._64)) {
        for (auto& [reg_, avail] : available_reg) {
            if (reg_._64 == reg._64) {
                avail = true;
                break;
            }
        }
    } else 
        TODO("register doesn't exist");
}
inline void free_float_reg(Register reg) {
    if (xmm.contains(reg._64)) {
        for (auto& [reg_, avail] : available_float_reg) {
            if (reg_._64 == reg._64) {
                avail = true;
                break;
            }
        }
    } else 
        TODO("register doesn't exist");
}
inline void free_mem(Memory mem) {
    switch (mem.asm_mem.type) {
        case AsmType::Reg: {
            if (is_float_reg(mem.asm_mem.reg))
                free_float_reg(mem.asm_mem.reg);
            else
                free_int_reg(mem.asm_mem.reg);
        } break;
        case AsmType::OffReg: {
            if (is_float_reg(mem.asm_mem.off_reg))
                free_float_reg(mem.asm_mem.off_reg);
            else
                free_int_reg(mem.asm_mem.off_reg);
        } break;
        case AsmType::TWO_Reg: {
            if (is_float_reg(mem.asm_mem.reg1)) free_float_reg(mem.asm_mem.reg1);
            else                               free_int_reg(mem.asm_mem.reg1);
            if (is_float_reg(mem.asm_mem.reg2)) free_float_reg(mem.asm_mem.reg2);
            else                               free_int_reg(mem.asm_mem.reg2);
        } break;

        case AsmType::ArrayIndex: 
        case AsmType::Global: 
        case AsmType::None:
            TODO("");
    }
}
