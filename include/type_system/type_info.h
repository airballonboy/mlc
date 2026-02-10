#pragma once
#include <string>
#include "type_system/type.h"

struct TypeInfo {
    size_t id = 0;
    Type type = Type::Int32_t;
    size_t size = 0;
    std::string name{};
    // TODO: add available cast functions to other types
};

inline size_t current_typeid_max = 0;
inline std::unordered_map <std::string, TypeInfo> type_infos = {
    {"void"   , {.id = current_typeid_max++ , .type = Type::Void_t  , .size = 1, .name = "void"}},
    {"char"   , {.id = current_typeid_max++ , .type = Type::Char_t  , .size = 1, .name = "char"}},
    {"int8"   , {.id = current_typeid_max++ , .type = Type::Int8_t  , .size = 1, .name = "int8"}},
    {"int16"  , {.id = current_typeid_max++ , .type = Type::Int16_t , .size = 2, .name = "int16"}},
    {"int"    , {.id = current_typeid_max   , .type = Type::Int32_t , .size = 4, .name = "int32"}},
    {"int32"  , {.id = current_typeid_max++ , .type = Type::Int32_t , .size = 4, .name = "int32"}},
    {"int64"  , {.id = current_typeid_max++ , .type = Type::Int64_t , .size = 8, .name = "int64"}},
    {"typeid" , {.id = current_typeid_max++ , .type = Type::Typeid_t, .size = 8, .name = "typeid"}},
    {"pointer", {.id = current_typeid_max++ , .type = Type::Ptr_t   , .size = 8, .name = "pointer"}},
    {"usize"  , {.id = current_typeid_max++ , .type = Type::Size_t  , .size = 8, .name = "uint64"}},
    {"string" , {.id = current_typeid_max++ , .type = Type::String_t, .size = 8, .name = "string"}},
    {"float"  , {.id = current_typeid_max++ , .type = Type::Float_t , .size = 4, .name = "float"}},
    {"double" , {.id = current_typeid_max++ , .type = Type::Double_t, .size = 8, .name = "double"}},
    {"bool"   , {.id = current_typeid_max++ , .type = Type::Bool_t  , .size = 1, .name = "bool"}}
};
