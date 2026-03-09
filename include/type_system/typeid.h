#pragma once
#include <cstddef>

namespace TypeId {
enum : size_t {
    Void,
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
    Typeid,
    Ptr,
    USize,
    String,
    Float,
    Double,
    Bool,
    BasicTypesCount
};
}
