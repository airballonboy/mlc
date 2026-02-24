#pragma once 
#include "codegen/base.h"
#include "type_system/type_info.h"
#include "type_system/variable.h"
#include "type_system/func.h"
#include "program.h"
#include <memory>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

class llvm_gen : public BaseCodegenerator {
public:
    llvm_gen(Program* prog);

    void call_func(Func func, VariableStorage args);
    void compileProgram() override;
    void compileFunction(Func func) override;

private:
    std::unique_ptr<llvm::LLVMContext> m_ctx;
    std::unique_ptr<llvm::Module>      m_mod;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    std::unordered_map<size_t, llvm::Type*> typeid_to_type;
};

