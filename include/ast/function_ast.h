#pragma once
#include "ast/ast.h"
#include "ast/body_ast.h"
#include "type_system/func.h"

class Func_Ast : public AstNode {
public:
    Func_Ast(Func& _func, BodyNode _body) 
        : func(_func), body(_body) {}

    Func& func;
    BodyNode body;
public:
    Memory codegen(BaseCodegen& cg) override;
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemented");}
    static std::shared_ptr<Func_Ast> make_node(Func& _func, BodyNode _body, Loc loc = {}) {
        auto x = std::make_shared<Func_Ast>(_func, _body);
        x->loc_start = loc;
        return x;
    }
};
typedef std::shared_ptr<Func_Ast> FuncNode;
