#include "codegen/llvm.h"
#include "codegen/base.h"
#include "context.h"
#include "operations.h"
#include "platform.h"
#include "program.h"
#include "tools/format.h"
#include "tools/logger.h"
#include "type_system/kind.h"
#include "type_system/type.h"
#include "type_system/type_info.h"
#include "type_system/typeid.h"
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
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <vector>

using std::unique_ptr;
using std::make_unique;

std::unordered_map<std::string, llvm::AllocaInst*> allocated_vars{};
std::unordered_map<std::string, std::string> func_aliases{};
std::unordered_map<std::string, llvm::Value*> temp_vars{};


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
        if (func.is_used || ctx.lib)
            compileFunction(func);
    }
    std::string mod_err;
    llvm::raw_string_ostream mod_err_stream(mod_err);
        m_mod->print(llvm::outs(), nullptr);
    if (llvm::verifyModule(*m_mod, &mod_err_stream)) {
        mlog::error("ERROR: ", "in module");
        mlog::println("----------------------------------------");
        mlog::println("----------------------------------------");
        mlog::println("Module verification failed: {}", mod_err);
        exit(1);
    }
    output_object_file();
}
void llvm_gen::compileFunction(Func func) {
    if (func.name == "main")
        int y;//asm("int3");
    if (func.name != func.link_name && func.external) {
        create_fn_wrapper(func);
    }
    if (func_aliases.contains(func.name))
        func.name = func_aliases.at(func.name);
    std::vector<llvm::Type*> args;
    for (auto arg : func.arguments) {
        if (arg.type.info.kind == Kind::Pointer)
            args.push_back(llvm::PointerType::get(*m_ctx, 0));
        else
            args.push_back(typeid_to_type.at(arg.type.info.id));
    }
    auto func_ret_type = *func.type.func_data->return_type;
    auto ft = llvm::FunctionType::get(typeid_to_type.at(func_ret_type.info.id), args, func.c_variadic);
    auto fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func.name, m_mod.get());
    if (func.external) return;
    auto func_entry = llvm::BasicBlock::Create(*m_ctx, "entry", fn);
    m_builder->SetInsertPoint(func_entry);
    for (auto arg : func.arguments) {
        if (arg.type.info.kind == Kind::Pointer) {
            llvm::AllocaInst* var = m_builder->CreateAlloca(llvm::PointerType::get(*m_ctx, 0), nullptr, arg.name);
            allocated_vars.emplace(arg.name, var);
        } else {
            llvm::AllocaInst* var = m_builder->CreateAlloca(typeid_to_type.at(arg.type.info.id), nullptr, arg.name);
            allocated_vars.emplace(arg.name, var);
        }
    }

    bool returned = false;
    for (auto& inst : func.body) {
        returned = false;
        switch (inst.op) {
            case Op::RETURN: {
                Variable arg = std::get<Variable>(inst.args[0]);

                auto ret = var_to_value(arg);
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
                auto t = var2.type;
                if (var2.parent != nullptr) {
                    auto par = var2;
                    //std::vector<size_t> iters{};
                    while (par.parent != nullptr) {
                        auto strct = Struct::get_from_name(get_base_type(par.parent->type).info.name, m_program->struct_storage);
                        //iters.push_back(strct.get_element_iterator(par.name));
                        par = *par.parent;
                    }
                    if (!allocated_vars.contains(par.name)) {
                        llvm::AllocaInst* var = m_builder->CreateAlloca(typeid_to_type.at(par.type.info.id), nullptr, par.name);
                        allocated_vars.emplace(par.name, var);
                    }
                    //auto llvm_par = allocated_vars[par.name];
                    //llvm::Value* elem = llvm_par;
                    //while (iters.size() > 0) {
                    //    auto iter = iters.back();
                    //    iters.pop_back();
                    //    if (elem->getType()->isPointerTy()) {
                    //        elem = m_builder->CreateLoad(
                    //            typeid_to_type.at(get_base_type(par.members[iter].type).info.id),
                    //            elem
                    //        );
                    //    }

                    //    elem = m_builder->CreateStructGEP(typeid_to_type.at(par.type.info.id), elem, iter);
                    //    par = par.members[iter];
                    //}
                    //m_builder->CreateStore(var_to_value(var1, t), elem);
                } else {
                    if (!allocated_vars.contains(var2.name)) {
                        llvm::AllocaInst* var = m_builder->CreateAlloca(typeid_to_type.at(var2.type.info.id), nullptr, var2.name);
                        allocated_vars.emplace(var2.name, var);
                    }
                    //auto llvm_var2 = allocated_vars[var2.name];
                    //m_builder->CreateStore(var_to_value(var1, t), llvm_var2);
                }
                m_builder->CreateStore(var_to_value(var1, var2.type), var_to_value(var2));
            }break;
            case Op::INIT_STRING: {
                Variable str = std::get<Variable>(inst.args[0]);
            }break;
            case Op::CALL: {
                Func func             = std::get<Func>(inst.args[0]);
                VariableStorage args  = std::get<VariableStorage>(inst.args[1]);
                Variable ret_address  = std::get<Variable>(inst.args[2]);

                auto val = call_func(func, args);
                if (ret_address.name.starts_with("%%temp")) {
                    temp_vars.emplace(ret_address.name, val);
                }
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
            case Op::BIN_OP: {
                auto op     = std::get<BinOp>(inst.args[0]);
                auto lhs    = std::get<Variable>(inst.args[1]);
                auto rhs    = std::get<Variable>(inst.args[2]);
                auto result = std::get<Variable>(inst.args[3]);

                llvm::Value* result_val;
                bool is_float = lhs.type.info.kind == Kind::Float || rhs.type.info.kind == Kind::Float;
                auto t_id = std::max(lhs.type.info.id, rhs.type.info.id);
                Type t = TypeInfo(t_id);

                switch (op) {
                    case BinOp::ADD:
                        if (is_float)
                            result_val = m_builder->CreateFAdd(var_to_value(lhs, t), var_to_value(rhs, t));
                        else
                            result_val = m_builder->CreateAdd(var_to_value(lhs, t), var_to_value(rhs, t));
                        break;
                    case BinOp::SUB:
                        if (is_float)
                            result_val = m_builder->CreateFSub(var_to_value(lhs, t), var_to_value(rhs, t));
                        else
                            result_val = m_builder->CreateSub(var_to_value(lhs, t), var_to_value(rhs, t));
                        break;
                    case BinOp::MUL:
                        if (is_float)
                            result_val = m_builder->CreateFMul(var_to_value(lhs, t), var_to_value(rhs, t));
                        else
                            result_val = m_builder->CreateMul(var_to_value(lhs, t), var_to_value(rhs, t));
                        break;
                    case BinOp::DIV:
                        if (is_float)
                            result_val = m_builder->CreateFDiv(var_to_value(lhs, t), var_to_value(rhs, t));
                        else
                            result_val = m_builder->CreateExactSDiv(var_to_value(lhs, t), var_to_value(rhs, t));
                        break;
                    case BinOp::EQ:
                        if (is_float)
                            result_val = m_builder->CreateFCmpOEQ(var_to_value(lhs, t), var_to_value(rhs, t));
                        else
                            result_val = m_builder->CreateICmpEQ(var_to_value(lhs, t), var_to_value(rhs, t));
                        break;
                    case BinOp::MOD:
                    case BinOp::NE:
                    case BinOp::LT:
                    case BinOp::LE:
                    case BinOp::GT:
                    case BinOp::GE:
                    case BinOp::LAND:
                    case BinOp::LOR:
                        TODO("not implemented");
                        break;
                    default:
                        assert(false && "unreachable");
                }        
                if (result.name.starts_with("%%temp")) {
                    temp_vars.emplace(result.name, result_val);
                }
            }break;
        }
    }
    if (!returned) {
        if (func.type.func_data->return_type->info.kind == Kind::Void) {
            m_builder->CreateRetVoid();
        } else if (func.type.func_data->return_type->info.kind == Kind::Int) {
            auto ret = llvm::ConstantInt::get(*m_ctx, llvm::APInt(fn->getReturnType()->getIntegerBitWidth(), 0));
            m_builder->CreateRet(ret);
        } else {
            TODO("unkown default return");
        }
    }
    allocated_vars.clear();
    temp_vars.clear();
    fn->print(llvm::outs());
    llvm::verifyFunction(*fn);
}
llvm::Value* llvm_gen::call_func(Func func, VariableStorage args) {
    if (func_aliases.contains(func.name))
        func.name = func_aliases.at(func.name);
    auto llvm_func = m_mod->getFunction(func.name);
    std::vector<llvm::Value*> arguments;
    for (auto& arg : args) {
        if (func.c_variadic && arg.type.info.kind == Kind::Float)
            arguments.push_back(var_to_value(arg, TypeInfo(TypeId::Double)));
        else
            arguments.push_back(var_to_value(arg));
    }
    return m_builder->CreateCall(llvm_func, arguments);
}
llvm::Value* llvm_gen::var_to_value(Variable var, Type t) {
    if (t.info.id == TypeId::Void)
        t = var.type.info;
    if (var.type.qualifiers & Qualifier::literal) {
        switch (var.type.info.kind) {
            case Kind::Int: {
                return llvm::ConstantInt::getSigned(typeid_to_type.at(t.info.id), var.Int_val);
            }break;
            case Kind::String: {
                return m_builder->CreateGlobalString(mlog::unescape(var.String_val), var.name);
            }break;
            case Kind::Float: {
                return llvm::ConstantFP::get(typeid_to_type.at(t.info.id), var.Double_val);
            }break;
            default: TODO(mlog::format("llvm_gen::var_to_value(): add type literal {}", var.type.info.name));
        }
    }
    if (var.name.starts_with("%%temp")) {
        if (!temp_vars.contains(var.name)) {
            TODO("unknown temp var");
        }
        return temp_vars.at(var.name);
    }
    if (var.deref_count < 0) {
        return allocated_vars.at(var.name);
    }
    if (var.parent != nullptr) {
        auto par = var;
        std::vector<size_t> iters{};
        while (par.parent != nullptr) {
            auto strct = Struct::get_from_name(get_base_type(par.parent->type).info.name, m_program->struct_storage);
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
            if (elem->getType()->isPointerTy()) {
                elem = m_builder->CreateLoad(
                    typeid_to_type.at(get_base_type(par.members[iter].type).info.id),
                    elem
                );
            }

            elem = m_builder->CreateStructGEP(typeid_to_type.at(par.type.info.id), elem, iter);
            par = par.members[iter];
        }
        return elem;
    }
    auto loaded_val = m_builder->CreateLoad(typeid_to_type.at(var.type.info.id), allocated_vars.at(var.name));
    if (t.info.id != var.type.info.id) {
        if (var.type.info.kind == Kind::Int && t.info.kind == Kind::Int) {
            if (t.info.size > var.type.info.size) {
                return m_builder->CreateSExt(loaded_val, typeid_to_type.at(t.info.id));
            } else {
                return m_builder->CreateTrunc(loaded_val, typeid_to_type.at(t.info.id));
            }
        } else if (var.type.info.kind == Kind::Float && t.info.kind == Kind::Float) {
            if (t.info.size > var.type.info.size) {
                return m_builder->CreateFPExt(loaded_val, typeid_to_type.at(t.info.id));
            } else {
                return m_builder->CreateFPTrunc(loaded_val, typeid_to_type.at(t.info.id));
            }
        } else if (var.type.info.kind == Kind::Int && t.info.kind == Kind::Float) {
            return m_builder->CreateSIToFP(loaded_val, typeid_to_type.at(t.info.id));
        } else {
            TODO(mlog::format("unkown cast ({} to {})", var.type.info.id, t.info.id));
        }
    } else {
        return loaded_val;
    }
    return nullptr;
}
void llvm_gen::create_fn_wrapper(Func fn) {
    if (fn.name != fn.link_name) {
        func_aliases.emplace(fn.name, fn.link_name);
    }
    return;
    // Create external fn
    Func ex = fn;
    if (fn.link_name != fn.name) {
        ex.name = fn.link_name;
        ex.link_name = ex.name;
        compileFunction(ex);
    }
    std::vector<llvm::Type*> args;
    for (auto arg : fn.arguments) {
        args.push_back(typeid_to_type.at(arg.type.info.id));
    }
    auto fn_ret_type = *fn.type.func_data->return_type;
    auto ft = llvm::FunctionType::get(typeid_to_type.at(fn_ret_type.info.id), args, fn.c_variadic);
    auto llvm_fn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, fn.name, m_mod.get());
    auto fn_entry = llvm::BasicBlock::Create(*m_ctx, "entry", llvm_fn);
    m_builder->SetInsertPoint(fn_entry);
    auto ret = call_func(ex, fn.arguments);
    if (fn.type.func_data->return_type->info.id == TypeId::Void)
        m_builder->CreateRetVoid();
    else
        m_builder->CreateRet(ret);
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
