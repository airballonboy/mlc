#pragma once
#include "tools/format.h"
#include "tools/logger.h"
#include "type_system/kind.h"
#include "type_system/type_info.h"
#include "type_system/struct.h"
#include "type_system/typeid.h"
#include <cstdint>
#include <vector>
#include <memory>

struct PtrData;
struct FuncData;

class Type {
public:
    Type(TypeInfo i, uint32_t qualifiers = 0);
    Type(Kind k, uint32_t qualifiers = 0);
    Type(const Type& other);
    Type() 
        :info(type_infos.at("void")),
        qualifiers(0)
    {}
    ~Type();
    Type& operator=(Type other);
    TypeInfo info;
    uint32_t qualifiers = 0;
    std::unique_ptr<Struct> struct_data;
    std::unique_ptr<PtrData> ptr_data;
    std::unique_ptr<FuncData> func_data;
};
struct PtrData {
    std::unique_ptr<Type> pointee;
};
struct FuncData {
    std::unique_ptr<Type> return_type;
    std::vector<std::unique_ptr<Type>> args;
};
Type get_base_type(Type t);
Type set_ptr_count(Type base, size_t count);
size_t get_ptr_count(Type t);
Type make_ptr(Type base);
size_t get_typeid(Type t);
size_t get_typeid(TypeInfo t);
size_t get_typeid(std::string t);
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
