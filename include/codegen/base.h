#pragma once
#include "type_system/func.h"
#include "program.h"

class BaseCodegenerator {
public:
    BaseCodegenerator();
    BaseCodegenerator(Program* prog) : m_program(prog) {}
    
    virtual void compileProgram() {}
    virtual void compileFunction(Func func) {}
protected:
    Program* m_program;
    std::string output;
};
