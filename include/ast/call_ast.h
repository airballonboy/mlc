#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include <vector>

class Call_Ast : public Expression_Ast {
public:
    Call_Ast(Func& _func, std::vector<Node> _args, Variable ret = {.type = type_infos.at("void")}) 
        : func(_func), args(std::move(_args)), ret_addr(ret) {}

    Func& func;
    std::vector<Node> args;
    Variable ret_addr;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitCall(this);
    }
    static std::unique_ptr<Call_Ast> make_node(Func& _func, std::vector<Node> _args, Variable ret = {.type = type_infos.at("void")}, Loc loc = {}) {
        auto fn = new Func();
        fn->type = _func.type;
        fn->name = _func.name;
        fn->c_variadic = _func.c_variadic;
        fn->variadic = _func.variadic;
        fn->is_member = _func.is_member;
        fn->is_static = _func.is_static;
        auto x = std::make_unique<Call_Ast>(*fn, std::move(_args));
        x->type = *_func.type.func_data->return_type;
        x->loc_start = loc;
        x->ret_addr = ret;
        return x;
    }
};

