#pragma once
#include "codegen/base.h"
#include "ast/ast.h"

class Label_Ast : public AstNode {
public:
    Label_Ast(std::string _label) 
        : label(_label) {}

    std::string label;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemented");};
    static std::unique_ptr<Label_Ast> make_node(std::string _label) {
        return std::make_unique<Label_Ast>(_label);
    }
};

