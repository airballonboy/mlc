#include "ast/body_ast.h"


Memory Body_Ast::codegen(BaseCodegen& cg) {
    for (auto& node : body) {
        node->codegen(cg);
    }
    return {};
}
