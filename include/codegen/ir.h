#pragma once 
#include "codegen/base.h"
#include "type_system/func.h"
#include "program.h"

class ir : public BaseCodegenerator{
public:
    ir(Program* prog) : BaseCodegenerator(prog) {}

    void compileProgram()  override;
    void compileFunction(Func func) override;
};
