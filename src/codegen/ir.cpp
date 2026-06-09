#include "codegen/ir.h"
#include <any>
#include <cstdint>
#include <cstdio>
#include <string>
#include <fstream>
#include "tools/format.h"
#include "context.h"
#include "operations.h"
#include "type_system/variable.h"
#include "type_system/type.h"

void ir::emitReturn(Return_Ast* nd) {
    TODO("emitReturn");
}
void ir::emitIf(If_Ast* nd) { 
    TODO("emitIf");
}
void ir::emitJump(Jump_Ast* nd) { 
    TODO("emitJump");
}
void ir::emitJumpIfNot(JumpIfNot_Ast* nd) { 
    TODO("emitJumpIfNot");
}
void ir::emitLabel(Label_Ast* nd) {
    TODO("emitLabel");
}
Memory ir::emitLoad(Load_Ast* nd) {
    TODO("emitLoad");
}
Memory ir::emitRef(Ref_Ast* nd) {
    TODO("emitRef");
}
Memory ir::emitDeref(Deref_Ast* nd) {
    TODO("emitDeref");
}
Memory ir::emitCall(Call_Ast* nd) {
    TODO("emitCall");
}
Memory ir::emitStore(Store_Ast* nd) {
    TODO("emitStore");
}
Memory ir::emitBinOp(BinOp_Ast* nd) {
    TODO("emitBinOp");
}
Memory ir::getVarPtr(Variable var) {
    TODO("getVarPtr");
}
void ir::compileProgram() {
    if (m_program == nullptr) return;
    for (auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    std::ofstream outfile(mlog::format("{}.ir", (build_path/input_file.stem()).string()));
    outfile << output;
    outfile.close();
}
void ir::compileFunction(Func& func) {
    output.append(mlog::format("{}:\n", func.name));
//    for (auto& inst : func.body) {
//        switch (inst.op) {
//            case Op::RETURN: {
//                if (std::get<Variable>(inst.args[0]).type.info.id == (int)TypeId::Int32)
//                    output.appendf("    ret({})\n", std::get<Variable>(inst.args[1]).Int_val);
//            }break;
//            case Op::CALL: {
//                Func            func = std::get<Func>(inst.args[0]);
//                VariableStorage args  = std::get<VariableStorage>(inst.args[1]);
//                output.appendf("    {}(", func.name);
//                for (size_t i = 0; i < args.size(); i++) {
//                    output.appendf("$({})", args[i].name);
//                    if (i+1 < args.size())
//                        output.appendf(", ");
//                }
//                output.appendf(")\n");
//
//            }break;
//        }
//    }
}
