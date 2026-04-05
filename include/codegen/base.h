#pragma once
#include "program.h"
#include "token.h"
#include "type_system/variable.h"

class BaseCodegen {
public:
    BaseCodegen();
    BaseCodegen(Program* prog) : m_program(prog) {}

    virtual void compileProgram() {}
    virtual void compileFunction(Func& func) {}

public:
    virtual void   emitReturn(Loc loc, Memory ret);
    virtual void   emitJump(Loc loc, std::string label);
    virtual void   emitJumpIfNot(Loc loc, std::string label, Memory cond);
    virtual void   emitLabel(Loc loc, std::string Label);
    virtual Memory emitLoad(Loc loc, Variable var);
    virtual Memory emitRef(Loc loc, Variable var);
    virtual Memory emitDeref(Loc loc, Memory lhs);
    virtual Memory emitCall(Loc loc, Memory func, std::vector<Node> args);
    virtual Memory emitStore(Loc loc, Memory lhs, Memory rhs);
    virtual Memory emitBinOp(Loc loc, BinOp op, Memory lhs, Memory rhs);
protected:
    Program* m_program;
    Func*    m_func;
    std::string output;
};
