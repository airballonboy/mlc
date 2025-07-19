#pragma once 
#include "codegen/base.h"
#include "types.h"

class gnu_asm : public BaseCodegenerator {
public:

    gnu_asm(Program* prog) : BaseCodegenerator(prog) {}

    void compileProgram()  override;
    void compileFunction(Func func) override;
};
