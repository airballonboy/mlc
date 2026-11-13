#pragma once
#include "ast/statement_ast.h"
#include "codegen/base.h"

class Return_Ast : public Statement_Ast {
public:
    Return_Ast(Node _ret) 
        : ret(_ret) {}

    Node ret;
public:
    Memory codegen(BaseCodegen& cg) override {
        cg.emitReturn(this);
        return {};
    }
    static std::shared_ptr<Return_Ast> make_node(Node _ret, Loc loc = {}) {
        auto x = std::make_shared<Return_Ast>(_ret);
        x->loc_start = loc;
        return x;
    }
};

