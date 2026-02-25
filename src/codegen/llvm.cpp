#include "codegen/llvm.h"
#include "codegen/base.h"
#include "context.h"
#include "program.h"
#include "tools/logger.h"
#include "type_system/variable.h"
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include <unordered_map>
#include <vector>

using std::unique_ptr;
using std::make_unique;



llvm_gen::llvm_gen(Program* prog) : BaseCodegenerator(prog) {
    m_ctx     = make_unique<llvm::LLVMContext>();
    m_mod     = make_unique<llvm::Module>("mlang_test", *m_ctx);
    m_builder = make_unique<llvm::IRBuilder<>>(*m_ctx);

    typeid_to_type = {
        {type_infos.at("void").id    , llvm::Type::getVoidTy(*m_ctx)},
        {type_infos.at("char").id    , llvm::Type::getInt8Ty(*m_ctx)},
        {type_infos.at("int8").id    , llvm::Type::getInt8Ty(*m_ctx)},
        {type_infos.at("int16").id   , llvm::Type::getInt16Ty(*m_ctx)},
        {type_infos.at("int32").id   , llvm::Type::getInt32Ty(*m_ctx)},
        {type_infos.at("int64").id   , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("typeid").id  , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("pointer").id , llvm::Type::getInt8Ty(*m_ctx)->getPointerTo()},
        {type_infos.at("usize").id   , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("string").id  , llvm::Type::getInt8Ty(*m_ctx)->getPointerTo()},
        {type_infos.at("float").id   , llvm::Type::getFloatTy(*m_ctx)},
        {type_infos.at("double").id  , llvm::Type::getDoubleTy(*m_ctx)},
        {type_infos.at("bool").id    , llvm::Type::getInt8Ty(*m_ctx)}
    };
}
void llvm_gen::compileProgram() {
    if (m_program == nullptr) return;
    for (const auto& func : m_program->func_storage) {
        if (!func.external && (func.is_used || ctx.lib)) {
            compileFunction(func);
        }
    }
}
void llvm_gen::compileFunction(Func func) {
    std::vector<llvm::Type*> args;
    for (auto arg : func.arguments) {
        args.push_back(typeid_to_type.at(arg.type_info.id));
    }
    auto ft = llvm::FunctionType::get(typeid_to_type.at(func.return_type.id), args, false);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func.name, m_mod.get());
    auto body = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(body);

    bool returned = false;
    for (auto& inst : func.body) {
        returned = false;
        switch (inst.op) {
            case Op::RETURN: {
                Variable arg = std::get<Variable>(inst.args[0]);
                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::get<Variable>(inst.args[0]);
                Variable var2 = std::get<Variable>(inst.args[1]);
            }break;
            case Op::INIT_STRING: {
                Variable str = std::get<Variable>(inst.args[0]);
            }break;
            case Op::CALL: {
                Func func             = std::get<Func>(inst.args[0]);
                VariableStorage args  = std::get<VariableStorage>(inst.args[1]);
                Variable ret_address  = std::get<Variable>(inst.args[2]);

                call_func(func, args);
            }break;
            case Op::JUMP_IF_NOT: {
                std::string label = std::get<std::string>(inst.args[0]);
                Variable    expr = std::get<Variable>(inst.args[1]);
            }break;
            case Op::JUMP: {
                std::string label = std::get<std::string>(inst.args[0]);
            }break;
            case Op::LABEL: {
                std::string label = std::get<std::string>(inst.args[0]);
            }break;
            case Op::ADD: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::SUB: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::MUL: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::DIV: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::MOD: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::EQ: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::NE: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::LT: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::LE: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::GT: { 
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::GE: { 
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::LAND: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
            case Op::LOR: {
                Variable lhs = std::get<Variable>(inst.args[0]);
                Variable rhs = std::get<Variable>(inst.args[1]);
                Variable result = std::get<Variable>(inst.args[2]);
            } break;
        }
    }
    if (!returned) {
        auto ret = llvm::ConstantInt::get(*m_ctx, llvm::APInt(8, 0));
        m_builder->CreateRet(ret);
    }
    llvm::verifyFunction(*fn);

    fn->print(llvm::outs());
}
void llvm_gen::call_func(Func func, VariableStorage args) {

}
