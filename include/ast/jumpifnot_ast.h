#pragma once
#include "codegen/base.h"
#include "ast/ast.h"
#include <memory>
#include <string>

class JumpIfNot_Ast : public Statement_Ast {
public:
    JumpIfNot_Ast(std::string _label, Node _cond) 
        : label(_label), condition(std::move(_cond)) {}

    std::string label;
    Node condition;
public:
    Memory codegen(BaseCodegen& cg) override {
        TODO("");
    }
    static std::unique_ptr<JumpIfNot_Ast> make_node(std::string _label, Node _cond, Loc loc = {}) {
        auto x = std::make_unique<JumpIfNot_Ast>(_label, std::move(_cond));
        x->loc_start = loc;
        return x;
    }
};

