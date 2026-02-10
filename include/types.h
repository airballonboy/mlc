#pragma once 
#include <string>
#include <filesystem>
#include <typeindex>
#include <vector>
#include <any>
#include <variant>
#include <unordered_map>
#include "token.h"

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
    // (lhs, rhs, result)
    // Binary operations
    ADD, SUB, MUL, DIV, MOD,
    // Comparison
    LT, LE, GT, GE, EQ, NE,
    // Logical AND/OR
    LAND, LOR,
    // LABEL(label)
    LABEL,
    // JUMP(label)
    JUMP,
    // JUMP_IF_NOT(label, variable)
    JUMP_IF_NOT,
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

// Forward Declared
struct Variable;
typedef std::vector<Variable> VariableStorage;
struct Func;


using Arg = std::variant<std::string, Variable, VariableStorage, Func>;
struct Instruction {
    Op op;
    std::vector<Arg> args;
};

struct Kind {
    bool constant = false;
    bool global   = false;
    bool literal  = false;
    size_t  pointer_count = 0;
    int64_t deref_offset  = -1;
    size_t  array_count   = 0;
    // TODO: add a vector of array data
};

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

struct Variable {
    TypeInfo    type_info = type_infos.at("void");
    std::string name{};
    std::any    value{};
    int64_t     deref_count = 0;
    size_t      offset = 0;
    size_t      size = 0;
    Variable* parent = nullptr;
    VariableStorage members{};
    Kind kind{};
};

struct Func {
    TypeInfo return_type = type_infos.at("void");
    Kind     return_kind{};

    int arguments_count = 0;
    VariableStorage arguments{};

    std::string name{};
    
    VariableStorage local_variables{};
    size_t stack_size = 0;

    bool external = false;
    bool variadic = false;
    bool c_variadic = false;
    bool is_static = false;
    std::string link_name{};
    std::string lib{};
    std::string search_path{};

    std::vector<Instruction> body{};
};
typedef std::vector<Func> FunctionStorage;

struct Module {
    std::string name{};
    std::unordered_map<std::string, Module> module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
};
//typedef std::vector<Module> ModuleStorage;
typedef std::unordered_map<std::string, Module> ModuleStorage;

struct Struct {
    size_t id = 0;
    std::string name{};
    VariableStorage var_storage;
    std::unordered_map<int, Variable> defaults;
    size_t size;
    size_t alignment;
    bool is_float_only = false;
};
typedef std::vector<Struct> StructStorage;

enum class Platform {
    Linux,
    Windows,
};

struct Program {
    ModuleStorage   module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
    StructStorage   struct_storage;
    Platform platform;
};

namespace fs = std::filesystem;
inline fs::path input_path;
inline fs::path input_file;
inline fs::path build_path;

