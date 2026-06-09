#pragma once
#include "ast/ast.h"
#include "ast/binop_ast.h"
#include "ast/deref_ast.h"
#include "ast/ref_ast.h"
#include "ast/function_ast.h"
#include "ast/jump_ast.h"
#include "ast/label_ast.h"
#include "ast/jumpifnot_ast.h"
#include "ast/load_ast.h"
#include "ast/program_ast.h"
#include "ast/return_ast.h"
#include "ast/if_ast.h"
#include "ast/call_ast.h"
#include "ast/store_ast.h"
#include <functional>


template<typename Arg_t, typename Func_t>
void walk_nodes(AstNode* node, Func_t&& func, Arg_t& data) {
    std::invoke(std::forward<Func_t>(func), node, data);
    
    if (auto nd = dynamic_cast<Return_Ast*>(node)) {
        walk_nodes(nd->ret.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<If_Ast*>(node)) {
        walk_nodes(nd->cond.get(), std::forward<Func_t>(func), data);
        walk_nodes(nd->then_block.get(), std::forward<Func_t>(func), data);
        if (nd->else_block) walk_nodes(nd->else_block.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<Body_Ast*>(node)) {
        for (auto& n : nd->body) {
            walk_nodes(n.get(), std::forward<Func_t>(func), data);
        }
    } else if (auto nd = dynamic_cast<BinOp_Ast*>(node)) {
        walk_nodes(nd->lhs.get(), std::forward<Func_t>(func), data);
        walk_nodes(nd->rhs.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<Call_Ast*>(node)) {
        for (auto& n : nd->args) {
            walk_nodes(n.get(), std::forward<Func_t>(func), data);
        }
    } else if (auto nd = dynamic_cast<Deref_Ast*>(node)) {
        walk_nodes(nd->lhs.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<JumpIfNot_Ast*>(node)) {
        walk_nodes(nd->condition.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<Store_Ast*>(node)) {
        walk_nodes(nd->lhs.get(), std::forward<Func_t>(func), data);
        walk_nodes(nd->rhs.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<Statement_Ast*>(node)) {
    } else if (auto nd = dynamic_cast<Ref_Ast*>(node)) {
    } else if (auto nd = dynamic_cast<Program_Ast*>(node)) {
    } else if (auto nd = dynamic_cast<Load_Ast*>(node)) {
    // TODO: doesn't work
    //} else if (auto nd = dynamic_cast<Jump_Ast*>(node)) {
    //} else if (auto nd = dynamic_cast<Label_Ast*>(node)) {
    //} else if (auto nd = dynamic_cast<Func_Ast*>(node)) {
    //    walk_nodes(nd->body.get(), std::forward<Func_t>(func), data);
    } else if (auto nd = dynamic_cast<Expression_Ast*>(node)) {
    } else {
        TODO("unkown node type");
    }
}
