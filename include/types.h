#pragma once 
#include <functional>
#include <ranges>
#include <string>
#include <typeindex>
#include <vector>
#include <any>
#include <unordered_map>

enum class TokenType {
    // Terminal
    EndOfFile,
    ParseError,
    // Values
    ID, StringLit,
    CharLit, IntLit,
    DoubleLit,
    // Puncts
    OCurly, CCurly, 
    OParen, CParen,
    OBracket, CBracket,
    Not, Mul, Div,
    Mod, And, Plus,
    PlusPlus, Minus,
    MinusMinus, Less,
    LessEq, Greater,
    GreaterEq, Or, Eq,
    OrOr, AndAnd,
    EqEq, NotEq, Shl,
    ShlEq, Shr, ShrEq,
    ModEq, OrEq, AndEq,
    PlusEq, MinusEq, MulEq,
    DivEq, Question,
    Colon, ColonColon,
    SemiColon, Comma,
    Hash, BSlash,
    SQoute, DQoute,
    Dot, Arrow,
    UScore,
    // Types
    //Int8_t, Int16_t, 
    //Int32_t, Int64_t, 
    //String_t, Char_t,
    //Float_t, Size_t,
    TypeID,
    // Keywords
    Extern, Case, If,
    Else, While, Switch,
    Goto, Return, Import,
    Func, From, Include,
    Module, Struct, Const,
};
enum class Type : int {
    Void_t = 0, 
    Struct_t,
    Int8_t, Int16_t, 
    Int32_t, Int64_t, 
    String_t, Char_t,
    Float_t, Size_t,
    Bool_t,
};
enum class Op {
    // stores variable1 into variable2
    // STORE_VAR(variable1, variable2)
    STORE_VAR,
    // stores the return of the last function called
    // STORE_RET(variable)
    STORE_RET,
    // INIT_STRING(string)
    INIT_STRING,
    // RETURN(variable)
    RETURN,
    // CALL(func_name, args)
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
    {"void"  , Type::Void_t},
    {"char"  , Type::Char_t},
    {"int8"  , Type::Int8_t},
    {"int16" , Type::Int16_t},
    {"int"   , Type::Int32_t},
    {"int32" , Type::Int32_t},
    {"int64" , Type::Int64_t},
    {"long"  , Type::Int64_t},
    {"string", Type::String_t},
    {"float" , Type::Float_t},
    {"usize" , Type::Size_t},
    {"bool"  , Type::Bool_t}
};
inline std::unordered_map<Type, std::string> printableTypeIds = {
    {Type::Void_t  , "void"  },
    {Type::Char_t  , "char"  },
    {Type::Int8_t  , "int8"  },
    {Type::Int16_t , "int16" },
    {Type::Int32_t , "int32" },
    {Type::Int64_t , "int64" },
    {Type::String_t, "string"},
    {Type::Float_t , "float" },
    {Type::Size_t  , "usize" },
    {Type::Bool_t  , "bool"  },
};

