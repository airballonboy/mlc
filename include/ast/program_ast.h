#pragma once
#include <ast/ast.h>
#include <ast/function_ast.h>

class Program_Ast : public AstNode {
public:
    Program_Ast() {}

    std::vector<FuncNode> func_storage;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("unimplemented");};
};
typedef std::unique_ptr<Program_Ast> ProgramNode;
