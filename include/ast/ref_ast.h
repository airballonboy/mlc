#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"

class Ref_Ast : public Expression_Ast {
public:
    Ref_Ast(Variable _lhs) 
        : lhs(_lhs) {}

    Variable lhs;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitRef(this);
    }
    static std::unique_ptr<Ref_Ast> make_node(Variable _lhs, Loc loc = {}) {
        auto x = std::make_unique<Ref_Ast>(_lhs);
        x->type = make_ptr(_lhs.type);
        x->loc_start = loc;
        return x;
    }
};

