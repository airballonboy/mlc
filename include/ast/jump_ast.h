#pragma once
#include "codegen/base.h"
#include "ast/ast.h"
#include <string>

class Jump_Ast : public Statment_Ast {
public:
    Jump_Ast(std::string _label) 
        : label(_label) {}

    std::string label;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemented");};
    static std::unique_ptr<Jump_Ast> make_node(std::string _label) {
        return std::make_unique<Jump_Ast>(_label);
    }
};

