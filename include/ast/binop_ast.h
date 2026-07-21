#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include "operations.h"

class BinOp_Ast : public Expression_Ast {
public:
    BinOp_Ast(Node _lhs, Node _rhs, BinOp _binop) 
        : lhs(_lhs), rhs(_rhs), binop(_binop) {}

    Node lhs;
    Node rhs;
    BinOp binop;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitBinOp(this);
    }
    static std::shared_ptr<BinOp_Ast> make_node(Node _lhs, Node _rhs, BinOp _binop, Loc loc = {}) {
        auto x = std::make_shared<BinOp_Ast>(_lhs, _rhs, _binop);
        bool is_bool = (_binop >= BinOp::LT && _binop <= BinOp::NE) ? true : false;
        x->type =  is_bool ? type_infos.at("bool") : _lhs->type;
        x->loc_start = loc;
        return x;
    }
};
