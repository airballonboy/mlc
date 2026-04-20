#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"

class Deref_Ast : public Expression_Ast {
public:
    Deref_Ast(Node _lhs)
        : lhs(std::move(_lhs))
    {
        is_lvalue = true;
    }

    Node lhs;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Deref_Ast> make_node(Node _lhs) {
        auto t = _lhs->type;
        auto x = std::make_unique<Deref_Ast>(std::move(_lhs));
        x->type = t;
        return x;
    }
};

