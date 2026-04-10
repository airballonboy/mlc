#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include <vector>

class Call_Ast : public Expression_Ast {
public:
    Call_Ast(Func& _func, std::vector<Node> _args) 
        : func(_func), args(std::move(_args)) {}

    Func& func;
    std::vector<Node> args;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::unique_ptr<Call_Ast> make_node(Func& _func, std::vector<Node> _args) {
        auto fn = new Func();
        fn->type = _func.type;
        fn->name = _func.name;
        auto x = std::make_unique<Call_Ast>(*fn, std::move(_args));
        x->type = *_func.type.func_data->return_type;
        return x;
    }
};

