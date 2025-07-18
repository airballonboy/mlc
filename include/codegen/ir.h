#pragma once 
#include "codegen/base.h"
#include "types.h"

class ir : public BaseCodegenerator{
public:
    ir(Program* prog) : BaseCodegenerator(prog) {}

    void compileProgram()  override;
    void compileFunction(Func func) override;
    std::string output;
};
