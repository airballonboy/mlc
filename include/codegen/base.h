#pragma once
#include "types.h"

class BaseCodegenerator {
public:
    BaseCodegenerator();
    BaseCodegenerator(Program* prog) : m_program(prog) {};
    
    virtual void compileProgram() {};
protected:
    Program* m_program;
};
