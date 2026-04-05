#pragma once
#include "ast/ast.h"

class Statment_Ast : public AstNode { 
    Memory codegen_ptr(BaseCodegen& cg) override { TODO("function wasn't made");};
};
typedef std::unique_ptr<Statment_Ast> StmtNode;