static const std::unordered_map<TokenType, std::string> printableToken = {
    // Terminal
    {TokenType::EndOfFile  ,"end of file"},
    {TokenType::ParseError ,"parse error"},
    // Values
    {TokenType::ID         ,"identifier"},
    {TokenType::StringLit  ,"string"},
    {TokenType::CharLit    ,"character"},
    {TokenType::IntLit     ,"integer literal"},
    {TokenType::DoubleLit  ,"double literal"},
    // Puncts
    {TokenType::OCurly     ,"`{`"},
    {TokenType::CCurly     ,"`}`"},
    {TokenType::OParen     ,"`(`"},
    {TokenType::CParen     ,"`)`"},
    {TokenType::OBracket   ,"`[`"},
    {TokenType::CBracket   ,"`]`"},
    {TokenType::Not        ,"`!`"},
    {TokenType::Mul        ,"`*`"},
    {TokenType::Div        ,"`/`"},
    {TokenType::Mod        ,"`%`"},
    {TokenType::And        ,"`&`"},
    {TokenType::AndAnd     ,"`&&`"},
    {TokenType::Plus       ,"`+`"},
    {TokenType::PlusPlus   ,"`++`"},
    {TokenType::Minus      ,"`-`"},
    {TokenType::MinusMinus ,"`--`"},
    {TokenType::Less       ,"`<`"},
    {TokenType::LessEq     ,"`<=`"},
    {TokenType::Greater    ,"`>`"},
    {TokenType::GreaterEq  ,"`>=`"},
    {TokenType::Or         ,"`|`"},
    {TokenType::OrOr       ,"`||`"},
    {TokenType::NotEq      ,"`!=`"},
    {TokenType::Eq         ,"`=`"},
    {TokenType::EqEq       ,"`==`"},
    {TokenType::Shl        ,"`<<`"},
    {TokenType::ShlEq      ,"`<<=`"},
    {TokenType::Shr        ,"`>>`"},
    {TokenType::ShrEq      ,"`>>=`"},
    {TokenType::ModEq      ,"`%=`"},
    {TokenType::OrEq       ,"`|=`"},
    {TokenType::AndEq      ,"`&=`"},
    {TokenType::PlusEq     ,"`+=`"},
    {TokenType::MinusEq    ,"`-=`"},
    {TokenType::MulEq      ,"`*=`"},
    {TokenType::DivEq      ,"`/=`"},
    {TokenType::Question   ,"`?`"},
    {TokenType::Colon      ,"`:`"},
    {TokenType::ColonColon ,"`::`"},
    {TokenType::SemiColon  ,"`;`"},
    {TokenType::Comma      ,"`,`"},
    {TokenType::Hash       ,"`#`"},
    {TokenType::BSlash     ,"`\\`"},
    {TokenType::SQoute     ,"`\'`"},
    {TokenType::DQoute     ,"`\"`"},
    {TokenType::Dot        ,"`.`"},
    {TokenType::Arrow      ,"`->`"},
    // Keyword
    {TokenType::TypeID     ,"typeid"},
    {TokenType::Extern     ,"keyword `extern`"},
    {TokenType::Case       ,"keyword `case`"},
    {TokenType::If         ,"keyword `if`"},
    {TokenType::Else       ,"keyword `else`"},
    {TokenType::While      ,"keyword `while`"},
    {TokenType::Switch     ,"keyword `switch`"},
    {TokenType::Goto       ,"keyword `goto`"},
    {TokenType::Return     ,"keyword `return`"},
    {TokenType::Import     ,"keyword `import`"},
    {TokenType::Func       ,"keyword `func`"},
    {TokenType::From       ,"keyword `from`"},
    {TokenType::Include    ,"keyword `include`"},
    {TokenType::Module     ,"keyword `module`"},
    {TokenType::Struct     ,"keyword `struct`"},
    {TokenType::Const      ,"keyword `const`"},
};

