#include "ast/binop_ast.h"
#include "ast/ast.h"
#include "codegen/base.h"
#include <memory>

Memory BinOp_Ast::codegen(BaseCodegen& cg) {
    auto lhs_mem = lhs->codegen(cg);
    auto rhs_mem = rhs->codegen(cg);
    return cg.emitBinOp(loc_start, binop, lhs_mem, rhs_mem);
}
