#pragma once
#include "codegen/asm_instruction.h"

enum class AsmType {
    None,
    Reg,
    OffReg,
    Global,
    ArrayIndex
};
class AsmMemory {
public:
    AsmType type;
    union {
        struct {
            Register reg;
        };
        struct {
            int64_t  off;
            Register off_reg;
        };
        struct {
            char*    label;
            Register label_reg;
        };
        struct {
            Register reg1;
            Register reg2;
        };
        struct {
            int64_t  disp;
            Register base;
            Register index;
            size_t   scale;
        };
    };
};
