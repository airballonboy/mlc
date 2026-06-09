#pragma once
#include "ast/statement_ast.h"

class Body_Ast : public Statement_Ast {
public:
    Body_Ast(){}

    std::vector<Node> body = {};
public:
    Memory codegen(BaseCodegen& cg) override {
        for (auto& node : body) {
            node->codegen(cg);
        }
        return {};
    }
};
typedef std::unique_ptr<Body_Ast> BodyNode;
