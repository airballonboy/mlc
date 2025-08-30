#pragma once 
#include "codegen/base.h"
#include "types.h"

class gnu_asm : public BaseCodegenerator {
public:

    gnu_asm(Program* prog) : BaseCodegenerator(prog) {}

    void move_var_to_reg(Variable arg, std::string_view reg);
    void move_reg_to_var(std::string_view reg, Variable arg2);
    void move_var_to_var(Variable arg1, Variable arg2);
    void move_reg_to_reg(std::string_view reg1, std::string_view reg2);
    void deref_var_to_reg(Variable arg, std::string_view reg);
    void call_func(std::string func_name, VariableStorage args);
    void compileProgram() override;
    void compileFunction(Func func) override;
};
