#pragma once
#include <unordered_map>
#include <string>

enum class Type : int {
    Void_t = 0, 
    Struct_t,
    Int8_t, Int16_t, 
    Int32_t, Int64_t,
    Typeid_t,
    Ptr_t, Size_t,
    String_t, Char_t,
    Float_t, Double_t,
    Bool_t,
};

inline std::unordered_map<std::string, Type> TypeIds = {
    {"void"   , Type::Void_t},
    {"char"   , Type::Char_t},
    {"int8"   , Type::Int8_t},
    {"int16"  , Type::Int16_t},
    {"int"    , Type::Int32_t},
    {"int32"  , Type::Int32_t},
    {"int64"  , Type::Int64_t},
    {"typeid" , Type::Typeid_t},
    {"pointer", Type::Ptr_t},
    {"usize"  , Type::Size_t},
    {"long"   , Type::Int64_t},
    {"string" , Type::String_t},
    {"float"  , Type::Float_t},
    {"double" , Type::Double_t},
    {"bool"   , Type::Bool_t}
};
inline std::unordered_map<Type, std::string> printableTypeIds = {
    {Type::Void_t  , "void"   },
    {Type::Char_t  , "char"   },
    {Type::Int8_t  , "int8"   },
    {Type::Int16_t , "int16"  },
    {Type::Int32_t , "int32"  },
    {Type::Int64_t , "int64"  },
    {Type::Typeid_t, "typeid" },
    {Type::Ptr_t   , "pointer"},
    {Type::Size_t  , "usize"  },
    {Type::String_t, "string" },
    {Type::Float_t , "float"  },
    {Type::Double_t, "double" },
    {Type::Bool_t  , "bool"   },
};
