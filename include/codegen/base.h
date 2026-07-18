#pragma once
#include "program.h"


class Return_Ast;
class Jump_Ast;
class JumpIfNot_Ast;
class Label_Ast;
class Load_Ast;
class Ref_Ast;
class Deref_Ast;
class Call_Ast;
class Store_Ast;
class BinOp_Ast;
class If_Ast;
class Loop_Ast;

class BaseCodegen {
public:
    BaseCodegen();
    virtual ~BaseCodegen() {};
    BaseCodegen(Program* prog) : m_program(prog) {}

    virtual void compileProgram() {}
    virtual void compileFunction(Func& func) {}

public:
    virtual void   emitReturn   (Return_Ast* nd)    { TODO("unimplemented"); }
    virtual void   emitJump     (Jump_Ast* nd)      { TODO("unimplemented"); }
    virtual void   emitJumpIfNot(JumpIfNot_Ast* nd) { TODO("unimplemented"); }
    virtual void   emitLabel    (Label_Ast* nd)     { TODO("unimplemented"); }
    virtual void   emitIf       (If_Ast* nd)        { TODO("unimplemented"); }
    virtual void   emitLoop     (Loop_Ast* nd)      { TODO("unimplemented"); }
    virtual Memory emitLoad     (Load_Ast* nd)      { TODO("unimplemented"); }
    virtual Memory emitRef      (Ref_Ast* nd)       { TODO("unimplemented"); }
    virtual Memory emitDeref    (Deref_Ast* nd)     { TODO("unimplemented"); }
    virtual Memory emitCall     (Call_Ast* nd)      { TODO("unimplemented"); }
    virtual Memory emitStore    (Store_Ast* nd)     { TODO("unimplemented"); }
    virtual Memory emitBinOp    (BinOp_Ast* nd)     { TODO("unimplemented"); }
    virtual Memory getVarPtr    (Variable var)      { TODO("unimplemented"); }
protected:
    Program* m_program;
    Func*    m_func;
    std::string output;
};
