#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include "ast/ast.h"
#include "type_system/variable.h"

class Load_Ast : public Expression_Ast {
public:
    Load_Ast(Variable _var) 
        : var(_var) {}

    Variable var;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitLoad(this);
    }
    Memory codegen_ptr(BaseCodegen& cg) override {
        return cg.getVarPtr(var);
    }
    static std::unique_ptr<Load_Ast> make_node(Variable _var) {
        auto x = std::make_unique<Load_Ast>(_var);
        x->type = _var.type;
        return x;
    }
};

