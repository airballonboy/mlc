#pragma once
#include "codegen/base.h"
#include "ast/ast.h"

class Label_Ast : public Statement_Ast {
public:
    Label_Ast(std::string _label) 
        : label(_label) {}

    std::string label;
public:
    Memory codegen(BaseCodegen& cg) override;
    static std::shared_ptr<Label_Ast> make_node(std::string _label, Loc loc = {}) {
        auto x = std::make_shared<Label_Ast>(_label);
        x->loc_start = loc;
        return x;
    }
};

