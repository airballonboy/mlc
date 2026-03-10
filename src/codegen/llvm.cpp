#include "codegen/llvm.h"
#include "codegen/base.h"
#include "context.h"
#include "platform.h"
#include "program.h"
#include "tools/logger.h"
#include "type_system/kind.h"
#include "type_system/type_info.h"
#include "type_system/variable.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <vector>

using std::unique_ptr;
using std::make_unique;

std::unordered_map<std::string, llvm::AllocaInst*> allocated_vars{};


llvm_gen::llvm_gen(Program* prog) : BaseCodegenerator(prog) {
    m_ctx     = make_unique<llvm::LLVMContext>();
    m_mod     = make_unique<llvm::Module>(input_file.string(), *m_ctx);
    m_builder = make_unique<llvm::IRBuilder<>>(*m_ctx);

    typeid_to_type = {
        {type_infos.at("void").id    , llvm::Type::getVoidTy(*m_ctx)},
        {type_infos.at("char").id    , llvm::Type::getInt8Ty(*m_ctx)},
        {type_infos.at("int8").id    , llvm::Type::getInt8Ty(*m_ctx)},
        {type_infos.at("int16").id   , llvm::Type::getInt16Ty(*m_ctx)},
        {type_infos.at("int32").id   , llvm::Type::getInt32Ty(*m_ctx)},
        {type_infos.at("int64").id   , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("typeid").id  , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("pointer").id , llvm::PointerType::get(*m_ctx, 0)},
        {type_infos.at("usize").id   , llvm::Type::getInt64Ty(*m_ctx)},
        {type_infos.at("string").id  , llvm::PointerType::get(*m_ctx, 0)},
        {type_infos.at("float").id   , llvm::Type::getFloatTy(*m_ctx)},
        {type_infos.at("double").id  , llvm::Type::getDoubleTy(*m_ctx)},
        {type_infos.at("bool").id    , llvm::Type::getInt8Ty(*m_ctx)}
    };
    for (auto &[name, info] : type_infos) {
        if (info.kind == Kind::Struct) {
            auto* llvm_strct = llvm::StructType::create(*m_ctx, info.name);
            auto strct = Struct::get_from_name(info.name, m_program->struct_storage);
            std::vector<llvm::Type*> body{};
            for (auto var : strct.var_storage) {
                body.push_back(typeid_to_type.at(var.type.info.id));
            }
            llvm_strct->setBody(body);
            typeid_to_type.emplace(info.id, llvm_strct);
        }
    }
}
void llvm_gen::compileProgram() {
    if (m_program == nullptr) return;
    for (const auto& func : m_program->func_storage) {
        compileFunction(func);
    }
    m_mod->print(llvm::outs(), nullptr);
    std::string mod_err;
    llvm::raw_string_ostream mod_err_stream(mod_err);
    if (llvm::verifyModule(*m_mod, &mod_err_stream)) {
        mlog::println("----------------------------------------");
        mlog::println(mlog::format("Module verification failed: {}", mod_err));
        abort();
    }
    output_object_file();
}
void llvm_gen::compileFunction(Func func) {
    std::vector<llvm::Type*> args;
    for (auto arg : func.arguments) {
        args.push_back(typeid_to_type.at(arg.type.info.id));
    }
    auto func_ret_type = *func.type.func_data->return_type;
    auto ft = llvm::FunctionType::get(typeid_to_type.at(func_ret_type.info.id), args, false);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func.name, m_mod.get());
    if (func.external) return;
    auto func_entry = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(func_entry);

    bool returned = false;
    for (auto& inst : func.body) {
        returned = false;
        switch (inst.op) {
            case Op::RETURN: {
                Variable arg = std::get<Variable>(inst.args[0]);

                auto ret = var_to_value(arg);
                mlog::println("{}: {} -> {}", func.name, arg.type.info.name, func_ret_type.info.name);
                if (func_ret_type.info.kind == Kind::Int) {
                    ret = m_builder->CreateZExt(ret, typeid_to_type.at(func_ret_type.info.id));
                    m_builder->CreateRet(ret);
                } else if (fn->getReturnType()->isVoidTy())
                    m_builder->CreateRetVoid();
                else {
                    m_builder->CreateRet(ret);
                }

                returned = true;
            }break;
            case Op::STORE_VAR: {
                Variable var1 = std::get<Variable>(inst.args[0]);
                Variable var2 = std::get<Variable>(inst.args[1]);
                if (var2.parent != nullptr) {
                    auto par = var2;
                    std::vector<size_t> iters{};
                    while (par.parent != nullptr) {
                        auto strct = Struct::get_from_name(par.parent->type.info.name, m_program->struct_storage);
                        iters.push_back(strct.get_element_iterator(par.name));
                        par = *par.parent;
                    }
                    if (!allocated_vars.contains(par.name)) {
                        llvm::AllocaInst* var = m_builder->CreateAlloca(typeid_to_type.at(par.type.info.id), nullptr, par.name);
                        allocated_vars.emplace(par.name, var);
                    }
                    auto llvm_par = allocated_vars[par.name];
                    llvm::Value* elem = llvm_par;
                    while (iters.size() > 0) {
                        auto iter = iters.back();
                        iters.pop_back();

                        elem = m_builder->CreateStructGEP(typeid_to_type.at(par.type.info.id), elem, iter);
                        par = par.members[iter];
                    }
                    m_builder->CreateStore(var_to_value(var1), elem);
                } else {
                    if (!allocated_vars.contains(var2.name)) {
                        llvm::AllocaInst* var = m_builder->CreateAlloca(typeid_to_type.at(var2.type.info.id), nullptr, var2.name);
                        allocated_vars.emplace(var2.name, var);
                    }
                    auto llvm_var2 = allocated_vars[var2.name];
                    m_builder->CreateStore(var_to_value(var1), llvm_var2);
                }
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

                llvm::Value* val = var_to_value(expr);
                if (!val->getType()->isIntegerTy(1)) {
                    val = m_builder->CreateICmpNE(val, llvm::ConstantInt::get(val->getType(), 0));
                }
                auto label_block = find_block(fn, label);
                llvm::BasicBlock* cont_bb = llvm::BasicBlock::Create(*m_ctx);

                m_builder->CreateCondBr(val, label_block, cont_bb);
                m_builder->SetInsertPoint(cont_bb);
            }break;
            case Op::JUMP: {
                std::string label = std::get<std::string>(inst.args[0]);
                m_builder->CreateBr(find_block(fn, label));
            }break;
            case Op::LABEL: {
                std::string label = std::get<std::string>(inst.args[0]);
                auto body = llvm::BasicBlock::Create(*m_ctx, label, fn);
                m_builder->CreateBr(body);
                m_builder->SetInsertPoint(body);
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
        auto ret = llvm::ConstantInt::get(*m_ctx, llvm::APInt(fn->getReturnType()->getIntegerBitWidth(), 0));
        m_builder->CreateRet(ret);
    }
    llvm::verifyFunction(*fn);
}
llvm::Value* llvm_gen::call_func(Func func, VariableStorage args) {
    auto llvm_func = m_mod->getFunction(func.name);
    std::vector<llvm::Value*> arguments;
    for (auto& arg : args) {
        arguments.push_back(var_to_value(arg));
    }
    return m_builder->CreateCall(llvm_func, arguments);
}
llvm::Value* llvm_gen::var_to_value(Variable var) {
    if (var.type.qualifiers & Qualifier::literal) {
        switch (var.type.info.kind) {
            case Kind::Int: {
                return llvm::ConstantInt::getSigned(typeid_to_type.at(var.type.info.id), var.Int_val);
            }break;
            case Kind::String: {
                return m_builder->CreateGlobalString(var.String_val, var.name);
            }break;
            default: TODO(mlog::format("llvm_gen::var_to_value(): add type literal {}", var.type.info.name));
        }
    }
    return m_builder->CreateLoad(typeid_to_type.at(var.type.info.id), allocated_vars.at(var.name));
}
void llvm_gen::compile_start_func() {
    TODO("doesn't work with libc");
    auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*m_ctx), {}, false);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "_start", m_mod.get());
    auto body = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(body);
    m_builder->CreateAlloca(m_builder->getInt8Ty(), llvm::ConstantInt::get(m_builder->getInt64Ty(), 0));

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
    if (mainfn->getReturnType()->isVoidTy())
        m_builder->CreateCall(exitfn, llvm::ConstantInt::getSigned(exitfn->arg_begin()->getType(), 0));
    else 
        m_builder->CreateCall(exitfn, {exit_code});

    m_builder->CreateUnreachable();
    llvm::verifyFunction(*fn);
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
        triple, "generic", "", llvm::TargetOptions(), std::optional<llvm::Reloc::Model>()
    );

    m_mod->setDataLayout(tm->createDataLayout());

    // --- emit object ---
    std::error_code ec;
    llvm::raw_fd_ostream out(output_path.string()+".o", ec);

    llvm::legacy::PassManager pm;
    tm->addPassesToEmitFile(pm, out, nullptr, llvm::CodeGenFileType::ObjectFile);
    pm.run(*m_mod);
    out.flush();
}
llvm::BasicBlock* llvm_gen::find_block(llvm::Function* fn, std::string name) {
    for (auto& bb : *fn) {
        if (bb.getName() == name)
            return &bb;
    }
    return nullptr;
}
