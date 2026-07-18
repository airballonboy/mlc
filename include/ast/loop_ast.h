#pragma once
#include "ast/expression_ast.h"
#include "ast/statement_ast.h"
#include "codegen/base.h"

class Loop_Ast : public Statement_Ast {
public:
    Loop_Ast(){}

    ExprNode cond   = {};
    Node do_block = {};

public:
    static std::unique_ptr<Loop_Ast> make_node(ExprNode cond, Node do_block, Loc loc = {}) {
        auto x = std::make_unique<Loop_Ast>();
        x->cond     = std::move(cond);
        x->do_block = std::move(do_block);
        return x;
    }
    Memory codegen(BaseCodegen& cg) override {
        cg.emitLoop(this);
        return {};
    }
};
