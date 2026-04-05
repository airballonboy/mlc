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
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Load_Ast> make_node(Variable _var) {
        return std::make_unique<Load_Ast>(_var);
    }
};

