#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"

class Store_Ast : public Expression_Ast {
public:
    Store_Ast(Node _lhs, Node _rhs)
        : lhs(std::move(_lhs)), rhs(std::move(_rhs)) {}

    Node lhs;
    Node rhs;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Store_Ast> make_node(Node _lhs, Node _rhs) {
        return std::make_unique<Store_Ast>(std::move(_lhs), std::move(_rhs));
    }
};

