#pragma once
#include "ast/statment_ast.h"

class Expression_Ast : public Statment_Ast {
public:
    bool is_lvalue = false;
};
