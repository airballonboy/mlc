#include "ast/call_ast.h"


Memory Call_Ast::codegen(BaseCodegen& cg) {
    return cg.emitCall(loc_start, func, std::move(args));
}
