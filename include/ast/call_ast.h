#pragma once
#include "ast/expression_ast.h"
#include "codegen/base.h"
#include <vector>

class Call_Ast : public Expression_Ast {
public:
    Call_Ast(Func& _func, std::vector<Node> _args, Variable ret = {.type = type_infos.at("void")}) 
        : func(_func), args(_args), ret_addr(ret) {}

    Func& func;
    std::vector<Node> args;
    Variable ret_addr;
public:
    Memory codegen(BaseCodegen& cg) override {
        return cg.emitCall(this);
    }
    static std::shared_ptr<Call_Ast> make_node(Func& _func, std::vector<Node> _args, Variable ret = {.type = type_infos.at("void")}, Loc loc = {}) {
        auto x = std::make_shared<Call_Ast>(_func, _args);
        x->type = *_func.type.func_data->return_type;
        x->loc_start = loc;
        x->ret_addr = ret;
        return x;
    }
};

