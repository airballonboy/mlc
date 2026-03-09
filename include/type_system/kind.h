#pragma once
#include <cstdint>
namespace Qualifier {
enum : uint32_t {
    none     = 0,
    constant = 1 << 0,
    global   = 1 << 1,
    literal  = 1 << 2,
};
}

enum class Kind : int {
    Void = 0, 
    Int,
    Float,
    String,
    Struct,
    Pointer,
    Func
};
