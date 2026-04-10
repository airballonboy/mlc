#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"

class Ref_Ast : public Expression_Ast {
public:
    Ref_Ast(Variable _lhs) 
        : lhs(_lhs) {}

    Variable lhs;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Ref_Ast> make_node(Variable _lhs) {
        auto x = std::make_unique<Ref_Ast>(_lhs);
        x->type = _lhs.type;
        return x;
    }
};

