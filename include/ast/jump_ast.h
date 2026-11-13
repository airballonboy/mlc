#pragma once
#include "ast/statement_ast.h"
#include "codegen/base.h"
#include <string>

class Jump_Ast : public Statement_Ast {
public:
    Jump_Ast(std::string _label) : label(_label) {}

    std::string label = {};
public:
    Memory codegen(BaseCodegen& cg) override {
        cg.emitJump(this);
        return {};
    }
    Memory codegen_ptr(BaseCodegen& cg) override {TODO("not implemented");};
    static std::shared_ptr<Jump_Ast> make_node(std::string _label, Loc loc = {}) {
        auto x = std::make_shared<Jump_Ast>(_label);
        x->loc_start = loc;
        return x;
    }
};

