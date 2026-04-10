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

Memory ir::emitLoad(Loc loc, Variable var) { }
Memory ir::emitRef(Loc loc, Variable var) { }
Memory ir::emitDeref(Loc loc, Memory lhs) { }
Memory ir::emitCall(Loc loc, Func& func, std::vector<Node> args) { }
void   ir::emitLabel(Loc loc, std::string label) { }
void   ir::emitJump(Loc loc, std::string label) { }
void   ir::emitJumpIfNot(Loc loc, std::string label, Memory cond) { }
void   ir::emitReturn(Loc loc, Memory ret) { }
Memory ir::emitStore(Loc loc, Memory lhs, Memory rhs) { }
Memory ir::emitBinOp(Loc loc, BinOp op, Memory lhs, Memory rhs) { }
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
