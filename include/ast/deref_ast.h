#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"

class Deref_Ast : public Expression_Ast {
public:
    Deref_Ast(Node _lhs)
        : lhs(_lhs)
    {
        is_lvalue = true;
    }

    Node lhs;
public:
    Memory codegen(BaseCodegen& cg) override {
        TODO("");
    }
    static std::shared_ptr<Deref_Ast> make_node(Node _lhs, Loc loc = {}) {
        auto t = _lhs->type;
        auto x = std::make_shared<Deref_Ast>(_lhs);
        x->type = t;
        x->loc_start = loc;
        return x;
    }
};

