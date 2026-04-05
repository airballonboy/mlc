#pragma once 
#include "codegen/base.h"
#include "type_system/func.h"
#include "program.h"

class ir : public BaseCodegen{
public:
    ir(Program* prog) : BaseCodegen(prog) {}

    void compileProgram()  override;
    void compileFunction(Func& func) override;
};
