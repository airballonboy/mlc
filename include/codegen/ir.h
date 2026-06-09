#pragma once 
#include "codegen/base.h"
#include "type_system/func.h"
#include "program.h"

class ir : public BaseCodegen {
public:
    ir(Program* prog) : BaseCodegen(prog) {}

    void   emitReturn   (Return_Ast*    nd) override;
    void   emitJump     (Jump_Ast*      nd) override;
    void   emitJumpIfNot(JumpIfNot_Ast* nd) override;
    void   emitLabel    (Label_Ast*     nd) override;
    void   emitIf       (If_Ast*        nd) override;
    Memory emitLoad     (Load_Ast*      nd) override;
    Memory emitRef      (Ref_Ast*       nd) override;
    Memory emitDeref    (Deref_Ast*     nd) override;
    Memory emitCall     (Call_Ast*      nd) override;
    Memory emitStore    (Store_Ast*     nd) override;
    Memory emitBinOp    (BinOp_Ast*     nd) override;
    Memory getVarPtr    (Variable var)      override;

    void compileProgram()  override;
    void compileFunction(Func& func) override;
};
