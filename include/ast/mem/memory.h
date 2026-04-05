#pragma once
#include "ast/mem/asm.h"
#include "type_system/type.h"

enum class MemType {
    None,
    Asm,
};
class Memory {
public:
    MemType mem_type;
    Type type;
    union {
        AsmMemory asm_mem;
    };
};
