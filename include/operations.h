#pragma once

enum class Op {
    // stores variable1 into variable2
    // STORE_VAR(variable1, variable2)
    STORE_VAR,
    // INIT_STRING(string)
    INIT_STRING,
    // RETURN(variable)
    RETURN,
    // CALL(func, args, return_address)
    CALL,
    // (lhs, rhs, result)
    // Binary operations
    ADD, SUB, MUL, DIV, MOD,
    // Comparison
    LT, LE, GT, GE, EQ, NE,
    // Logical AND/OR
    LAND, LOR,
    // LABEL(label)
    LABEL,
    // JUMP(label)
    JUMP,
    // JUMP_IF_NOT(label, variable)
    JUMP_IF_NOT,
};
