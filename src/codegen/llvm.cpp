#include "codegen/llvm.h"
#include "codegen/base.h"
#include "context.h"
#include "platform.h"
#include "program.h"
#include "tools/logger.h"
#include "type_system/variable.h"
#include <any>
#include <cstdlib>
#include <llvm-14/llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"
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
        compileFunction(func);
    }
    compile_start_func();
    output_object_file();
}
void llvm_gen::compileFunction(Func func) {
    if (func.name.starts_with("TypeInfo")) return;
    std::vector<llvm::Type*> args;
    for (auto arg : func.arguments) {
        args.push_back(typeid_to_type.at(arg.type_info.id));
    }
    auto ft = llvm::FunctionType::get(typeid_to_type.at(func.return_type.id), args, false);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func.name, m_mod.get());
    if (func.external) return;
    auto body = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(body);

    bool returned = false;
    for (auto& inst : func.body) {
        returned = false;
        switch (inst.op) {
            case Op::RETURN: {
                Variable arg = std::get<Variable>(inst.args[0]);

                auto ret = llvm::ConstantInt::get(*m_ctx, llvm::APInt(8, 0));
                m_builder->CreateRet(ret);

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
llvm::Value* llvm_gen::call_func(Func func, VariableStorage args) {
    auto llvm_func = m_mod->getFunction(func.name);
    std::vector<llvm::Value*> arguments;
    for (auto& arg : args) {
        if (arg.type_info.type == Type::Int8_t||arg.type_info.type == Type::Int16_t||arg.type_info.type == Type::Int32_t||arg.type_info.type == Type::Int64_t) {
            arguments.push_back(llvm::ConstantInt::getSigned(typeid_to_type.at(arg.type_info.id), std::any_cast<int64_t>(arg.value)));
        }
    }
    return m_builder->CreateCall(llvm_func, arguments);
}
void llvm_gen::compile_start_func() {
    auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*m_ctx), {}, false);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "_start", m_mod.get());
    auto body = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(body);
    auto mainfn = m_mod->getFunction("main");
    auto exitfn = m_mod->getFunction("exit");
    if (!mainfn) {
        mlog::println("main function was not found");
        abort();
    }
    if (!exitfn) {
        mlog::println("exit function was not found");
        abort();
    }
    auto exit_code = m_builder->CreateCall(mainfn);
    m_builder->CreateCall(exitfn, {exit_code});
    m_builder->CreateUnreachable();
    llvm::verifyFunction(*fn);
    fn->print(llvm::outs());
}
void llvm_gen::output_object_file() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string triple{};
    if (m_program->platform == Platform::Linux)
        triple = "x86_64-pc-linux-gnu";
    if (m_program->platform == Platform::Windows)
        triple = "x86_64-pc-windows-gnu";
    m_mod->setTargetTriple(triple);

    std::string err;
    auto target = llvm::TargetRegistry::lookupTarget(triple, err);

    auto tm = target->createTargetMachine(
        triple, "generic", "", llvm::TargetOptions(), llvm::Optional<llvm::Reloc::Model>()
    );

    m_mod->setDataLayout(tm->createDataLayout());

    // --- emit object ---
    std::error_code ec;
    llvm::raw_fd_ostream out(output_path.string()+".o", ec);

    llvm::legacy::PassManager pm;
    tm->addPassesToEmitFile(pm, out, nullptr, llvm::CGFT_ObjectFile);
    pm.run(*m_mod);
    out.flush();
}
