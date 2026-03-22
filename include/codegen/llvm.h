#pragma once 
#include "codegen/base.h"
#include "type_system/typeid.h"
#include "type_system/variable.h"
#include "type_system/func.h"
#include "program.h"
#include "llvm/IR/Value.h"
#include <memory>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

class llvm_gen : public BaseCodegenerator {
public:
    llvm_gen(Program* prog);

    llvm::Value* call_func(Func func, VariableStorage args);
    void compileProgram() override;
    void compileFunction(Func func) override;

private:
    llvm::Value* var_to_value(Variable var, Type type_id = TypeInfo(TypeId::Void));
    void compile_start_func(); 
    void output_object_file(); 
    void create_fn_wrapper(Func f);
    llvm::BasicBlock* find_block(llvm::Function* fn, std::string name);


private:
    std::unique_ptr<llvm::LLVMContext> m_ctx;
    std::unique_ptr<llvm::Module>      m_mod;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    std::unordered_map<size_t, llvm::Type*> typeid_to_type;
};

