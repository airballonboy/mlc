#pragma once
#include "codegen/base.h"
#include "ast/ast.h"
#include <memory>
#include <string>

class JumpIfNot_Ast : public AstNode {
public:
    JumpIfNot_Ast(std::string _label, Node _cond) 
        : label(_label), condition(std::move(_cond)) {}

    std::string label;
    Node condition;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemeneted");};
    static std::unique_ptr<JumpIfNot_Ast> make_node(std::string _label, Node _cond) {
        return std::make_unique<JumpIfNot_Ast>(_label, std::move(_cond));
    }
};

