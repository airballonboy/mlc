#include "ast/load_ast.h"
#include "codegen/gnu_asm_x86_64.h"


Memory Load_Ast::codegen(BaseCodegen& cg) {
    return cg.emitLoad(loc_start, var);
}
Memory Load_Ast::codegen_ptr(BaseCodegen& cg) {
    return cg.getVarPtr(loc_start, var);
}
