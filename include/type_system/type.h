#pragma once
#include "tools/format.h"
#include "tools/logger.h"
#include "type_system/kind.h"
#include "type_system/type_info.h"
#include "type_system/typeid.h"
#include <cstdint>
#include <vector>

class Struct;
class Variable;
struct PtrData;
struct FuncData;

class Type {
public:
    Type(TypeInfo i, uint32_t qualifiers = 0);
    Type(Kind k, uint32_t qualifiers = 0);
    Type(const Type& other);
    Type() { info = type_infos.at("void");}
    ~Type();
    Type& operator=(const Type& other);
    TypeInfo info;
    uint32_t qualifiers = 0;
    Struct* struct_data = nullptr;
    PtrData* ptr_data;
    FuncData* func_data;
};
struct PtrData {
    Type* pointee;
};
struct FuncData {
    Type* return_type;
    std::vector<Type*> args{};
};
inline Type get_base_type(Type t) {
    while (t.info.kind == Kind::Pointer) {
        t = *t.ptr_data->pointee;
    }
    return t;
}
inline Type set_ptr_count(Type base, size_t count) {
    Type ptr = base;
    while (count-- > 0) {
        auto old_ptr = ptr;
        ptr = Type(Kind::Pointer);
        *ptr.ptr_data->pointee = old_ptr;
    }
    return ptr;
}
inline size_t get_ptr_count(Type t) {
    size_t i = 0;
    while (t.info.kind == Kind::Pointer) {
        i++;
        t = *t.ptr_data->pointee;
    }
    return i;
}
inline Type make_ptr(Type base) {
    Type ptr = Type(Kind::Pointer);
    *ptr.ptr_data->pointee = base;
    return ptr;
}

inline size_t get_typeid(Type t) {
    return t.info.id;
}
inline size_t get_typeid(TypeInfo t) {
    return t.id;
}
inline size_t get_typeid(std::string t) {
    if (!type_infos.contains(t)) {
        TODO(mlog::format("cannot find type {}", t));
    }
    return type_infos.at(t).id;
}
inline std::unordered_map<std::string, size_t> TypeIds = {
    {"void"   , TypeId::Void},
    {"char"   , TypeId::Char},
    {"int8"   , TypeId::Int8},
    {"int16"  , TypeId::Int16},
    {"int"    , TypeId::Int32},
    {"int32"  , TypeId::Int32},
    {"int64"  , TypeId::Int64},
    {"typeid" , TypeId::Typeid},
    {"pointer", TypeId::Ptr},
    {"usize"  , TypeId::USize},
    {"string" , TypeId::String},
    {"float"  , TypeId::Float},
    {"double" , TypeId::Float},
    {"bool"   , TypeId::Bool}
};
inline std::unordered_map<size_t, std::string> printableTypeIds = {
    {TypeId::Void  , "void"   },
    {TypeId::Char  , "char"   },
    {TypeId::Int8  , "int8"   },
    {TypeId::Int16 , "int16"  },
    {TypeId::Int32 , "int32"  },
    {TypeId::Int64 , "int64"  },
    {TypeId::Typeid, "typeid" },
    {TypeId::Ptr   , "pointer"},
    {TypeId::USize  , "usize"  },
    {TypeId::String, "string" },
    {TypeId::Float , "float"  },
    {TypeId::Double, "double" },
    {TypeId::Bool  , "bool"   },
};
