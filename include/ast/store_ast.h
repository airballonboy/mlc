#pragma once
#include "ast/expression_ast.h"
#include "ast/load_ast.h"
#include "codegen/base.h"

class Store_Ast : public Expression_Ast {
public:
    Store_Ast(Node _lhs, Node _rhs)
        : lhs(_lhs), rhs(_rhs) {}

    Node lhs;
    Node rhs;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitStore(this);
    }
    static std::shared_ptr<Store_Ast> make_node(Variable _lhs, ExprNode _rhs, Loc loc = {}) {
        auto x = std::make_shared<Store_Ast>(Load_Ast::make_node(_lhs), _rhs);
        x->type = _lhs.type;
        x->loc_start = loc;
        return x;
    }
    static std::shared_ptr<Store_Ast> make_node(Node _lhs, Node _rhs, Loc loc = {}) {
        auto t = _lhs->type;
        auto x = std::make_shared<Store_Ast>(_lhs, _rhs);
        x->loc_start = loc;
        x->type = t;
        return x;
    }
};

