#pragma once
#include "program.h"
#include "token.h"
#include "type_system/variable.h"

class BaseCodegen {
public:
    BaseCodegen();
    virtual ~BaseCodegen() {};
    BaseCodegen(Program* prog) : m_program(prog) {}

    virtual void compileProgram() {}
    virtual void compileFunction(Func& func) {}

public:
    virtual void   emitReturn(Loc loc, Memory ret)                        {TODO("unimplemented");}
    virtual void   emitJump(Loc loc, std::string label)                   {TODO("unimplemented");}
    virtual void   emitJumpIfNot(Loc loc, std::string label, Memory cond) {TODO("unimplemented");}
    virtual void   emitLabel(Loc loc, std::string Label)                  {TODO("unimplemented");}
    virtual Memory emitLoad(Loc loc, Variable var)                        {TODO("unimplemented");}
    virtual Memory emitRef(Loc loc, Variable var)                         {TODO("unimplemented");}
    virtual Memory emitDeref(Loc loc, Memory lhs)                         {TODO("unimplemented");}
    virtual Memory emitCall(Loc loc, Func& func, std::vector<Node> args) {TODO("unimplemented");}
    virtual Memory emitStore(Loc loc, Memory lhs, Memory rhs)             {TODO("unimplemented");}
    virtual Memory emitBinOp(Loc loc, BinOp op, Memory lhs, Memory rhs)   {TODO("unimplemented");}
protected:
    Program* m_program;
    Func*    m_func;
    std::string output;
};
