#pragma once
#include "ast/ast.h"
#include "ast/body_ast.h"
#include "type_system/func.h"

class Func_Ast : public AstNode {
public:
    Func_Ast(Func& _func, BodyNode _body) 
        : func(_func), body(std::move(_body)) {}

    Func& func;
    BodyNode body;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemented");}
    static std::unique_ptr<Func_Ast> make_node(Func& _func, BodyNode _body) {
        return std::make_unique<Func_Ast>(_func, std::move(_body));
    }
};
typedef std::unique_ptr<Func_Ast> FuncNode;
