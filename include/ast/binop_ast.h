#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include "operations.h"

class BinOp_Ast : public Expression_Ast {
public:
    BinOp_Ast(Node _lhs, Node _rhs, BinOp _binop) 
        : lhs(std::move(_lhs)), rhs(std::move(_rhs)), binop(_binop) {}

    Node lhs;
    Node rhs;
    BinOp binop;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<BinOp_Ast> make_node(Node _lhs, Node _rhs, BinOp _binop) {
        auto x = std::make_unique<BinOp_Ast>(std::move(_lhs), std::move(_rhs), _binop);
        x->type = _lhs->type;
        return x;
    }
};
