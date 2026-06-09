#pragma once
#include "ast/expression_ast.h"
#include "ast/statement_ast.h"
#include "codegen/base.h"

class If_Ast : public Statement_Ast {
public:
    If_Ast(){}

    ExprNode cond   = {};
    Node then_block = {};
    Node else_block = {};

public:
    static std::unique_ptr<If_Ast> make_node(ExprNode cond, Node then_block, Node else_block, Loc loc = {}) {
        auto x = std::make_unique<If_Ast>();
        x->cond       = std::move(cond);
        x->then_block = std::move(then_block);
        x->else_block = std::move(else_block);
        return x;
    }
    Memory codegen(BaseCodegen& cg) override {
        cg.emitIf(this);
        return {};
    }
};
