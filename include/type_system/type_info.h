#pragma once
#include <string>
#include <unordered_map>
#include "type_system/typeid.h"
#include "type_system/kind.h"
class TypeInfo {
public:
    static TypeInfo get_from_id(size_t id);
    size_t id = TypeId::Void;
    Kind kind = Kind::Int;
    size_t size = 0;
    std::string name = "";
};

inline size_t current_typeid_max = TypeId::BasicTypesCount + 1;
inline std::unordered_map <std::string, TypeInfo> type_infos = {
    {"void"   , {.id = TypeId::Void  , .kind = Kind::Void  , .size = 1, .name = "void"}},
    {"char"   , {.id = TypeId::Char  , .kind = Kind::Int   , .size = 1, .name = "char"}},
    {"int8"   , {.id = TypeId::Int8  , .kind = Kind::Int   , .size = 1, .name = "int8"}},
    {"int16"  , {.id = TypeId::Int16 , .kind = Kind::Int   , .size = 2, .name = "int16"}},
    {"int"    , {.id = TypeId::Int32 , .kind = Kind::Int   , .size = 4, .name = "int32"}},
    {"int32"  , {.id = TypeId::Int32 , .kind = Kind::Int   , .size = 4, .name = "int32"}},
    {"int64"  , {.id = TypeId::Int64 , .kind = Kind::Int   , .size = 8, .name = "int64"}},
    {"typeid" , {.id = TypeId::Typeid, .kind = Kind::Int   , .size = 8, .name = "typeid"}},
    {"pointer", {.id = TypeId::Ptr   , .kind = Kind::Int   , .size = 8, .name = "pointer"}},
    {"usize"  , {.id = TypeId::USize , .kind = Kind::Int   , .size = 8, .name = "uint64"}},
    {"string" , {.id = TypeId::String, .kind = Kind::String, .size = 8, .name = "string"}},
    {"float"  , {.id = TypeId::Float , .kind = Kind::Float , .size = 4, .name = "float"}},
    {"double" , {.id = TypeId::Double, .kind = Kind::Float , .size = 8, .name = "double"}},
    {"bool"   , {.id = TypeId::Bool  , .kind = Kind::Int   , .size = 1, .name = "bool"}}
};
