#pragma once
#include "ast/statement_ast.h"

class Expression_Ast : public Statement_Ast {
public:
    bool is_lvalue = false;
};
typedef std::shared_ptr<Expression_Ast> ExprNode;
