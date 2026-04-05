#pragma once
#include <memory>
#include "ast/mem/memory.h"
#include "token.h"

class BaseCodegen;
class AstNode;
typedef std::unique_ptr<AstNode> Node;

class AstNode {
public:
    Loc loc_start;
    Loc loc_end;
    virtual ~AstNode() = default;
    virtual Memory codegen(BaseCodegen& cg) = 0;
    virtual Memory codegen_ptr(BaseCodegen& cg) = 0;
};
