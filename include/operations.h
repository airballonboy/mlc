#pragma once

enum class BinOp {
    // Arithmetic
    ADD, SUB, MUL, DIV, MOD,
    // Comparison
    LT, LE, GT, GE, EQ, NE,
    // Logical AND/OR
    LAND, LOR,
};
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
    // BIN_OP(bin_op, lhs, rhs, result)
    BIN_OP,
    // LABEL(label)
    LABEL,
    // JUMP(label)
    JUMP,
    // JUMP_IF_NOT(label, variable)
    JUMP_IF_NOT,
};