static const std::unordered_map<std::string, TokenType> PUNCTUATION = {
    {"_" ,  TokenType::UScore},
    {"\'" , TokenType::SQoute},
    {"\"" , TokenType::DQoute},
    {"."  , TokenType::Dot},
    {"->" , TokenType::Arrow},
    {"#"  , TokenType::Hash},
    {"\\" , TokenType::BSlash},
    {"?"  , TokenType::Question},
    {"{"  , TokenType::OCurly},
    {"}"  , TokenType::CCurly},
    {"("  , TokenType::OParen},
    {")"  , TokenType::CParen},
    {"["  , TokenType::OBracket},
    {"]"  , TokenType::CBracket},
    {";"  , TokenType::SemiColon},
    {"::" , TokenType::ColonColon},
    {":"  , TokenType::Colon},
    {","  , TokenType::Comma},
    {"--" , TokenType::MinusMinus},
    {"-=" , TokenType::MinusEq},
    {"-"  , TokenType::Minus},
    {"++" , TokenType::PlusPlus},
    {"+=" , TokenType::PlusEq},
    {"+"  , TokenType::Plus},
    {"*=" , TokenType::MulEq},
    {"*"  , TokenType::Mul},
    {"%=" , TokenType::ModEq},
    {"%"  , TokenType::Mod},
    {"/=" , TokenType::DivEq},
    {"/"  , TokenType::Div},
    {"|=" , TokenType::OrEq},
    {"||" , TokenType::OrOr},
    {"|"  , TokenType::Or},
    {"&=" , TokenType::AndEq},
    {"&&" , TokenType::AndAnd},
    {"&"  , TokenType::And},
    {"==" , TokenType::EqEq},
    {"="  , TokenType::Eq},
    {"!=" , TokenType::NotEq},
    {"!"  , TokenType::Not},
    {"<<=", TokenType::ShlEq},
    {"<<" , TokenType::Shl},
    {"<=" , TokenType::LessEq},
    {"<"  , TokenType::Less},
    {">>=", TokenType::ShrEq},
    {">>" , TokenType::Shr},
    {">=" , TokenType::GreaterEq},
    {">"  , TokenType::Greater},
};

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"extern" , TokenType::Extern},
    {"case"   , TokenType::Case},
    {"if"     , TokenType::If},
    {"else"   , TokenType::Else},
    {"while"  , TokenType::While},
    {"switch" , TokenType::Switch},
    {"goto"   , TokenType::Goto},
    {"return" , TokenType::Return},
    {"import" , TokenType::Import},
    {"func"   , TokenType::Func},
    {"from"   , TokenType::From},
    {"include", TokenType::Include},
    {"module" , TokenType::Module},
    {"struct" , TokenType::Struct},
    {"const"  , TokenType::Const},
};


struct Loc {
    size_t line   = 1;
    size_t offset = 1;
    std::string inputPath;
};

struct Token {
    TokenType type;
    Loc loc;
    std::string string_value;
    int64_t     int_value;
    double      double_value;
};

struct Instruction {
    Op op;
    std::vector<std::any> args;
};

// Forward Declared
struct Variable;
typedef std::vector<Variable> VariableStorage;

struct Kind {
    bool constant = false;
    bool global   = false;
    bool literal = false;
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

inline std::unordered_map <std::string, TypeInfo> type_infos = {
    {"void"  , {.id = 0 , .type = Type::Void_t  , .size = 1, .name = "void"}},
    {"char"  , {.id = 1 , .type = Type::Char_t  , .size = 1, .name = "char"}},
    {"int8"  , {.id = 2 , .type = Type::Int8_t  , .size = 1, .name = "int8"}},
    {"int16" , {.id = 3 , .type = Type::Int16_t , .size = 2, .name = "int16"}},
    {"int"   , {.id = 4 , .type = Type::Int32_t , .size = 4, .name = "int32"}},
    {"int32" , {.id = 4 , .type = Type::Int32_t , .size = 4, .name = "int32"}},
    {"int64" , {.id = 5 , .type = Type::Int64_t , .size = 8, .name = "int64"}},
    {"long"  , {.id = 5 , .type = Type::Int64_t , .size = 8, .name = "int64"}},
    {"string", {.id = 6 , .type = Type::String_t, .size = 8, .name = "string"}},
    {"float" , {.id = 7 , .type = Type::Float_t , .size = 4, .name = "float"}},
    {"usize" , {.id = 8 , .type = Type::Size_t  , .size = 8, .name = "uint64"}},
    {"bool"  , {.id = 9 , .type = Type::Bool_t  , .size = 1, .name = "bool"}}
};
inline size_t current_typeid_max = 9;

struct Variable {
    TypeInfo*   type_info = nullptr;
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

    int arguments_count = 0;
    VariableStorage arguments{};

    std::string name{};
    
    VariableStorage local_variables{};
    size_t stack_size = 0;

    bool external = false;
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
};
typedef std::vector<Struct> StructStorage;

struct Program {
    ModuleStorage   module_storage;
    FunctionStorage func_storage;
    VariableStorage var_storage;
    StructStorage   struct_storage;
};

inline std::string input_no_extention;
inline std::string input_path;
inline std::string build_path;

