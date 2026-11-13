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
    static std::shared_ptr<If_Ast> make_node(ExprNode cond, Node then_block, Node else_block, Loc loc = {}) {
        auto x = std::make_shared<If_Ast>();
        x->cond       = cond;
        x->then_block = then_block;
        x->else_block = else_block;
        return x;
    }
    Memory codegen(BaseCodegen& cg) override {
        cg.emitIf(this);
        return {};
    }
};
