#pragma once
#include "ast/ast.h"

class Statement_Ast : public AstNode { 
public:
    //Memory codegen(BaseCodegen& cg) override { TODO("function wasn't made"); }
    Memory codegen_ptr(BaseCodegen& cg) override { TODO("function wasn't made"); }
};
typedef std::unique_ptr<Statement_Ast> StmtNode;

class Empty_Ast : public Statement_Ast {
public:
    Memory codegen(BaseCodegen& cg) override {
        return {};
    }
    static std::unique_ptr<Empty_Ast> make_node(Loc loc = {}) {
        return std::make_unique<Empty_Ast>();
    }
    Memory codegen_ptr(BaseCodegen& cg) override {
        TODO("function wasn't made"); 
    }
};
