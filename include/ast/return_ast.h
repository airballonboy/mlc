#pragma once
#include "ast/statement_ast.h"
#include "codegen/base.h"

class Return_Ast : public Statement_Ast {
public:
    Return_Ast(Node _ret) 
        : ret(std::move(_ret)) {}

    Node ret;
public:
    Memory codegen(BaseCodegen& cg) override {
        cg.emitReturn(this);
        return {};
    }
    static std::unique_ptr<Return_Ast> make_node(Node _ret, Loc loc = {}) {
        auto x = std::make_unique<Return_Ast>(std::move(_ret));
        x->loc_start = loc;
        return x;
    }
};

