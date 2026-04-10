#include "ast/store_ast.h"


Memory Store_Ast::codegen(BaseCodegen& cg) {
    return cg.emitStore(loc_start, lhs->codegen_ptr(cg), rhs->codegen(cg));
}
