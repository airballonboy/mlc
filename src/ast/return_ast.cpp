#include "ast/return_ast.h"

Memory Return_Ast::codegen(BaseCodegen& cg) {
    auto ret_mem = ret->codegen(cg);
    cg.emitReturn({}, ret_mem);
    return {};
}
