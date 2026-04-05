#pragma once
#include "ast/statment_ast.h"

class Body_Ast : public Statment_Ast {
public:
    Body_Ast(){}

    std::vector<Node> body = {};
public:
    Memory codegen(BaseCodegen& cg) override;
};
typedef std::unique_ptr<Body_Ast> BodyNode;
