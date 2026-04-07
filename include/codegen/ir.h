#pragma once 
#include "codegen/base.h"
#include "type_system/func.h"
#include "program.h"

class ir : public BaseCodegen {
public:
    ir(Program* prog) : BaseCodegen(prog) {}

    Memory emitLoad(Loc loc, Variable var) override;
    Memory emitRef(Loc loc, Variable var) override;
    Memory emitDeref(Loc loc, Memory lhs) override;
    Memory emitCall(Loc loc, Memory func, std::vector<Node> args) override;
    void   emitLabel(Loc loc, std::string label) override;
    void   emitJump(Loc loc, std::string label) override;
    void   emitJumpIfNot(Loc loc, std::string label, Memory cond) override;
    void   emitReturn(Loc loc, Memory ret) override;
    Memory emitStore(Loc loc, Memory lhs, Memory rhs) override;
    Memory emitBinOp(Loc loc, BinOp op, Memory lhs, Memory rhs) override;

    void compileProgram()  override;
    void compileFunction(Func& func) override;
};
