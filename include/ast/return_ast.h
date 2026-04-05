#pragma once
#include "ast/statment_ast.h"
#include "codegen/base.h"

class Return_Ast : public Statment_Ast {
public:
    Return_Ast(Node _ret) 
        : ret(std::move(_ret)) {}

    Node ret;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Return_Ast> make_node(Node _ret) {
        return std::make_unique<Return_Ast>(std::move(_ret));
    }
};

